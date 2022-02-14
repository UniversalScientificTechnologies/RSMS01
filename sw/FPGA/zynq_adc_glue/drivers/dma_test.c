#include "shared.h"

void usage()
{
	fprintf(stderr, "Usage: dma_test [-r] [-m] [-b NO_OF_BYTES] [-f FILENAME]\n\n"
					"\t-r  dumps registers and exits\n"
					"\t-m  enables mock data source\n"
					"\t-b  burst mode, transfers the specified amount of bytes "
							"and exits\n\t    (supports k and m suffixes for units"
							" of 1024 and 1024^2)\n"
					"\t-f  writes the transferred data to the given file,"
							" '-' for stdout.\n\t    in burst mode, slow writes won't"
							" cause dropped frames\n\n");
}

void dump_regs()
{
	fprintf(stderr, "DMA engine (s2mm)\n");
	fprintf(stderr, "\tdmacr=0x%x\n", dma_regs->s2mm_dmacr);
	fprintf(stderr, "\tdmssr=0x%x\n", dma_regs->s2mm_dmasr);
	fprintf(stderr, "\tcurdesc=0x%x\n", dma_regs->s2mm_curdesc);
	fprintf(stderr, "\ttaildesc=0x%x\n", dma_regs->s2mm_taildesc);
	fprintf(stderr, "Custom\n");
	fprintf(stderr, "\tpl_design_id=0x%x ", our_regs->pl_design_id);

	switch (our_regs->pl_design_id & 0xffff0000) {
		case 0xd5170000:
			fprintf(stderr, "(dev series no. %d)\n",
					our_regs->pl_design_id & 0xffff);
			break;

		default:
			fprintf(stderr, "(unknown)\n");
			break;
	}

	fprintf(stderr, "\tcontrol=0x%x\n", our_regs->control);
	fprintf(stderr, "\tframe_counter=0x%x\n", our_regs->frame_counter);
	fprintf(stderr, "\toverflow_counter=0x%x\n", our_regs->overflow_counter);
	fprintf(stderr, "\t[0x10]=0x%x\n", *(((uint32_t *) our_regs) + 4));
	fprintf(stderr, "\t[0x14]=0x%x\n", *(((uint32_t *) our_regs) + 5));
	fprintf(stderr, "\t[0x18]=0x%x\n", *(((uint32_t *) our_regs) + 6));
}

int atoi_suffixed(const char *s)
{
	int unit;

	int c = s[0] != '\0'? s[strlen(s)-1] : 0;
	switch (c) {
		case 'k': case 'K':
			unit = 1024;
			break;

		case 'm': case 'M':
			unit = 1024 * 1024;
			break;

		default:
			unit = 1;
	}

	return atoi(s) * unit;
}

