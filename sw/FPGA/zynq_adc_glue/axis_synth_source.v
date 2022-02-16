`timescale 1 ns / 1 ps

`define AXIS_WIDTH     32
`define AXI_DATA_WIDTH 32
`define AXI_ADDR_WIDTH 6

module async_reset_ff
	(
		input  wire	clk, async_resetn,
		output wire	resetn
	);

	(* ASYNC_REG = "TRUE" *) reg s0, s1;
	assign resetn = s1;
	always @(posedge clk or negedge async_resetn)
		if (!async_resetn) {s1, s0} <= 2'b0;
		else               {s1, s0} <= {s0, 1'b1};
endmodule

module counter_12bit #
	(
		parameter integer MODULO = 4096
	)
	(
		input wire clk, resetn,
		output reg [11:0] value
	);

	always @(posedge clk)
	begin
		if (!resetn)
		begin
			value <= 0;
		end
		else begin
			if (value < MODULO-1)	value <= value + 1;
			else					value <= 0;
		end
	end
endmodule

module adc_shift_reg(in, dclk, out);
	input in, dclk;
	output [11:0] out;

	reg [5:0] odd;
	reg [5:0] even;

	always @(posedge dclk) odd  <= {odd[4:0],  in};
	always @(negedge dclk) even <= {even[4:0], in};

	assign out = {odd[5], even[5], odd[4], even[4], odd[3], even[3],
				  odd[2], even[2], odd[1], even[1], odd[0], even[0]};
endmodule

module clock_detector(probe, clk, present);
	input probe, clk;
	output reg present;

	reg [6:0] clk_div;
	always @(posedge clk) clk_div <= clk_div + 1;
	wire clk_div128 = clk_div[6];

	(* ASYNC_REG = "TRUE" *) reg p_s0, p_s1;
	always @(posedge clk)
		{p_s1, present} <= {p_s0, p_s1};

	(* ASYNC_REG = "TRUE" *) reg test_ff;
	always @(posedge clk_div128)
		p_s0 <= test_ff;

	always @(posedge clk_div128 or negedge probe)
		if (clk_div128) test_ff <= 1'b0;
		else			test_ff <= 1'b1;
endmodule

module clk_div_1024(in, out);
	input in;
	output out;

	reg [9:0] div;
	always @(posedge in) div <= div + 1;

	assign out = div[9];
endmodule

module clock_counter(clk, clk_known, reset, result);
	input clk, clk_known, reset;
	output reg [15:0] result;

	reg [15:0] counter;
	reg clk_known_delay;

	always @(posedge clk or posedge reset)
	begin
		if (reset) begin
			counter <= 0;
			result <= 0;
		end
		else begin
			if (clk_known && ~clk_known_delay) begin
				result <= counter;
				counter <= 0;
			end
			else begin
				counter <= counter + 1;
			end
		end

		clk_known_delay <= clk_known;
	end
endmodule

module adc_deser(d_p, d_n, fclk_p, fclk_n,
				 dclk_p, dclk_n, data, data_en);
	input  wire [7:0]	d_p, d_n;
	input  wire			fclk_p, fclk_n;
	input  wire			dclk_p, dclk_n;

	output wire			data_en;
	output wire [95:0]	data;

	/* there's a catch! adc_d[3:0] and adc_dclk have their differential
	   pairs connected inversely. we have to adhere to the pin polarity,
	   not the true polarity, when creating ibufds, and so the lower data
	   lines are inverted on ibufds' output */

	wire dclk_neg_in, fclk;
	IBUFGDS #(.DIFF_TERM("TRUE"), .IOSTANDARD("LVDS_25"))
			dclk_buf (.I(dclk_n), .IB(dclk_p), .O(dclk_neg_in));
	IBUFDS #(.DIFF_TERM("TRUE"), .IOSTANDARD("LVDS_25"))
			fclk_buf (.I(fclk_p), .IB(fclk_n), .O(fclk));

	wire [3:0] d_lower_n;
	wire [3:0] d_upper;
	IBUFDS #(.DIFF_TERM("TRUE"), .IOSTANDARD("LVDS_25"))
			d_buf_lower[3:0] (.I(d_n[3:0]), .IB(d_p[3:0]), .O(d_lower_n));
	IBUFDS #(.DIFF_TERM("TRUE"), .IOSTANDARD("LVDS_25"))
			d_buf_upper[3:0] (.I(d_p[7:4]), .IB(d_n[7:4]), .O(d_upper));

	wire dclk_neg_iddr;
	BUFIO dclk_bufio(.I(dclk_neg_in), .O(dclk_neg_iddr));
	wire dclk_neg;
	BUFR dclk_bufr(.I(dclk_neg_in), .O(dclk_neg));

	/* from here, we merge the inverted and non-inverted data lines
	   and correct that when assigning to the 'data' output */

	/* we have a IDDR in SAME_EDGE mode clocked by negative dclk
	   
	   upon a negative edge on dclk, d_q1 contains the value
	   captured on the corresponding d line at the previous dclk negedge
	   and d_q2 contains the value captured on the posedge just before that */

	wire [7:0] d_q1, d_q2;
	IDDR #(.DDR_CLK_EDGE("SAME_EDGE"), .SRTYPE("SYNC"))
		d_iddr[7:0] (.Q1(d_q1), .Q2(d_q2), .C(dclk_neg_iddr),
						 .CE(1'b1), .D({d_upper, d_lower_n}),
						 .R(1'b0), .S(1'b0));

	wire fclk_q;
	IDDR #(.DDR_CLK_EDGE("SAME_EDGE"), .SRTYPE("SYNC"))
		fclk_iddr (.Q1(), .Q2(fclk_q), .C(dclk_neg_iddr),
					   .CE(1'b1), .D(fclk), .R(1'b0), .S(1'b0));

	reg fclk_q_delay;
	always @(posedge dclk_neg) fclk_q_delay <= fclk_q;
	wire latch_en = fclk_q && !fclk_q_delay;

	(* ASYNC_REG = "TRUE" *) reg [95:0] latch;

	genvar i;
	generate
		for (i = 0; i < 8; i = i+1)
		begin
			reg [5:0] odd;
			reg [5:0] even;

			always @(posedge dclk_neg)
			begin
				odd  <= {d_q1[i], odd[5:1]};
				even <= {d_q2[i], even[5:1]};

				if (latch_en)
					latch[12*i+11:12*i] <= {
				  		odd[5], even[5], odd[4], even[4], odd[3], even[3],
						odd[2], even[2], odd[1], even[1], odd[0], even[0]
					};
			end
		end
	endgenerate

	assign data_en = fclk_q_delay;
	assign data = {latch[95:48], ~latch[47:0]};
endmodule

module synth_source_v1_0_M00_AXIS #
	(
	)
	(
		input  wire clk,
		input  wire resetn,

		/* Master AXI4-Stream Interface */
		//input  wire M_AXIS_ACLK,
		//input  wire M_AXIS_ARESETN,

		input  wire							M_AXIS_TREADY,
		output wire							M_AXIS_TVALID,
		output wire							M_AXIS_TLAST,
		output wire [`AXIS_WIDTH-1:0]		M_AXIS_TDATA,
		output wire [(`AXIS_WIDTH/8)-1:0]	M_AXIS_TSTRB,
		
		/* Slave AXI-Lite Interface */
		//input wire S_AXI_ACLK,
		//input wire S_AXI_ARESETN,

		input  wire [`AXI_ADDR_WIDTH-1:0]	S_AXI_AWADDR,
		input  wire							S_AXI_AWVALID,
		output wire							S_AXI_AWREADY,

		input  wire [`AXI_DATA_WIDTH-1:0]	S_AXI_WDATA,
		input  wire [`AXI_DATA_WIDTH/8-1:0]	S_AXI_WSTRB,
		input  wire							S_AXI_WVALID,
		output wire							S_AXI_WREADY,

		output wire [1:0]					S_AXI_BRESP,
		output wire							S_AXI_BVALID,
		input  wire							S_AXI_BREADY,

		input  wire [`AXI_ADDR_WIDTH-1:0]	S_AXI_ARADDR,
		input  wire							S_AXI_ARVALID,
		output wire							S_AXI_ARREADY,

		output wire [`AXI_DATA_WIDTH-1:0]	S_AXI_RDATA,
		output wire [1:0]					S_AXI_RRESP,
		output wire							S_AXI_RVALID,
		input  wire							S_AXI_RREADY,

		input  wire [7:0]	adc_d_p, adc_d_n,
		input  wire			adc_fclk_p, adc_fclk_n,
		input  wire			adc_dclk_p, adc_dclk_n,

		input  wire mock_clk
	);

	wire S_AXI_ACLK    = clk;
	wire S_AXI_ARESETN = resetn;

	wire [31:0]	reg_control;
	reg  [31:0] frame_counter;
	reg  [31:0] overflow_counter;
	wire [31:0] reg_diag;

	wire soft_reset     = !resetn || reg_control[1];
	wire soft_enable    = reg_control[0] && !soft_reset;
	wire mock_data_mode = reg_control[2];

	reg [3:0] _s;
	always @(posedge clk)
	begin
		_s <= {_s[2], _s[1], _s[0], soft_reset};
	end

	wire [31:0] reg_control_clear_mask = {30'b0, _s[3], 1'b0};
	registers r(.S_AXI_ACLK(S_AXI_ACLK), .S_AXI_ARESETN(S_AXI_ARESETN),
				.S_AXI_AWADDR(S_AXI_AWADDR), .S_AXI_AWVALID(S_AXI_AWVALID),
				.S_AXI_AWREADY(S_AXI_AWREADY), .S_AXI_WDATA(S_AXI_WDATA),
				.S_AXI_WSTRB(S_AXI_WSTRB), .S_AXI_WVALID(S_AXI_WVALID),
				.S_AXI_WREADY(S_AXI_WREADY), .S_AXI_BRESP(S_AXI_BRESP),
				.S_AXI_BVALID(S_AXI_BVALID), .S_AXI_BREADY(S_AXI_BREADY),
				.S_AXI_ARADDR(S_AXI_ARADDR), .S_AXI_ARVALID(S_AXI_ARVALID),
				.S_AXI_ARREADY(S_AXI_ARREADY), .S_AXI_RDATA(S_AXI_RDATA),
				.S_AXI_RRESP(S_AXI_RRESP), .S_AXI_RVALID(S_AXI_RVALID),
				.S_AXI_RREADY(S_AXI_RREADY),
				.reg_control(reg_control), .reg_control_clear_mask(reg_control_clear_mask),
				.reg_frame_counter(frame_counter), .reg_overflow_counter(overflow_counter),
				.reg_diagnostics(reg_diag));

	wire [127:0] f2s_data_wide;
	wire [95:0]  f2s_data;
	wire		 f2s_valid;
	wire 		 f2s_ready;

	genvar i;
	generate
		for (i = 0; i < 4; i = i+1)
		begin
			assign f2s_data_wide[i*32+20+:12] = f2s_data[i*24+12+:12];
			assign f2s_data_wide[i*32+16+: 4] = 4'b0;
			assign f2s_data_wide[i*32+ 4+:12] = f2s_data[i*24+ 0+:12];
			assign f2s_data_wide[i*32+ 0+: 4] = 4'b0;
		end
	endgenerate

	axis_serializer #(.AXIS_WIDTH(`AXIS_WIDTH), .NO_CHANNELS(4)) 
		serializer(.clk(clk), .reset(soft_reset), .M_AXIS_TVALID(M_AXIS_TVALID),
					.M_AXIS_TDATA(M_AXIS_TDATA), .M_AXIS_TSTRB(M_AXIS_TSTRB),
					.M_AXIS_TLAST(M_AXIS_TLAST), .M_AXIS_TREADY(M_AXIS_TREADY),
					.in_ready(f2s_ready), .in_data(f2s_data_wide),
					.in_valid(f2s_valid));

	wire f2s_empty;
	assign f2s_valid = !f2s_empty;

	wire fifo_full;
	wire fifo_wr_en;
	wire [95:0] fifo_in;

	fifo_sync_x64_96b fifo_sync(.empty(f2s_empty), .rd_en(f2s_ready), .dout(f2s_data),
								.full(fifo_full), .wr_en(fifo_wr_en), .din(fifo_in),
								.clk(clk), .rst(soft_reset));
	
	always @(posedge clk)
	begin
		if (soft_reset) begin
			frame_counter <= 0;
			overflow_counter <= 0;
		end
		else begin
			if (fifo_wr_en)
			begin
				frame_counter <= frame_counter + 1;

				// TODO: check fifo_full has the right semantics for this to be accurate
				if (fifo_full)
					overflow_counter <= overflow_counter + 1;
			end
		end
	end

	wire [95:0] payload_data_async;
	wire		payload_en_async;

	assign fifo_in = payload_data_async;

	/* the following synchronization logic is safe as long as the positive pulse
	   on payload_en_async is more than 3 clk-cycles wide and the pulse occurs
	   after 3 clk-cycles of payload_en_async being low. the data on payload_data_async
	   is sampled somewhen during the 3 clk-cycles after the enable is asserted. */

	(* ASYNC_REG = "TRUE" *) reg _pl_en_stage0, _pl_en_stage1;
	always @(posedge clk)
	begin
		_pl_en_stage0 <= payload_en_async;
		_pl_en_stage1 <= _pl_en_stage0;
	end
	wire payload_en_sync = _pl_en_stage1;

	reg payload_en_sync_delay, payload_fifo_wr_en;
	assign fifo_wr_en = payload_fifo_wr_en;
	always @(posedge clk)
	begin
		if (!resetn) begin
			payload_fifo_wr_en <= 0;
		end
		else begin
			if (soft_enable && payload_en_sync && ~payload_en_sync_delay)
				payload_fifo_wr_en <= 1;
			else
				payload_fifo_wr_en <= 0;
		end

		payload_en_sync_delay <= payload_en_sync;
	end

	/* below this line is the logic that ought to supply payload_data_async with data
	   and signalize its presence using payload_en_async

	   we have two implementations: a mock data source and the ADC interface
	*/

	wire mock_resetn;
	async_reset_ff mock_reset_ff(.clk(mock_clk), .async_resetn(resetn),
								.resetn(mock_resetn));

	wire [95:0] mock_data;
	wire mock_data_en = !mock_clk;

	counter_12bit #(.MODULO(4096)) _c1(.clk(mock_clk), .resetn(mock_resetn), .value(mock_data[ 0+:12]));
	counter_12bit #(.MODULO(4093)) _c2(.clk(mock_clk), .resetn(mock_resetn), .value(mock_data[12+:12]));
	counter_12bit #(.MODULO(4091)) _c3(.clk(mock_clk), .resetn(mock_resetn), .value(mock_data[24+:12]));
	counter_12bit #(.MODULO(4079)) _c4(.clk(mock_clk), .resetn(mock_resetn), .value(mock_data[36+:12]));
	counter_12bit #(.MODULO(4073)) _c5(.clk(mock_clk), .resetn(mock_resetn), .value(mock_data[48+:12]));
	counter_12bit #(.MODULO(4057)) _c6(.clk(mock_clk), .resetn(mock_resetn), .value(mock_data[60+:12]));
	counter_12bit #(.MODULO(4051)) _c7(.clk(mock_clk), .resetn(mock_resetn), .value(mock_data[72+:12]));
	counter_12bit #(.MODULO(4049)) _c8(.clk(mock_clk), .resetn(mock_resetn), .value(mock_data[84+:12]));

	/* ADC */
	wire adc_data_en;
	wire [95:0] adc_data;
	adc_deser adc_deser(.fclk_p(adc_fclk_p), .fclk_n(adc_fclk_n),
						.dclk_p(adc_dclk_p), .dclk_n(adc_dclk_n),
						.d_p(adc_d_p), .d_n(adc_d_n),
						.data(adc_data), .data_en(adc_data_en));


	assign payload_en_async   = mock_data_mode? mock_data_en : adc_data_en;
	assign payload_data_async = mock_data_mode? mock_data    : adc_data;

	/* Clock Detectors */
	/*
	(* ASYNC_REG = "TRUE" *) reg fclk_samples_dclk, dclk_samples_fclk;
	always @(posedge adc_fclk) fclk_samples_dclk <= adc_dclk;
	always @(posedge adc_dclk) dclk_samples_fclk <= adc_fclk;

	clock_detector clk_det_fclk(adc_fclk, clk, reg_diag[0]),
				   clk_det_dclk(adc_dclk, clk, reg_diag[1]),
				   clk_det_fclk_samp(fclk_samples_dclk, clk, reg_diag[2]),
				   clk_det_dclk_samp(dclk_samples_fclk, clk, reg_diag[3]),
				   clk_det_mock_clk(mock_clk, clk, reg_diag[16]),
				   clk_det_payload_en_async(payload_en_async, clk, reg_diag[17]);
	*/
/*
	wire clk_div1024;
	clk_div_1024 clk_div_1024_inst(clk, clk_div1024);

	clock_counter fclk_counter(adc_fclk, clk_div1024, soft_reset, reg_diag[15:0]),
				  dclk_counter(adc_dclk, clk_div1024, soft_reset, reg_diag[31:16]);
*/
endmodule
