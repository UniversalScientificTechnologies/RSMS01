`timescale 1ns / 1ps

import axi_vip_pkg::*;
import testbench_axi_vip_0_0_pkg::*;
import testbench_axi_vip_1_0_pkg::*;
import testbench_axi_vip_2_0_pkg::*;

module outer_tb();
    bit clk;
    bit resetn;
    always #5 clk <= ~clk;
    
    initial begin
        resetn <= 0;
        repeat(16) @(posedge clk);
        resetn <= 1;
    end
    
    bit adc_dclk_p;
    wire adc_dclk_n = ~adc_dclk_p;
    bit adc_fclk_p;
    wire adc_fclk_n = ~adc_fclk_p;

    bit sync_p;
    wire sync_n = ~sync_p;

    testbench bd(
        .adc_fclk_p(adc_fclk_p),
        .adc_fclk_n(adc_fclk_n),
        .adc_dclk_p(adc_dclk_p),
        .adc_dclk_n(adc_dclk_n),
        .clk(clk),
        .resetn(resetn),

        .adc_d_p(8'h00),
        .adc_d_n(8'hff),
        
        .sync_p(sync_p),
        .sync_n(sync_n)
    );

    initial begin
        adc_dclk_p <= 0;

        forever begin
            #4 adc_fclk_p <= ~adc_fclk_p;
            #4 adc_dclk_p <= ~adc_dclk_p;
            repeat(5) #8 adc_dclk_p <= ~adc_dclk_p;
        end
    end
    
    initial begin
        forever begin
            repeat(257) @(posedge adc_dclk_p);
            sync_p <= 1;
            repeat(3) @(posedge adc_dclk_p);
            sync_p <= 0;
        end
    end
    
    bit [31:0] rd_data;
    xil_axi_resp_t rd_resp;
    xil_axi_resp_t wr_resp;
    
    testbench_axi_vip_0_0_slv_mem_t dma_s_ag;
    
    testbench_axi_vip_1_0_mst_t conf_m_ag;
    initial begin
        $display("main");
        
        dma_s_ag = new("DMA S", bd.testbench_i.axi_vip_0.inst.IF);
        dma_s_ag.set_agent_tag("DMA S");
        dma_s_ag.start_slave();

        conf_m_ag = new("Conf M", bd.testbench_i.axi_vip_1.inst.IF);
        conf_m_ag.set_agent_tag("Conf M");
        conf_m_ag.set_verbosity(XIL_AXI_VERBOSITY_NONE);
        conf_m_ag.start_master();
        
        
        @(posedge resetn);
        repeat(4) @(posedge clk);

        conf_m_ag.AXI4LITE_READ_BURST(16'h1000, 0, rd_data, rd_resp);
        $display("Periph: Read ID: 0x%08X", rd_data);
        $display("Periph: Resetting");
        conf_m_ag.AXI4LITE_WRITE_BURST(16'h1004, 0, 2, wr_resp);

        write_dma_desc(32'hf0000000, 32'hf0001000, 0, 16'h1000);

        $display("DMA: Writing CURDESC");
        conf_m_ag.AXI4LITE_WRITE_BURST(8'h38, 0, 32'hf0000000, wr_resp);
        $display("DMA: Enabling");
        conf_m_ag.AXI4LITE_WRITE_BURST(8'h30, 0, 1, wr_resp);
        $display("DMA: Writing TAILDESC");
        conf_m_ag.AXI4LITE_WRITE_BURST(8'h40, 0, 32'hf0000000, wr_resp);

        repeat(16) @(posedge clk);
        $display("Periph: Enabling");
        conf_m_ag.AXI4LITE_WRITE_BURST(16'h1004, 0, 1, wr_resp);
        //periph_m_ag.AXI4LITE_WRITE_BURST(16'h0104, 0, 1, wr_resp);


        repeat(128) @(posedge clk);
    end
    
    task mem_write(
        input xil_axi_ulong addr,
        input xil_axi_ulong data
    );
        dma_s_ag.mem_model.backdoor_memory_write(addr, data, 4'b1111);
    endtask :mem_write
    
    task write_dma_desc(
        input xil_axi_ulong addr,
        input xil_axi_ulong next_desc_addr,
        input xil_axi_ulong buffer_addr,
        input xil_axi_ulong buffer_len
    );
        // next desc
        mem_write(addr,         next_desc_addr);
        mem_write(addr + 8'h04, 0);
        // buffer
        mem_write(addr + 8'h08, buffer_addr);
        mem_write(addr + 8'h0c, 0);
        // reserved
        mem_write(addr + 8'h10, buffer_len);
        mem_write(addr + 8'h14, buffer_len);
        // control
        mem_write(addr + 8'h18, buffer_len);
        // status
        mem_write(addr + 8'h1c, 0);
    endtask :write_dma_desc
endmodule