int main(int argc, char const *argv[])
{
	assert(sizeof(dma_regs_t) == 0x44);
	init_mmapings();

	int cr_mode_select = 0;

	int burst = 0;
	size_t burst_nbytes = 0;

	int observe_sync = 0;

	int output_fd = -1;

	int c;
	while ((c = getopt(argc, argv, "mrb:f:c:ts:")) != -1) {
		switch (c) {
			case 'm':
				cr_mode_select |= 0x4;
				break;

			case 'r':
				dump_regs();
				return 0;

			case 'b':
				burst = 1;
				burst_nbytes = atoi_suffixed(optarg);
				break;

			case 'f':
				if (strcmp(optarg, "-") == 0) {
					output_fd = STDOUT_FILENO;
				} else {
					output_fd = open(optarg, O_WRONLY | O_CREAT | O_TRUNC);
					if (output_fd < 0)
						err(1, "open '%s'", optarg);
				}
				break;

			case 'c':
				cr_mode_select |= atoi(optarg);
				break;

			case 't':
				our_regs->control = 0x2;
				return 0;

			case 's':
				observe_sync = atoi(optarg);
				break;

			default:
				usage();
				return 1;
		}
	}

	argc -= optind; argv += optind;

	init_descriptors();

	{ /* talk to our logic */
		fprintf(stderr, "pl_design_id=0x%x\n", our_regs->pl_design_id);

		our_regs->control = 0x2 | cr_mode_select; /* reset */
	}

	{ /* set the DMA to a known state first */
		dma_regs->s2mm_dmacr = 0x4; /* triggers a reset */
		usleep(20000);
		fprintf(stderr, "(after reset) s2mm_dmasr=0x%x\n", dma_regs->s2mm_dmasr);
	}

	const uint32_t rb_piece_size = 0x00400000; /* span covered by one descriptor */
	assert(rb_piece_size <= DESC_BUFLEN_MAX);

	const uint32_t rb_start_addr = 0x32000000;
	uint32_t rb_end_addr = rb_start_addr + (burst ? burst_nbytes : 0x8000000);

	if (rb_end_addr > RESMEM_BASE_ADDR + RESMEM_LENGTH)
		errx(1, "out of reserved memory");

	desc_t *head_desc = 0;
	desc_t *tail_desc = 0;

	{ /* set up the first batch of descriptors. */
		desc_t *prev = 0;

		uint32_t addr = rb_start_addr;
		while (addr < rb_end_addr) {
			int desc_buflen = rb_piece_size;
			if (addr + desc_buflen > rb_end_addr)
				desc_buflen = rb_end_addr - addr;

			desc_t *desc = desc_alloc();
			desc->buffer_address = addr;
			desc->control = desc_buflen & DESC_BUFLEN_MASK;

			if (prev) {
				desc_set_next(prev, desc);
			} else {
				head_desc = desc;
			}

			addr += desc_buflen;
			prev = desc;
		}

		desc_set_next(prev, head_desc);
		tail_desc = prev;
	}

	{ /* enable the DMA, but do not trigger a descriptor fetch */
		dma_regs->s2mm_curdesc = virt_to_phys(head_desc);
		dma_regs->s2mm_dmacr |= 0x01;

		usleep(10000);
		fprintf(stderr, "(after s2mm_dmacr write) s2mm_dmasr=0x%x\n", dma_regs->s2mm_dmasr);
	}

	{ /* clear the target memory for the first descriptor. for the subsequent ones,
		 memory is not cleared. this is done just to see that the memory indeed
		 gets overwritten. */
		memset(phys_to_virt(head_desc->buffer_address), 0, head_desc->control);
	}

	{ /* trigger a descriptor fetch by writing to s2mm_taildesc */
		dma_regs->s2mm_taildesc = virt_to_phys(tail_desc);
	}

	{ /* enable the data source */
		our_regs->control = 0x1 | cr_mode_select;
	}

	uint32_t prev_sync = our_regs->last_sync;

	{
		uint64_t transferred_bytes = 0;

		while (1) {
			/* poll the head descriptor for completion */
			while (!(head_desc->status & DESC_STATUS_CMPLT))
				usleep(1000);

			transferred_bytes += head_desc->status & DESC_BUFLEN_MASK;

			uint32_t curr_sync = our_regs->last_sync;
			if (observe_sync && curr_sync != prev_sync) {
				/* whole part */
				int whole = ((curr_sync & 0xffffff00) - (prev_sync & 0xffffff00)) >> 8;
				int fraction_max = observe_sync;
				int fraction;
				fraction = (fraction_max + (prev_sync & 0xff) - (curr_sync & 0xff));
				if (fraction < fraction_max)
					whole -= 1;
				else
					fraction -= fraction_max;

				if (prev_sync != 0xffffffff)
					printf("%d.%d\n", whole, fraction);

				prev_sync = curr_sync;
			}

			if (!observe_sync) {
				fprintf(stderr, "\rtransferred=0x%"PRIx64" counter=0x%08x overflow=0x%08x",
						transferred_bytes, our_regs->frame_counter, our_regs->overflow_counter);
			}

			if (output_fd != -1) {
				void *addr = phys_to_virt(head_desc->buffer_address);
				int buflen = head_desc->status & DESC_BUFLEN_MASK;

				if (write(output_fd, addr, buflen) != buflen)
					warn("short write");
			}

			if (burst) {
				if (head_desc == tail_desc)
					break;

				head_desc = desc_get_next(head_desc);
			} else {
				/* clear the descriptor before reuse */
				head_desc->status = 0;

				/* shift the pointers */
				tail_desc = head_desc;
				head_desc = desc_get_next(head_desc);

				dma_regs->s2mm_taildesc = virt_to_phys(tail_desc);
			}
		}

		fprintf(stderr, "\n");
	}

	return 0;
}
