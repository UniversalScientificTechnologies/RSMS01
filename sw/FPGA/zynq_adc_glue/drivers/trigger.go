package main

import (
	"fmt"
	"log"
	"os"
	"path"
	"time"
	"unsafe"
	"bytes"
	"os/exec"
	"os/signal"

	"encoding/binary"
	"golang.org/x/sys/unix"

	"sync"
	"flag"
	"runtime"
)

// #include "shared.h"
import "C"

func Sh(shCmd string) {
	cmd := exec.Command("/bin/sh", "-c", shCmd)
	err := cmd.Run()
	if err != nil {
		log.Printf("running '%s': %v", shCmd, err.Error())
	}
}

func WatchTrigger(dir string, edge string) chan struct{} {
	Sh(fmt.Sprintf("echo %s > %s", edge, path.Join(dir, "edge")))

	fd, err := unix.Open(path.Join(dir, "value"), unix.O_RDONLY|unix.O_NONBLOCK, 0)
	if err != nil {
		log.Fatalf("trigger open: %s", err)
	}

	ch := make(chan struct{})

	go func() {
		defer unix.Close(fd)

		for {
			fds := []unix.PollFd{
				{int32(fd), unix.POLLPRI, 0},
			}

			if _, err := unix.Poll(fds, -1); err != nil {
				log.Fatalf("trigger poll: %s", err)
			}

			if fds[0].Revents&unix.POLLPRI != 0 {
				unix.Seek(fd, 0, os.SEEK_SET)
				unix.Read(fd, make([]byte, 128))
				ch <- struct{}{}
			}
		}
	}()

	return ch
}

type descTarget struct {
	base   uint32
	len    uint32
	donech chan<- struct{}
}

func (t descTarget) fill(d *C.desc_t) {
	d.buffer_address = C.uint32_t(t.base)
	d.control = C.uint32_t(t.len & C.DESC_BUFLEN_MASK)
	d.status = 0
}

func (t descTarget) done() {
	if t.donech != nil {
		close(t.donech)
	}
}

func setupFirstDescBatch(depth int64, targetch <-chan descTarget) (head *C.desc_t, tail *C.desc_t, total int64) {
	total = 0

	var prev *C.desc_t
	prev = nil
	for i := int64(0); i < depth; i++ {
		desc := C.desc_alloc()

		t := <-targetch
		t.fill(desc)
		total += int64(t.len)

		if prev != nil {
			C.desc_set_next(prev, desc)
		} else {
			head = desc
		}

		prev = desc
	}

	C.desc_set_next(prev, head)
	tail = prev
	return
}

type SyncPos struct { w int64; f uint8 }
func (a SyncPos) LaterThan(b SyncPos) bool {
	return a.w >= b.w
}
type SyncLog struct {
	sync.RWMutex
	v []SyncPos
}

var syncLog SyncLog

func (k *SyncLog) Append(p SyncPos) {
	k.Lock()
	defer k.Unlock()

	minLen := 128
	maxLen := minLen+minLen/2

	k.v = append(k.v, p)
	if len(k.v) >= maxLen {
		new := make([]SyncPos, minLen, maxLen)
		copy(new[:], k.v[len(k.v)-minLen:])
		k.v = new
	}
}

func (k *SyncLog) CopyLastN(N int) []SyncPos {
	k.Lock()
	defer k.Unlock()

	if len(k.v) < N {
		N = len(k.v)
	}

	ret := make([]SyncPos, N)
	copy(ret, k.v[len(k.v)-N:])
	return ret
}

func (k *SyncLog) CopySince(thr SyncPos) []SyncPos {
	k.Lock()
	defer k.Unlock()

	N := 0
	for N < len(k.v) {
		if thr.LaterThan(k.v[len(k.v)-1-N]) {
			break
		}
		N += 1
	}

	ret := make([]SyncPos, N)
	copy(ret, k.v[len(k.v)-N:])
	return ret
}

func (k *SyncLog) Copy() []SyncPos {
	k.Lock()
	defer k.Unlock()

	ret := make([]SyncPos, len(k.v))
	copy(ret, k.v)
	return ret
}

