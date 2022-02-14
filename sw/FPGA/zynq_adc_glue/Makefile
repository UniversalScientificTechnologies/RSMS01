VIVADO = vivado -log vivado_prj/vivado.log -journal vivado_prj/vivado.jou

PRJFILE = vivado_prj/system.xpr
BITFILE = build/zynq_adc_glue.bit
BINFILE = build/parallella.bit.bin
DTBFILE = build/devicetree.dtb

HDLSOURCE = $(wildcard *.v)

define RUN_COMMANDS
reset_run synth_1
launch_runs impl_1 -jobs 8
wait_on_run impl_1
endef
export RUN_COMMANDS

define BITSTREAM_COMMANDS
open_checkpoint vivado_prj/system.runs/impl_1/top_block_wrapper_routed.dcp
write_bitstream -force -bin_file -file $(BITFILE)
endef
export BITSTREAM_COMMANDS

all: $(BINFILE) $(DTBFILE)

$(BINFILE): $(BITFILE)
	objcopy -I binary -O binary --reverse-bytes=4 build/zynq_adc_glue.bin build/parallella.bit.bin

$(BITFILE): $(PRJFILE) $(HDLSOURCE)
	echo "$$RUN_COMMANDS" | $(VIVADO) -mode tcl $(PRJFILE)
	echo "$$BITSTREAM_COMMANDS" | $(VIVADO) -mode tcl $(PRJFILE)

$(PRJFILE): # create_project.tcl
	$(VIVADO) -mode batch -source create_project.tcl

$(DTBFILE): devicetree.dts
	dtc -o $@ $<

clean:
	rm -r build/*
	rm -r vivado_prj/*

write_project_script:
	echo "write_project_tcl -target_proj_dir vivado_prj -force create_project.tcl" | $(VIVADO) -mode tcl $(PRJFILE)
	
	# strip the script of its noisy header
	mv create_project.tcl create_project_.tcl
	awk -v M=0 '/^([^#].+)?$$/||M!=0{print;M++;}' < create_project_.tcl > create_project.tcl
	rm create_project_.tcl
