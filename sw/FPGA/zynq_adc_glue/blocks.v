`timescale 1 ns / 1 ps

module adc_deser(d_p, d_n, fclk_p, fclk_n,
				 dclk_p, dclk_n, sync_p, sync_n,
				 data, data_sync, data_en);
	input  wire [7:0]	d_p, d_n;
	input  wire			sync_p, sync_n;
	input  wire			fclk_p, fclk_n;
	input  wire			dclk_p, dclk_n;

	output wire			data_en;
	output wire [11:0]	data_sync;
	output wire [127:0]	data;

	/* there's a catch! adc_d[3:0] and adc_dclk have their differential
	   pairs connected inversely. to create ibufds, we have to adhere
	   to the package polarity and not to the true polarity. this means
	   the lower lines are inverted on ibufds' output */

	wire dclk_neg_in, fclk;
	IBUFGDS #(.DIFF_TERM("TRUE"), .IOSTANDARD("LVDS_25"))
			dclk_buf (.I(dclk_n), .IB(dclk_p), .O(dclk_neg_in));
	IBUFDS #(.DIFF_TERM("TRUE"), .IOSTANDARD("LVDS_25"))
			fclk_buf (.I(fclk_p), .IB(fclk_n), .O(fclk));

	IBUFDS #(.DIFF_TERM("TRUE"), .IOSTANDARD("LVDS_25"))
			sync_buf (.I(sync_p), .IB(sync_n), .O(sync));

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

	/* from here on, we merge the inverted and non-inverted data lines
	   and correct that when assigning to 'data_narrow' */

	/* we have an IDDR in SAME_EDGE mode clocked by negative dclk
	   
	   upon a negative edge on dclk, d_q1 contains the value
	   captured on the corresponding d line at the previous dclk negedge
	   and d_q2 contains the value captured on the posedge just before that */

	wire [7:0] d_q1, d_q2;
	IDDR #(.DDR_CLK_EDGE("SAME_EDGE"), .SRTYPE("SYNC"))
		d_iddr[7:0] (.Q1(d_q1), .Q2(d_q2), .C(dclk_neg),
						 .CE(1'b1), .D({d_upper, d_lower_n}),
						 .R(1'b0), .S(1'b0));

	wire fclk_q;
	IDDR #(.DDR_CLK_EDGE("SAME_EDGE"), .SRTYPE("SYNC"))
		fclk_iddr (.Q1(), .Q2(fclk_q), .C(dclk_neg),
					   .CE(1'b1), .D(fclk), .R(1'b0), .S(1'b0));

	wire sync_q1, sync_q2;
	IDDR #(.DDR_CLK_EDGE("SAME_EDGE"), .SRTYPE("SYNC"))
		sync_iddr (.Q1(sync_q1), .Q2(sync_q2), .C(dclk_neg_iddr),
					   .CE(1'b1), .D(sync), .R(1'b0), .S(1'b0));

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

	(* ASYNC_REG = "TRUE" *) reg [11:0] latch_sync;
	reg [5:0] sync_odd;
	reg [5:0] sync_even;
	always @(posedge dclk_neg)
	begin
		sync_odd  <= {sync_q1, sync_odd[5:1]};
		sync_even <= {sync_q2, sync_even[5:1]};

		if (latch_en)
			latch_sync <= {
		  		sync_odd[5], sync_even[5], sync_odd[4], sync_even[4], sync_odd[3], sync_even[3],
				sync_odd[2], sync_even[2], sync_odd[1], sync_even[1], sync_odd[0], sync_even[0]
			};
	end

	assign data_en = fclk_q_delay;
	wire [95:0] data_narrow = {latch[95:48], ~latch[47:0]};

	generate
		for (i = 0; i < 8; i = i+1)
		begin
			assign data[i*16+4+:12] = data_narrow[i*12+:12];
			assign data[i*16+0+: 4] = 4'b0;
		end
	endgenerate

	assign data_sync = latch_sync;
endmodule


module overflow_barrier #
	(
		parameter integer TDATA_WIDTH = 16,
		parameter integer TUSER_WIDTH = 0
	)
	(
		input  wire clk,
		output wire overflow,

		output wire							S_AXIS_TREADY,
		input  wire							S_AXIS_TVALID,
		input  wire [TDATA_WIDTH-1:0]		S_AXIS_TDATA,
		input  wire [TUSER_WIDTH-1:0]		S_AXIS_TUSER,

		input  wire							M_AXIS_TREADY,
		output wire							M_AXIS_TVALID,
		output wire [TDATA_WIDTH-1:0]		M_AXIS_TDATA,
		output wire [TUSER_WIDTH-1:0]		M_AXIS_TUSER
	);

	assign overflow = S_AXIS_TVALID && !M_AXIS_TREADY;

	assign S_AXIS_TREADY = 1;
	assign M_AXIS_TVALID = S_AXIS_TVALID;
	assign M_AXIS_TDATA  = S_AXIS_TDATA;
	assign M_AXIS_TUSER  = S_AXIS_TUSER;
endmodule


module axis_coupler #
	(
		parameter integer TDATA_WIDTH = 0,
		parameter integer TUSER_WIDTH = 0
	)
	(
		input  wire in_clk,
		input  wire [TDATA_WIDTH-1:0]	in_data,
		input  wire [TUSER_WIDTH-1:0]	in_user,

		input  wire clk,
		input  wire enable,
		input  wire resetn,
		output wire overflow, // signalized wrt axis clk

		input  wire						M_AXIS_TREADY,
		output wire						M_AXIS_TVALID,
		output wire [TDATA_WIDTH-1:0]	M_AXIS_TDATA,
		output wire [TUSER_WIDTH-1:0]	M_AXIS_TUSER
	);

	reg [TDATA_WIDTH-1:0] tdata_latch;
	reg [TUSER_WIDTH-1:0] tuser_latch;
	reg data_en;

	assign M_AXIS_TVALID = data_en;
	assign M_AXIS_TDATA  = tdata_latch;
	assign M_AXIS_TUSER  = tuser_latch;
	assign overflow = M_AXIS_TVALID && ~M_AXIS_TREADY;

	(* ASYNC_REG = "TRUE" *) reg _inclk_stage0, _inclk_stage1;
	always @(posedge clk)
	begin
		_inclk_stage0 <= in_clk;
		_inclk_stage1 <= _inclk_stage0;
	end
	wire inclk_sync = _inclk_stage1;

	reg inclk_sync_delay;
	always @(posedge clk)
	begin
		if (!resetn) begin
			data_en <= 0;
		end
		else begin
			if (enable && inclk_sync && ~inclk_sync_delay) begin
				data_en <= 1;
				tdata_latch <= in_data;
				tuser_latch <= in_user;
			end
			else begin
				data_en <= 0;
			end
		end

		inclk_sync_delay <= inclk_sync;
	end
endmodule


module axis_bypass #
	(
		parameter integer TDATA_WIDTH = 0,
		parameter integer TUSER_WIDTH = 0
	)
	(
		input  wire bypass_en,

		input  wire						M_INNER_AXIS_TREADY,
		output wire						M_INNER_AXIS_TVALID,
		output wire [TDATA_WIDTH-1:0]	M_INNER_AXIS_TDATA,
		output wire [TUSER_WIDTH-1:0]	M_INNER_AXIS_TUSER,

		output wire						S_INNER_AXIS_TREADY,
		input  wire						S_INNER_AXIS_TVALID,
		input  wire [TDATA_WIDTH-1:0]	S_INNER_AXIS_TDATA,
		input  wire [TUSER_WIDTH-1:0]	S_INNER_AXIS_TUSER,

		output wire						S_AXIS_TREADY,
		input  wire						S_AXIS_TVALID,
		input  wire [TDATA_WIDTH-1:0]	S_AXIS_TDATA,
		input  wire [TUSER_WIDTH-1:0]	S_AXIS_TUSER,

		input  wire						M_AXIS_TREADY,
		output wire						M_AXIS_TVALID,
		output wire [TDATA_WIDTH-1:0]	M_AXIS_TDATA,
		output wire [TUSER_WIDTH-1:0]	M_AXIS_TUSER
	);

	assign M_INNER_AXIS_TVALID = S_AXIS_TVALID && ~bypass_en;
	assign M_INNER_AXIS_TDATA = S_AXIS_TDATA;
	assign M_INNER_AXIS_TUSER = S_AXIS_TUSER;

	assign M_AXIS_TDATA  = bypass_en ? S_AXIS_TDATA  : S_INNER_AXIS_TDATA;
	assign M_AXIS_TUSER  = bypass_en ? S_AXIS_TUSER  : S_INNER_AXIS_TUSER;
	assign M_AXIS_TVALID = bypass_en ? S_AXIS_TVALID : S_INNER_AXIS_TVALID;

	assign S_AXIS_TREADY = bypass_en ? M_AXIS_TREADY : M_INNER_AXIS_TREADY;
	assign S_INNER_AXIS_TREADY = S_AXIS_TREADY && ~bypass_en;
endmodule


module sync_edge_detector #
	(
		parameter integer IN_CAPTURES = 12,
		parameter integer COUNTER_WIDTH = 8,
		parameter integer TDATA_WIDTH = 128
	)
	(
		input  wire clk,
		input  wire resetn,

		input  wire polarity,

		output wire						S_AXIS_TREADY,
		input  wire						S_AXIS_TVALID,
		input  wire [TDATA_WIDTH-1:0]	S_AXIS_TDATA,
		input  wire [IN_CAPTURES-1:0]	S_AXIS_TUSER,

		input  wire						M_AXIS_TREADY,
		output wire						M_AXIS_TVALID,
		output wire [TDATA_WIDTH-1:0]	M_AXIS_TDATA,
		output wire [COUNTER_WIDTH-1:0]	M_AXIS_TUSER
	);

	reg have_data;
	reg [TDATA_WIDTH-1:0]	data;
	reg [COUNTER_WIDTH-1:0]	counter;

	assign M_AXIS_TVALID = have_data;
	assign M_AXIS_TDATA  = data;
	assign M_AXIS_TUSER  = counter;

	wire take_in =  S_AXIS_TVALID && S_AXIS_TREADY;
	wire take_out = M_AXIS_TVALID && M_AXIS_TREADY;

	assign S_AXIS_TREADY = !have_data || take_out;

	always @(posedge clk)
	begin
		if (!resetn) begin
			have_data <= 0;
		end
		else begin
			if (take_out || take_in)
				have_data <= take_in;

			if (take_in)
				data <= S_AXIS_TDATA;
		end
	end

	localparam integer counter_max = 2**COUNTER_WIDTH - 1;

	wire counting = (counter != counter_max);

	reg capture_carryover;
	wire [IN_CAPTURES-1:0] captures = polarity ? ~S_AXIS_TUSER : S_AXIS_TUSER;
	wire [IN_CAPTURES-1:0] edges = captures & ~{captures[IN_CAPTURES-2:0], capture_carryover};

	wire [3:0] edge_pos =
		edges[11] ? 4'b0000 :
		edges[10] ? 4'b0001 :
		edges[9]  ? 4'b0010 :
		edges[8]  ? 4'b0011 :
		edges[7]  ? 4'b0100 :
		edges[6]  ? 4'b0101 :
		edges[5]  ? 4'b0110 :
		edges[4]  ? 4'b0111 :
		edges[3]  ? 4'b1000 :
		edges[2]  ? 4'b1001 :
		edges[1]  ? 4'b1010 :
		edges[0]  ? 4'b1011 : 4'b1111;

	always @(posedge clk)
	begin
		if (!resetn) begin
			counter <= counter_max;
			capture_carryover <= 0;
		end
		else begin
			if (take_in) begin
				capture_carryover <= captures[IN_CAPTURES-1];

				if (counter > counter_max - IN_CAPTURES) begin
					if (|edges)
						counter <= edge_pos;
					else
						counter <= counter_max;
				end
				else begin
					counter <= counter + IN_CAPTURES;
				end
			end
		end
	end
endmodule


module sync_counter #
	(
		parameter SUBSAMPLE_C_WIDTH = 8,
		parameter SAMPLE_C_WIDTH = 24
	)
	(
		input  wire clk,
		input  wire resetn,

		input  wire count_en,
		input  wire [SUBSAMPLE_C_WIDTH-1:0] subsample,

		output reg  [SUBSAMPLE_C_WIDTH+SAMPLE_C_WIDTH-1:0] sync_reg
	);

	reg [SAMPLE_C_WIDTH-1:0] sample_count;
	reg [SUBSAMPLE_C_WIDTH-1:0] subsample_delay;

	always @(posedge clk)
	begin
		if (!resetn) begin
			sample_count <= 0;
			subsample_delay <= 0;
			sync_reg <= -1;
		end
		else begin
			if (count_en) begin
				subsample_delay <= subsample;
				sample_count <= sample_count + 1;

				if (subsample < subsample_delay)
					sync_reg <= {sample_count, subsample};
			end
		end
	end
endmodule


module axis_serializer #
	(
		parameter integer AXIS_WIDTH = 32,
		parameter integer NO_CHANNELS = 4
	)
	(
		input  wire clk,
		input  wire resetn,

		output wire									S_AXIS_TREADY,
		input  wire									S_AXIS_TVALID,
		input  wire									S_AXIS_TLAST,
		input  wire [AXIS_WIDTH*NO_CHANNELS-1:0]	S_AXIS_TDATA,

		input  wire							M_AXIS_TREADY,
		output wire							M_AXIS_TVALID,
		output wire							M_AXIS_TLAST,
		output wire [AXIS_WIDTH-1:0]		M_AXIS_TDATA
	);

	/* returns integer ceiling of log2(bit_depth) */
	function integer clogb2 (input integer bit_depth);
		begin
			for(clogb2=0; bit_depth>0; clogb2=clogb2+1)
				bit_depth = bit_depth >> 1;
		end
	endfunction

	wire reset = !resetn;

	wire [AXIS_WIDTH*NO_CHANNELS-1:0] in_data = S_AXIS_TDATA;
	wire in_valid							  = S_AXIS_TVALID;
	assign S_AXIS_TREADY                      = !state_have_data;

	reg									state_have_data;
	reg [clogb2(NO_CHANNELS)-1:0]		state_chan_out;
	reg [AXIS_WIDTH*NO_CHANNELS-1:0]	state_data;

	assign M_AXIS_TVALID = state_have_data;
	assign M_AXIS_TDATA  = state_data[state_chan_out*AXIS_WIDTH+:AXIS_WIDTH];
	assign M_AXIS_TLAST  = 1'b0;

	always @(posedge clk)
	begin
		if (reset)
		begin
			state_have_data <= 0;
			state_chan_out <= 0;
			state_data <= 0;
		end
		else begin
			if (state_have_data)
			begin
				if (M_AXIS_TREADY)
				begin
					if (state_chan_out == NO_CHANNELS-1)
					begin
						state_have_data <= 0;
						state_chan_out <= 0;
					end
					else begin
						state_chan_out <= state_chan_out + 1;
					end
				end
			end
			else begin
				if (in_valid)
				begin
					state_data <= in_data;
					state_have_data <= 1;
				end
			end
		end
	end
endmodule
