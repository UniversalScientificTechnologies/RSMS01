create_clock -period 8.333 -name adc_clk -waveform {0.000 4.167} [get_nets {adc_dclk_n adc_dclk_p}]
#set_clock_groups -asynchronous -group [get_clocks {clk_fpga_0 clk_fpga_3}] -group [get_clocks adc_clk]

set_false_path -from [get_clocks adc_clk] -to [get_clocks clk_fpga_0]
set_bus_skew -from [get_clocks adc_clk] -to [get_clocks clk_fpga_0] 25.000
set_max_delay -datapath_only -from [get_clocks adc_clk] -to [get_clocks clk_fpga_0] 80.000

#set_false_path -from [get_clocks clk_fpga_3] -to [get_clocks clk_fpga_0]
#set_bus_skew -from [get_clocks clk_fpga_3] -to [get_clocks clk_fpga_0] 25.000
#set_max_delay -datapath_only -from [get_clocks clk_fpga_3] -to [get_clocks clk_fpga_0] 80.000

#set_false_path -from [get_pins {top_block_i/rst_ps7_0_100M/U0/ACTIVE_LOW_PR_OUT_DFF[0].FDRE_PER_N/C}] -to [get_pins top_block_i/synth_source_v1_0_M0_0/inst/mock_reset_ff/*/CLR]

