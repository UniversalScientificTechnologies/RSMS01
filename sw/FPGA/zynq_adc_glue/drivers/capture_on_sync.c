#include "shared.h"

int main(int argc, char const *argv[])
{
	assert(sizeof(dma_regs_t) == 0x44);
	init_mmapings();

	int cr_mode_select = 0;

	int c;
	while ((c = getopt(argc, argv, "c:p:")) != -1) {
		switch (c) {
			case 'c':
				cr_mode_select |= atoi(optarg);
				break;

			default:
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

	const uint32_t rb_piece_size = 0x00100000; /* span covered by one descriptor */
	assert(rb_piece_size <= DESC_BUFLEN_MAX);

	const uint32_t rb_start_addr = 0x32000000;
	uint32_t rb_size = 0x8000000;
	uint32_t rb_end_addr = rb_start_addr + rb_size;

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

	{ /* trigger a descriptor fetch by writing to s2mm_taildesc */
		dma_regs->s2mm_taildesc = virt_to_phys(tail_desc);
	}

	{ /* enable the data source */
		our_regs->control = 0x1 | cr_mode_select;
	}

	int sync_fract = (cr_mode_select & 0x100) != 0 ? 12 : 48;

	uint32_t last_read_sync = our_regs->last_sync;
	uint32_t last_handled_sync = last_read_sync;

	{
		uint64_t transferred_bytes = 0;

		while (1) {
			/* poll the head descriptor for completion */
			while (!(head_desc->status & DESC_STATUS_CMPLT))
				usleep(1000);

			transferred_bytes += head_desc->status & DESC_BUFLEN_MASK;
			assert(transferred_bytes % 16 == 0);

			uint32_t last_sync = our_regs->last_sync;
			if (last_sync != last_handled_sync) {
				int whole = last_sync >> 8;
				uint32_t bufpos = (uint32_t) (transferred_bytes >> 4 & 0x00ffffff);
				int rel = ((int32_t) (0x01000000 + whole - bufpos)) % 0x01000000;
				if (rel > 0x1000000 >> 1)
					rel -= 0x1000000;

				if (abs(rel) > 0x1000000 / 4) { /* ~0.4 sec at 10 msps */ 
					fprintf(stderr, "dropping edge: too far from current bufpos (rel: %d)", rel);
					last_handled_sync = last_sync;
				} else if (rel < -1024) {
					uint8_t scratch[65536];
					ringbufcpy(
						phys_to_virt(rb_start_addr),
						rb_size,
						/* TODO */
						(transferred_bytes % rb_size) + rel * 16 - sizeof(scratch)/2,
						sizeof(scratch), scratch
					);

					fprintf(stderr, "we have a go! %02x%02x%02x%02x\n",
							scratch[0], scratch[1], scratch[2], scratch[3]);

					int scratch_subsample = (sizeof(scratch)/16/2+1)*sync_fract \
											- (last_sync & 0xff);

					write(STDOUT_FILENO, &sync_fract, 4);
					write(STDOUT_FILENO, &scratch_subsample, 4);
					write(STDOUT_FILENO, scratch, sizeof(scratch));

					last_handled_sync = last_sync;
				}
			}
			last_read_sync = last_sync;

			/* clear the descriptor before reuse */
			head_desc->status = 0;

			/* shift the pointers */
			tail_desc = head_desc;
			head_desc = desc_get_next(head_desc);

			dma_regs->s2mm_taildesc = virt_to_phys(tail_desc);
		}

		fprintf(stderr, "\n");
	}

	return 0;
}
