# zynq_adc_glue

These are sources for the programmable logic layer between the ADCs and the Parallella's ARM processor.

## Build Prerequisities

 * [Xilinx Vivado](https://www.xilinx.com/products/design-tools/vivado.html) v2018.2
   The free WebPACK edition works. Note the version.

 * [Device Tree Compiler](https://git.kernel.org/pub/scm/utils/dtc/dtc.git/plain/Documentation/manual.txt)
   Converts device trees between their source text and blob format. Usually packaged as 'dtc'.

Make sure that the binaries `vivado` and `dtc` are in `PATH`.

## Why There's No Project File

Vivado projects are not well-suited for being managed by a version control system. For this reason, there's 'create_project.tcl' instead, which is a Vivado TCL script for recreating the project from scratch. Vivado generates these scripts upon request. The TCL script contains the build settings and the block design in a diffable text format. More about this in Xilinx's [application note](https://www.xilinx.com/support/documentation/application_notes/xapp1165.pdf).

## make Commands

Currently the bitfile cannot be produced by calling `make` alone. After checkout and recreation of the Vivado project file, you must use the GUI to export the `glue` block design as an IP core, say into `../ip_repo`, and then you must update reference to this IP core from within the `top_block` block design. Then, the bitfile can be build as below or with the GUI.

	make

Generates all the build products. It starts with the Vivado project, which is generated in 'vivado_prj/', then it builds the bitfile as 'build/zynq_adc_glue.bit', and finally converts the bitfile into a 'build/parallella.bit.bin', which can be used to program the Zynq. Also creates the device tree blob.

	make vivado_prj/system.xpr

Creates the Vivado project only. The project can be then opened with the Vivado GUI.

	make clean

Removes both the build products in 'build/' and all the project files in 'vivado_prj/'.

	make write_project_script

This rewrites 'create_project.tcl' according to the local Vivado project so that changes made there can be commited.

## Device Tree

Linux is informed of the relevant Zynq configuration via a so-called [device tree](https://www.kernel.org/doc/Documentation/devicetree/usage-model.txt). For reasons that haven't been searched for yet, the configuration in the produced bitfile is not compatible with the one described by Adapteva's device trees to the point that Linux freezes during boot. 'devicetree.dts' contains source for a custom device tree based on the one Xilinx's tools [generated for the design](http://www.wiki.xilinx.com/Build+Device+Tree+Blob). Besides making the system bootable, this device tree also:

 * excludes the top 256 MiB of memory from kernel usage, so that it can be safely used as a target for DMA transfers
 * enables the FCLK3 clock, which was formerly used by the programmable logic when generating mock data

To use the custom device tree, it first needs to be compiled into a blob by the Device Tree Compiler. The makefile has a rule for that with the resultant blob being at 'build/devicetree.dtb'.

## Testing the Result

To boot the Parallella with the custom PL configuration, one replaces `parallella.bit.bin` and `devicetree.dtb` on the boot partition with the build results from `build/`.

To trigger a DMA transfer and, optionally, retrieve the transferred data, use the `dma_test` tool.

	Usage: dma_test [-r] [-m] [-b NO_OF_BYTES] [-f FILENAME]

		-r  dumps registers and exits
		-m  enables mock data source
		-b  burst mode, transfers the specified amount of bytes and exits
			(supports k and m suffixes for units of 1024 and 1024^2)
		-f  writes the transferred data to the given file, '-' for stdout.
			in burst mode, slow writes won't cause dropped frames

Successful continuous transfer looks like this:

	# ./dma_test
	pl_design_id=0xd5170001
	(after reset) s2mm_dmasr=0x10009
	(after s2mm_dmacr write) s2mm_dmasr=0x10008
	transferred=0x8a00000 counter=0x008a02a5 overflow=0x00000000

(The last line gets updated.) 'transferred' refers to transferred bytes. 'counter' is a number of frames counted in the logic. 'overflow' is the number of dropped frames due to FIFO overflow. The latter two correspond to the registers explained below.

For a quick look at the samples there's `tools/samples_plot.py`. Try the following on a remote computer (remote to Parallella):

	$ ssh PARALLELLA_HOSTNAME ./dma_test -f - -b 1m | ./tools/samples_plot.py

This assumes `dma_test` lies compiled in the home directory on Parallella.

## Software Side Interface

The software running on Zynq's ARM communicates with the programmable logic design via two sets of registers that are mapped into the physical address space of the CPU. The first set starts at `0x60000000`, and these are registers of an [off-the-shelf Xilinx DMA engine block](https://www.xilinx.com/support/documentation/ip_documentation/axi_dma/v7_1/pg021_axi_dma.pdf). The second set is one page above, at `0x60001000`, and these are our custom registers.

The software uses the registers of the DMA engine to arrange for the transfer of data into RAM. The second set of registers is then used to reset, enable and check the status of the logic that feeds the DMA engine with data. This logic ("our peripheral") receives the frames, filters them, buffers them in a FIFO and serializes them for transfer by the DMA engine. The data is fed to the DMA engine as an unannotated stream of bytes and their placement into memory is completely subject to the software's configuration of the engine.

The DMA engine has been synthesized with the Gather/Scatter Mode enabled and with the buffer size field of one descriptor being 24 bits wide. These parameters influence how the engine should be configured by software.

### The Custom Set of Registers

All registers are little endian.

| Offset | Byte Length | R/RW | Description                                |
| ------:| -----------:| ---- | ------------------------------------------ |
|  +0x00 |           4 | R    | ID of the PL design (0xd5170006) |
|  +0x04 |           4 | RW   | Control register |
|  +0x08 |           4 | R    | Number of frames received |
|  +0x0c |           4 | R    | Number of frames dropped due to FIFO overflow |
|  +0x10 |           4 | R    | Reserved for error flags |
|  +0x14 |           4 | R    | Timing of the last captured SYNC edge |

The bits of the control register have the following meaning:

| Bit Pos. (starts from 0) | Description                                |
| ------------:| ------------------------------------------ |
|            0 | Sample Input Enable |
|            1 | Pipeline Reset |
|            8 | Filter Bypass Enable |
|            9 | SYNC Edge Select (set for falling edge, unset for rising edge) |

The sample input enable influences whether new samples received on the ADC data lines enter the programmable logic pipeline.

The pipeline reset clears the FIFO contents, resets the FIR filter inner state and other pipeline state. The reset bit is deasserted automatically after a few cycles. The pipeline reset does not clear the contents of the control register.

### Recommended Programming Sequence

 1. reset the pipeline without enabling the sample input
 2. prepare the DMA engine for transfer so that it accepts data
 3. enable sample input

## Data Format

Samples from each of the eight 12-bit ADC channels are zero-padded from the LSB side to 2 bytes. These 2 bytes per channel are concatenated in little-endian into a 16-byte frame. The transferred data is a stream of these frames. After the reset of our peripheral, the transfer starts with a new frame and, if overflow occurs, whole frames are dropped. Therefore the software should never get desynchronized with respect to the byte-within-frame position.

## Synchronization

The logic design has an input port, called SYNC, dedicated to time synchronization. This input port is sampled as are the ADC data lines, on each edge of the ADC bit clock. Once an edge on the SYNC input with the selected-for polarity is detected, the position of the edge in the stream of frames is noted. This position includes both an absolute frame number -- that is, absolute number since pipeline reset -- and a fractional part signifying position of the edge within one ADC sample period. The positional information is only retained for the last detected edge and exposed in a register. This register is doubleword-long and is at +0x14.

Top three bytes of the register contain the absolute frame number modulo 2^24. The bottom byte of the register contains the fractional part. The fractional part is in units of 1/12 of undecimated sample period. The fractional part has values in the range [0, 48) for FIR enabled and [0, 12) for FIR disabled.

 * *FIR bypassed:* If FIR filter is bypassed, the absolute frame number points to the frame whose bits were being transferred when the edge was detected. The fractional part then specifies which particular bit has been sampled when the transition on the SYNC input was observed. Starting with fractional part equal to zero, in which case the last bit has been sampled, up to fractional part being eleven, in which case the first bit has been sampled when the transition was observed. See fig. 1 in the AFE5801 datasheet. If, for example, fractional part equals 3, the transition must have occured between sampling of the D7 and D8 bit. Note that ADC is in LSB-first-mode.

 * *FIR in the loop*: If FIR filter is enabled, the position as seen in register +0x14 accounts for decimation. The frame number is counted in decimated frames and the fractional part is in units of 1/48 of sample period of the decimated stream. Much in the same way as with FIR bypassed, an increasing fractional part shifts occurence of the edge in the opposite direction compared to increasing frame number.

## Bugs & TODO

 * Test peripherals. Do they work properly with our design? (SPI, I2C, something else also?) I have tried to copy from Adapteva's design in areas that are not relevant to our goal. Let's check that nothing is broken.

 * Look into the device tree incompatibility.