func (k *SyncLog) Print() {
	k.RLock()
	defer k.RUnlock()

	for _, p := range k.v {
		fmt.Printf("[%d %d] ", p.w, p.f)
	}
	fmt.Printf("\n")
}

func Transfer(chainDepth int64, targetch <-chan descTarget) {
	head, tail, bytesTotal := setupFirstDescBatch(chainDepth, targetch)

	log.Printf("descriptor chain set up, enabling source")

	{ /* enable the DMA, but do not trigger a descriptor fetch */
		C.dma_regs.s2mm_curdesc = C.virt_to_phys(unsafe.Pointer(head))
		C.dma_regs.s2mm_dmacr |= 0x01

		time.Sleep(10 * time.Millisecond)
	}

	{ /* trigger a descriptor fetch by writing to s2mm_taildesc */
		C.dma_regs.s2mm_taildesc = C.virt_to_phys(unsafe.Pointer(tail))
	}

	{ /* enable the data source */
		C.our_regs.control |= 0x01
	}

	syncch := make(chan SyncPos)
	go func() {
		for v := range syncch {
			syncLog.Append(v)
		}
	}()

	lastReadSync := C.our_regs.last_sync;
	knownOverflowBytes := int64(-chainDepth*int64(descSpan))
	knownOverflow := C.our_regs.overflow_counter;

	for {
		for (head.status & C.uint32_t(C.DESC_STATUS_CMPLT)) == 0 {
			time.Sleep(time.Millisecond)
		}

		{
			t := <-targetch
			t.fill(head)
			bytesTotal += int64(t.len)
		}

		tail = head
		head = C.desc_get_next(head)

		C.dma_regs.s2mm_taildesc = C.virt_to_phys(unsafe.Pointer(tail))

		if lastReadSync != C.our_regs.last_sync {
			lastReadSync = C.our_regs.last_sync

			/* convert the sync value to an absolute sample position */
			whole := (lastReadSync >> 8) & 0x00ffffff
			fraction := uint8(lastReadSync & 0xff)
			_ = fraction

			/* ASSUMPTION: all descriptors follow descSpan */
			pos := (bytesTotal - depth*int64(descSpan)) / 16
			rel := (0x01000000 + int64(whole) - pos) % 0x01000000
			rel  = (0x01000000 + rel) % 0x01000000

			if rel > 0x01000000>>1 {
				rel -= 0x01000000
			}

			if rel > 3*int64(descSpan)/16 || rel < -(0x01000000>>2) {
				log.Printf("dropping sync (%x): rel too big (%d)", uint32(lastReadSync), rel)
			} else {
				log.Printf("abs sync %d\n", pos+rel)
				syncch <- SyncPos{ pos+rel, fraction }
			}
		}

		if (head.status & C.uint32_t(C.DESC_STATUS_CMPLT)) == 0 {
			readOverflow := C.our_regs.overflow_counter
			transferredBytes := bytesTotal - chainDepth*int64(descSpan)
			if readOverflow != knownOverflow {
				delta := uint32(readOverflow - knownOverflow)
				log.Printf("overflow detected: dropped 0x%x frames between bytes total 0x%x and 0x%x\n",
						   delta, knownOverflowBytes, transferredBytes)
				knownOverflow = readOverflow
			}
			knownOverflowBytes = transferredBytes
		}
	}

	close(syncch)
}

type Ringbuf struct {
	lo, hi uint32
	pos    uint32
	abs    int64
}

func (r Ringbuf) Size() uint32 {
	return r.hi - r.lo
}

func (r Ringbuf) Ptr() uint32 {
	return r.lo + r.pos
}

func (r *Ringbuf) Advance(s uint32) {
	r.pos += s
	r.abs += int64(s)

	l := r.hi - r.lo
	for r.pos >= l {
		r.pos -= l
	}
}

func sigintStack() {
	c := make(chan os.Signal, 1)
	signal.Notify(c, os.Interrupt)
	go func(){
    	for _ = range c {
			buf := make([]byte, 1<<16)
			runtime.Stack(buf, true)
			fmt.Printf("%s", buf)
        	os.Exit(0)
    	}
	}()
}

