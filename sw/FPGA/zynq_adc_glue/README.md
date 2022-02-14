# zynq_adc_glue

These are sources for the programmable logic layer between the ADCs and the Parallella's ARM processor. This is work-in-progress. The current state is that the layer can be used to grab unfiltered, undecimated samples from the ADCs.

## Build Prerequisities

 * [Xilinx Vivado](https://www.xilinx.com/products/design-tools/vivado.html) v2018.2
   The free WebPACK edition works. Note the version.

 * [Device Tree Compiler](https://git.kernel.org/pub/scm/utils/dtc/dtc.git/plain/Documentation/manual.txt)
   Converts device trees between their source text and blob format. Usually packaged as 'dtc'.

Make sure that the binaries 'vivado', 'mkbootimage' and 'dtc' are in 'PATH'.

## Why There's No Project File

Vivado projects are not well-suited for being managed by a version control system. For this reason, there's 'create_project.tcl' instead, which is a Vivado TCL script for recreating the project from scratch. Vivado generates these files upon request. The TCL script therefore contains the build settings and the block design in a diffable text format. More about this in Xilinx's [application note](https://www.xilinx.com/support/documentation/application_notes/xapp1165.pdf).

## make Commands

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
 * enables the FCLK3 clock used by the programmable logic when generating mock data

To use the custom device tree, it first needs to be compiled into a blob by the Device Tree Compiler. The makefile has a rule for that with the resultant blob being at 'build/devicetree.dtb'.

## Testing the Result

To boot the Parallella with the custom PL configuration, one replaces `parallella.bit.bin` and `devicetree.dtb` on the boot partition with the build results from `build/`.

To trigger a DMA transfer and, optionally, get the transferred data, use the `dma_test` tool.

	Usage: dma_test [-r] [-m] [-b NO_OF_BYTES] [-f FILENAME]

		-r  dumps registers and exits
		-m  enables mock data source
		-b  burst mode, transfers the specified amount of bytes and exits
			(supports k and m suffixes for units of 1024 and 1024^2)
		-f  writes the transferred data to the given file, '-' for stdout.
			in burst mode, slow writes won't cause dropped frames

By default, what's transferred are the samples received over the ADC data lines. With the `-m` flag, the program puts the logic layer into a mode in which the ADC samples are replaced with generated mock data. Successful continuous transfer looks like this:

	# ./dma_test
	pl_design_id=0xd5170001
	(after reset) s2mm_dmasr=0x10009
	(after s2mm_dmacr write) s2mm_dmasr=0x10008
	transferred=0x8a00000 counter=0x008a02a5 overflow=0x00000000

(The last line gets updated.) 'transferred' refers to transferred bytes. 'counter' is a number of frames counted in the logic. 'overflow' is the number of dropped frames due to FIFO overflow. The latter two correspond to the registers explained below.

For a quick look at the samples there's `tools/samples_plot.py`. Try the following on a remote computer (remote to Parallella):

	$ ssh PARALLELLA_HOSTNAME ./dma_test -f - -b 1m | ./tools/samples_plot.py

This assumes `dma_test` lies compiled in home directory on Parallella.

### Mock Data

Before you can test a transfer with mock data, you need to change the frequency of the Zynq's FCLK3 clock, which directly influences the speed with which the data is generated. One frame, i.e. 8 samples or 16 bytes, is generated each cycle. To set the frequency under Linux, use:

	# echo fclk3 > /sys/devices/soc0/amba/f8007000.devcfg/fclk_export
	# echo 1000000 > /sys/devices/soc0/amba/f8007000.devcfg/fclk/fclk3/set_rate

The first command makes the driver populate the '/sys' namespace with files by which the clock can be controlled. The second command sets the frequency to 1 MHz. You can experiment with other frequencies. The glue logic should be safe to use with upto 25 MHz -- value above that might cause frames to be dropped without it being registered (by incrementing 'overflow_counter').

## Software Side Interface

The software running on Zynq's ARM communicates with the programmable logic design via two sets of registers that are mapped into the physical address space of the CPU. The first set starts at `0x60000000`, and these are registers of an [off-the-shelf Xilinx DMA engine block](https://www.xilinx.com/support/documentation/ip_documentation/axi_dma/v7_1/pg021_axi_dma.pdf). The second set is one page above, at `0x60001000`, and these are our custom registers.

The software uses the registers of the DMA engine to arrange for the transfer of data into RAM. The second set of registers is then used to reset, enable and check the status of the logic that feeds the DMA engine with data. This logic ("our peripheral") receives the frames, buffers them in a FIFO, and serializes them for transfer by the DMA engine. The data is fed to the DMA engine as an unannotated stream of bytes and their placement into memory is completely subject to the software's configuration of the engine.

The DMA engine has been synthesized with the Gather/Scatter Mode enabled and with the buffer size field of one descriptor being 24 bits wide. These parameters influence how the engine should be configured by software.

### The Custom Set of Registers

| Offset | Byte Length | R/RW | Description                                |
| ------:| -----------:| ---- | ------------------------------------------ |
|   +0x0 |           4 | R    | ID of the PL design (0xd5170003) |
|   +0x4 |           4 | RW   | Control register |
|   +0x8 |           4 | R    | Number of frames received |
|   +0xc |           4 | R    | Number of frames dropped due to FIFO overflow |

The bits of the control register are active when set to one:

 * The 0'th bit enables the peripheral. This influences whether the FIFO gets filled with new data.
 * The 1'st bit resets the peripheral and is deasserted automatically. The reset impacts the FIFO and the frame serialization logic, both of which get cleared. This *doesn't* disable the peripheral.
 * The 2'nd bit selects data to come from the mock source instead of the ADC data lines.

### Recommended Programming Sequence

 1. reset our peripheral while keeping it disabled
 2. prepare the DMA engine for transfer so that it accepts data
 3. enable our peripheral

## Data Format

Samples from each of the eight 12-bit ADC channels are zero-padded from the LSB side to 2 bytes. These 2 bytes per channel are concatenated in little-endian into a 16-byte frame. The transferred data is a stream of these frames. After the reset of our peripheral, the transfer starts with a new frame and, if overflow occurs, whole frames are dropped. Therefore the software should never get desynchronized with respect to the byte-within-frame position.

The mock data are generated by eight counters, each for one of the channels. Their value is incremented per frame and they are, respectively from the first channel to the last, modulo 4096, 4093, 4091, 4079, 4073, 4057, 4051 and 4049. The generation of frames in this manner is independent of the enable/reset of our peripheral and of the ADC data/frame clocks.

## Bugs & TODO

 * Test peripherals. Do they work properly with our design? (SPI, I2C, something else also?) I have tried to copy from Adapteva's design in areas that are not relevant to our goal. Let's check that nothing is broken.

 * Look into the device tree incompatibility.
