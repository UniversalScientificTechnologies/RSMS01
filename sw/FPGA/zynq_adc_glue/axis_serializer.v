`timescale 1 ns / 1 ps

module axis_serializer #
	(
		parameter integer AXIS_WIDTH = 32,
		parameter integer NO_CHANNELS = 4
	)
	(
		input  wire clk,
		input  wire reset,

		output wire									in_ready,
		input  wire									in_valid,
		input  wire [AXIS_WIDTH*NO_CHANNELS-1:0]	in_data,

		input  wire							M_AXIS_TREADY,
		output wire							M_AXIS_TVALID,
		output wire							M_AXIS_TLAST,
		output wire [AXIS_WIDTH-1:0]		M_AXIS_TDATA,
		output wire [(AXIS_WIDTH/8)-1:0]	M_AXIS_TSTRB
	);

	/* returns integer ceiling of log2(bit_depth) */
	function integer clogb2 (input integer bit_depth);
		begin
			for(clogb2=0; bit_depth>0; clogb2=clogb2+1)
				bit_depth = bit_depth >> 1;
		end
	endfunction

	reg									state_have_data;
	reg [clogb2(NO_CHANNELS)-1:0]		state_chan_out;
	reg [AXIS_WIDTH*NO_CHANNELS-1:0]	state_data;

	assign in_ready = !state_have_data;

	assign M_AXIS_TVALID = state_have_data;
	assign M_AXIS_TDATA  = state_data[state_chan_out*AXIS_WIDTH+:AXIS_WIDTH];
	assign M_AXIS_TSTRB  = {(AXIS_WIDTH/8){1'b1}};
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
