/*
	The DMA block is called 'AXI DMA v7.1', documentation can be found here:
	https://www.xilinx.com/support/documentation/ip_documentation/axi_dma/v7_1/pg021_axi_dma.pdf

	It is configured for descriptors with 24-bit wide buffer lengths (out of the 26-bit maximum).
*/

#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <errno.h>
#include <err.h>

/* for printing uint64 */
#define __STDC_FORMAT_MACROS
#include <inttypes.h>

#define DMA_REGS_BASE_ADDR	0x60000000

#define RESMEM_BASE_ADDR	0x30000000
#define RESMEM_LENGTH		0x10000000

/* registers of the DMA engine at 0x60000000 */
typedef struct {
/* +0x00 */ uint32_t mm2s_dmacr;
/* +0x04 */ uint32_t mm2s_dmasr;
/* +0x08 */ uint32_t mm2s_curdesc;
/* +0x0c */ uint32_t _res0;
/* +0x10 */ uint32_t mm2s_taildesc;
/* +0x14 */ uint32_t _res1[6];
/* +0x2c */ uint32_t sg_ctl;
/* +0x30 */ uint32_t s2mm_dmacr;
/* +0x34 */ uint32_t s2mm_dmasr;
/* +0x38 */ uint32_t s2mm_curdesc;
/* +0x3c */ uint32_t _res2;
/* +0x40 */ uint32_t s2mm_taildesc;
} __attribute__((packed)) dma_regs_t;

/* our registers, one page above the DMA's, i.e. at 0x60001000 */
typedef struct {
/* +0x00 */ uint32_t pl_design_id;
/* +0x04 */ uint32_t control;
/* +0x08 */ uint32_t frame_counter;
/* +0x0c */ uint32_t overflow_counter;
} __attribute__((packed)) our_regs_t;

/* must be aligned to 0x40 bytes */
typedef struct {
/* +0x00 */ uint32_t nxtdesc;
/* +0x04 */ uint32_t _res0;
/* +0x08 */ uint32_t buffer_address;
/* +0x0c */ uint32_t _res1[3];
/* +0x18 */ uint32_t control;
/* +0x1c */ uint32_t status;
} __attribute__((packed)) desc_t;

#define DESC_BUFLEN_MAX		(1<<24)
#define DESC_BUFLEN_MASK	DESC_BUFLEN_MAX-1

#define DESC_CONTROL_TXEOF (1<<26)
#define DESC_CONTROL_TXSOF (1<<27)

#define DESC_CONTROL_RXEOF (1<<26)
#define DESC_CONTROL_RXSOF (1<<27)

#define DESC_STATUS_CMPLT  (1<<31)

int devmem_fd;
dma_regs_t *dma_regs;
our_regs_t *our_regs;
void *resmem_ptr;

void init_mmapings()
{
	devmem_fd = open("/dev/mem", O_RDWR|O_SYNC);
	if (devmem_fd < 0) err(1, "opening /dev/mem");

	dma_regs = (dma_regs_t *) mmap(NULL, 2*0x1000, PROT_READ|PROT_WRITE,
								   MAP_SHARED, devmem_fd, DMA_REGS_BASE_ADDR);
	if (dma_regs == MAP_FAILED) err(1, "mmaping registers");
	our_regs = (our_regs_t *) (((void *) dma_regs) + 0x1000);

	resmem_ptr = mmap(NULL, RESMEM_LENGTH, PROT_READ|PROT_WRITE,
					  MAP_SHARED, devmem_fd, RESMEM_BASE_ADDR);
	if (resmem_ptr == MAP_FAILED) err(1, "mmaping reserved memory");
}

void *phys_to_virt(uint32_t phys)
{
	if (phys == 0) return NULL;
	return resmem_ptr + (phys - RESMEM_BASE_ADDR);
}

uint32_t virt_to_phys(void *virt)
{
	if (virt == NULL) return 0;
	return RESMEM_BASE_ADDR + (uint32_t) (virt - resmem_ptr);
}

void desc_set_next(desc_t *desc, desc_t *next)
{
	assert((((uint32_t) next) & 0x3f) == 0); /* check alignment */
	desc->nxtdesc = virt_to_phys(next);
}

desc_t *desc_get_next(desc_t *desc)
{
	return phys_to_virt(desc->nxtdesc);
}

desc_t *desc_freelist_first, *desc_freelist_last;

void init_descriptors()
{
	desc_t *prev = NULL;

	/* devote the first 4kB of reserved memory to descriptors */
	void *p;
	for (p = resmem_ptr; p < resmem_ptr + 4096; p += 0x40) {
		desc_t *desc = (desc_t *) p;
 
		if (prev != NULL) {
			desc_set_next(prev, desc);
		} else {
			desc_freelist_first = desc;
		}

		prev = desc;
	}

	desc_set_next(prev, NULL);
	desc_freelist_last = prev;
}

void desc_free(desc_t *desc)
{
	if (desc_freelist_last != NULL)
		desc_set_next(desc_freelist_last, desc);
	else
		desc_freelist_first = desc;

	desc_set_next(desc, NULL);
	desc_freelist_last = desc;
}

desc_t *desc_alloc()
{
	desc_t *ret = desc_freelist_first;

	if (ret == NULL)
		errx(1, "out of free descriptors");

	assert((desc_get_next(ret) == NULL) == (ret == desc_freelist_last));

	desc_freelist_first = desc_get_next(ret);
	if (ret == desc_freelist_last)
		desc_freelist_last = NULL;

	memset(ret, 0, sizeof(desc_t));
	return ret;
}

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
	init_descriptors();

	int cr_mode_select = 0;

	int burst = 0;
	size_t burst_nbytes = 0;

	int output_fd = -1;

	int c;
	while ((c = getopt(argc, argv, "mrb:f:")) != -1) {
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

			default:
				usage();
				return 1;
		}
	}

	argc -= optind; argv += optind;

	{ /* talk to our logic */
		fprintf(stderr, "pl_design_id=0x%x\n", our_regs->pl_design_id);

		our_regs->control = 0x2 | cr_mode_select; /* reset */
	}

	{ /* set the DMA to a known state first */
		dma_regs->s2mm_dmacr = 0x4; /* triggers a reset */
		usleep(20000);
		fprintf(stderr, "(after reset) s2mm_dmasr=0x%x\n", dma_regs->s2mm_dmasr);
	}

	const uint32_t rb_piece_size = 0x00200000; /* span covered by one descriptor */
	assert(rb_piece_size <= DESC_BUFLEN_MAX);

	const uint32_t rb_start_addr = 0x32000000;
	uint32_t rb_end_addr = rb_start_addr + (burst ? burst_nbytes : rb_piece_size*32);

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

	{
		uint64_t transferred_bytes = 0;

		while (1) {
			/* poll the head descriptor for completion */
			while (!(head_desc->status & DESC_STATUS_CMPLT))
				usleep(1000);

			transferred_bytes += head_desc->status & DESC_BUFLEN_MASK;

			fprintf(stderr, "\rtransferred=0x%"PRIx64" counter=0x%08x overflow=0x%08x",
					transferred_bytes, our_regs->frame_counter, our_regs->overflow_counter);

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