func (r *Ringbuf) AbsToPtr(abs int64) uint32 {
	l := r.hi - r.lo

	if abs > r.abs || abs <= r.abs-int64(l) {
		panic(fmt.Sprintf("AbsToPtr called with an out-of-reach abs (arg=%d curr=%d)", abs, r.abs))
	}

	return r.lo + uint32(abs%int64(l))
}

func fileWrite(fn string, piecech <-chan []byte, donech chan<- struct{}) {
	/* get last 10 sync values */
	sync := syncLog.CopyLastN(5)
	syncWait := time.After(5 * time.Second)

	if err := os.MkdirAll(path.Dir(fn), 750); err != nil {
		log.Printf("could not create directories: %s", err)
	}

	f, err := os.OpenFile(fn, os.O_WRONLY|os.O_CREATE, 755)
	if err != nil {
		log.Printf("opening %s: %s", fn, err)
		f, _ = os.OpenFile(os.DevNull, os.O_WRONLY, 0)
	} else {
		defer f.Close()
	}	

	for piece := range piecech {
		/* TODO: f.Write ret */
		_, err = f.Write(piece)
		if err != nil {
			log.Printf("%s", err)
		}
	}
	close(donech)

	<-syncWait
	if len(sync) != 0 {
		sync = append(sync, syncLog.CopySince(sync[len(sync)-1])...)
	} else {
		sync = syncLog.Copy()
	}

	for _, pos := range sync {
		binary.Write(f, binary.LittleEndian, pos.w)
		binary.Write(f, binary.LittleEndian, pos.f)
	}
}

func recHeader(startOffset int64, descSpan uint32, preTrigger int64, postTrigger int64, fractBase int) []byte {
	var buf bytes.Buffer

	binary.Write(&buf, binary.LittleEndian, startOffset)
	binary.Write(&buf, binary.LittleEndian, int64(descSpan))
	binary.Write(&buf, binary.LittleEndian, preTrigger)
	binary.Write(&buf, binary.LittleEndian, postTrigger)
	binary.Write(&buf, binary.LittleEndian, int64(fractBase))

	return buf.Bytes()
}

func physRangeToSlice(lo uint32, hi uint32) []byte {
	virtLo := C.phys_to_virt(C.uint32_t(lo))
	return (*[1 << 28]byte)(unsafe.Pointer(virtLo))[: hi-lo : hi-lo]
}

func min(a, b int64) int64 {
	if a < b {
		return a
	} else {
		return b
	}
}

func abs(a int64) int64 {
	if a >= 0 {
		return a
	} else {
		return -a
	}
}

const (
	sampleBytes int64 = 16
)

var (
	descSpan uint32 = 0x100000 /* 1 MiB */
	depth    int64  = 10

	savePreTrigger  int64 = 4 /* in units of descSpan */
	savePostTrigger int64 = 4

	noFIR bool    = false
	recDir string = ""
)

func init() {
	flag.Int64Var(&savePreTrigger, "pre", 4, "no. of MiB blocks to save that were sampled before the trigger")
	flag.Int64Var(&savePostTrigger, "post", 4, "no. of MiB blocks to save that were sampled after the trigger")
	flag.Int64Var(&depth, "depth", 10, "depth of the descriptor chain maintained for the DMA engine")

	flag.BoolVar(&noFIR, "nofir", false, "bypass the FIR filter in programmable logic")
	flag.StringVar(&recDir, "recdir", "rec", "path to directory to save the recordings into (missing nodes will be created)")
}

