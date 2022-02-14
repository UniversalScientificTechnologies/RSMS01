`timescale 1 ps / 1 ps

module top_block_wrapper
   (DDR_addr,
    DDR_ba,
    DDR_cas_n,
    DDR_ck_n,
    DDR_ck_p,
    DDR_cke,
    DDR_cs_n,
    DDR_dm,
    DDR_dq,
    DDR_dqs_n,
    DDR_dqs_p,
    DDR_odt,
    DDR_ras_n,
    DDR_reset_n,
    DDR_we_n,
    FIXED_IO_ddr_vrn,
    FIXED_IO_ddr_vrp,
    FIXED_IO_mio,
    FIXED_IO_ps_clk,
    FIXED_IO_ps_porb,
    FIXED_IO_ps_srstb,
    a_sync_n,
    a_sync_p,
    adc_d_n,
    adc_d_p,
    adc_dclk_n,
    adc_dclk_p,
    adc_fclk_n,
    adc_fclk_p,
    b_sync_n,
    b_sync_p,
    i2c_scl,
    i2c_sda);
  inout [14:0]DDR_addr;
  inout [2:0]DDR_ba;
  inout DDR_cas_n;
  inout DDR_ck_n;
  inout DDR_ck_p;
  inout DDR_cke;
  inout DDR_cs_n;
  inout [3:0]DDR_dm;
  inout [31:0]DDR_dq;
  inout [3:0]DDR_dqs_n;
  inout [3:0]DDR_dqs_p;
  inout DDR_odt;
  inout DDR_ras_n;
  inout DDR_reset_n;
  inout DDR_we_n;
  inout FIXED_IO_ddr_vrn;
  inout FIXED_IO_ddr_vrp;
  inout [53:0]FIXED_IO_mio;
  inout FIXED_IO_ps_clk;
  inout FIXED_IO_ps_porb;
  inout FIXED_IO_ps_srstb;
  input a_sync_n;
  input a_sync_p;
  input [7:0]adc_d_n;
  input [7:0]adc_d_p;
  input adc_dclk_n;
  input adc_dclk_p;
  input adc_fclk_n;
  input adc_fclk_p;
  input b_sync_n;
  input b_sync_p;
  inout i2c_scl;
  inout i2c_sda;

  wire [14:0]DDR_addr;
  wire [2:0]DDR_ba;
  wire DDR_cas_n;
  wire DDR_ck_n;
  wire DDR_ck_p;
  wire DDR_cke;
  wire DDR_cs_n;
  wire [3:0]DDR_dm;
  wire [31:0]DDR_dq;
  wire [3:0]DDR_dqs_n;
  wire [3:0]DDR_dqs_p;
  wire DDR_odt;
  wire DDR_ras_n;
  wire DDR_reset_n;
  wire DDR_we_n;
  wire FIXED_IO_ddr_vrn;
  wire FIXED_IO_ddr_vrp;
  wire [53:0]FIXED_IO_mio;
  wire FIXED_IO_ps_clk;
  wire FIXED_IO_ps_porb;
  wire FIXED_IO_ps_srstb;
  wire a_sync_n;
  wire a_sync_p;
  wire [7:0]adc_d_n;
  wire [7:0]adc_d_p;
  wire adc_dclk_n;
  wire adc_dclk_p;
  wire adc_fclk_n;
  wire adc_fclk_p;
  wire b_sync_n;
  wire b_sync_p;
  wire i2c_scl;
  wire i2c_sda;

  top_block top_block_i
       (.DDR_addr(DDR_addr),
        .DDR_ba(DDR_ba),
        .DDR_cas_n(DDR_cas_n),
        .DDR_ck_n(DDR_ck_n),
        .DDR_ck_p(DDR_ck_p),
        .DDR_cke(DDR_cke),
        .DDR_cs_n(DDR_cs_n),
        .DDR_dm(DDR_dm),
        .DDR_dq(DDR_dq),
        .DDR_dqs_n(DDR_dqs_n),
        .DDR_dqs_p(DDR_dqs_p),
        .DDR_odt(DDR_odt),
        .DDR_ras_n(DDR_ras_n),
        .DDR_reset_n(DDR_reset_n),
        .DDR_we_n(DDR_we_n),
        .FIXED_IO_ddr_vrn(FIXED_IO_ddr_vrn),
        .FIXED_IO_ddr_vrp(FIXED_IO_ddr_vrp),
        .FIXED_IO_mio(FIXED_IO_mio),
        .FIXED_IO_ps_clk(FIXED_IO_ps_clk),
        .FIXED_IO_ps_porb(FIXED_IO_ps_porb),
        .FIXED_IO_ps_srstb(FIXED_IO_ps_srstb),
        .a_sync_n(a_sync_n),
        .a_sync_p(a_sync_p),
        .adc_d_n(adc_d_n),
        .adc_d_p(adc_d_p),
        .adc_dclk_n(adc_dclk_n),
        .adc_dclk_p(adc_dclk_p),
        .adc_fclk_n(adc_fclk_n),
        .adc_fclk_p(adc_fclk_p),
        .b_sync_n(b_sync_n),
        .b_sync_p(b_sync_p),
        .i2c_scl(i2c_scl),
        .i2c_sda(i2c_sda));
endmodule
