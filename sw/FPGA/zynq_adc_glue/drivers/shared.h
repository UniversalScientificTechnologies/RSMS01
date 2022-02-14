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
/* +0x10 */ uint32_t _res0;
/* +0x14 */ uint32_t last_sync;
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

#define DESC_STATUS_CMPLT  ((uint32_t) (1<<31))

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
	for (p = resmem_ptr; p < resmem_ptr + 0x1000000; p += 0x40) {
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

void ringbufcpy(void *rbstart, size_t rblen,
				size_t copyoff, size_t copylen, void *target)
{
	int piece;

	while (copylen > 0) {
		copyoff = (copyoff % rblen + rblen) % rblen;
		piece = copylen;
		if (copyoff + piece > rblen)
			piece = rblen - copyoff;
		memcpy(target, rbstart + copyoff, piece);
		copyoff += piece;
		target += piece;
		copylen -= piece;
	}
}