func main() {
	flag.Parse()

	go sigintStack()

	C.init_mmapings()
	C.init_descriptors()

	{ /* talk to our logic */
		fmt.Fprintf(os.Stderr, "pl_design_id=0x%x\n", C.our_regs.pl_design_id)

		if noFIR {
			C.our_regs.control = 0x2 | 0x100
		} else {
			C.our_regs.control = 0x2
		}
		
	}

	{ /* set the DMA to a known state first */
		C.dma_regs.s2mm_dmacr = 0x4 /* triggers a reset */
		time.Sleep(20 * time.Millisecond)
	}

	scratchRb := Ringbuf{lo: 0x31000000, hi: 0x32000000}
	mainRb := Ringbuf{lo: 0x32000000, hi: 0x40000000}

	if !(scratchRb.Size()%descSpan == 0 && mainRb.Size()%descSpan == 0 &&
		uint32(savePreTrigger+savePostTrigger)*descSpan < mainRb.Size() &&
		uint32(savePreTrigger+depth)*descSpan < mainRb.Size()) {
		panic("assertions failed")
	}

	trigger := WatchTrigger("/sys/class/gpio/gpio960", "rising")

	targetch := make(chan descTarget)
	go Transfer(depth, targetch)

	totalSamples := int64(0)

	for {
		rb := mainRb

	loop:
		for {
			target := descTarget{base: rb.Ptr(), len: descSpan}

			select {
			case targetch <- target:
				rb.Advance(descSpan)

			case <-trigger:
				if rb.abs < int64(depth)*int64(descSpan) {
					log.Printf("trigger came too soon, ignoring")
				} else {
					break loop
				}
			}
		}

		total := totalSamples*int64(sampleBytes) + rb.abs - depth*int64(descSpan)
		log.Printf("trigger at bytes total 0x%x\n", total)

		donech := make(chan struct{})
		piecech := make(chan []byte, 1024)
		fn := fmt.Sprintf("rec_%s", time.Now().Format("060102-150405.00"))

		actualPreTrigger := savePreTrigger
		if actualPreTrigger*int64(descSpan) > rb.abs - depth*int64(descSpan) {
			actualPreTrigger = rb.abs/int64(descSpan) - depth
		}

		offset := totalSamples + (rb.abs - (depth+actualPreTrigger)*int64(descSpan))/sampleBytes

		go fileWrite(path.Join(recDir, fn), piecech, donech)

		var fractBase int
		if noFIR {
			fractBase = 12
		} else {
			fractBase = 48
		}
		piecech <- recHeader(
			offset, descSpan, actualPreTrigger, savePostTrigger, fractBase,
		)

		log.Printf("triggered, writing to %s", fn)

		/* now, every time targetch consumes one target, we know that the target
		   that was sent 'depth' targets ago must have been filled by the DMA engine. */

		rb2 := scratchRb
		mainRbTargets := savePostTrigger - depth

		rbReadPos := rb.abs - (depth+savePreTrigger)*int64(descSpan)
		rbDMAPos := rb.abs - depth*int64(descSpan)
		rbReadEnd := rb.abs + (savePostTrigger-depth)*int64(descSpan)

		for {
			for rbReadPos < min(rbDMAPos, rbReadEnd) {
				if rbReadPos >= 0 {
					ptr := rb.AbsToPtr(rbReadPos)
					piecech <- physRangeToSlice(ptr, ptr+descSpan)
				}
				rbReadPos += int64(descSpan)
			}

			log.Printf("rbReadPos=%d rbDMAPos=%d rbReadEnd=%d rb=%d", rbReadPos, rbDMAPos, rbReadEnd, rb.abs)

			if rbReadPos >= rbReadEnd {
				break
			}

			var target descTarget
			if mainRbTargets > 0 {
				target = descTarget{base: rb.Ptr(), len: descSpan}
				rb.Advance(descSpan)
				mainRbTargets -= 1
			} else {
				target = descTarget{base: rb2.Ptr(), len: descSpan}
				rb2.Advance(descSpan)
			}

			targetch <- target
			rbDMAPos += int64(descSpan)
		}
		close(piecech)

		log.Printf("waiting for writing to finish")

	loop2:
		for {
			target := descTarget{base: rb2.Ptr(), len: descSpan}

			select {
			case targetch <- target:
				rb2.Advance(descSpan)

			case _, _ = <-donech:
				break loop2
			}
		}

		log.Printf("done writing (%d samples after the recorded interval dropped)", rb2.abs)

		totalSamples += (rb.abs + rb2.abs) / sampleBytes
	}
}
