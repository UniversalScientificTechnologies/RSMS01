`timescale 1 ns / 1 ps

`define DATA_WIDTH 32
`define ADDR_WIDTH 6

module registers
	(
		input wire S_AXI_ACLK,
		input wire S_AXI_ARESETN,

		input  wire [`ADDR_WIDTH-1:0]	S_AXI_AWADDR,
		input  wire						S_AXI_AWVALID,
		output wire						S_AXI_AWREADY,

		input  wire [`DATA_WIDTH-1:0]	S_AXI_WDATA,
		input  wire [`DATA_WIDTH/8-1:0]	S_AXI_WSTRB,
		input  wire						S_AXI_WVALID,
		output wire						S_AXI_WREADY,

		output wire [1:0]				S_AXI_BRESP,
		output wire						S_AXI_BVALID,
		input  wire						S_AXI_BREADY,

		input  wire [`ADDR_WIDTH-1:0]	S_AXI_ARADDR,
		input  wire						S_AXI_ARVALID,
		output wire						S_AXI_ARREADY,

		output wire [`DATA_WIDTH-1:0]	S_AXI_RDATA,
		output wire [1:0]				S_AXI_RRESP,
		output wire						S_AXI_RVALID,
		input  wire						S_AXI_RREADY,

		output reg	[31:0]	reg_control,
		input  wire	[31:0]	reg_frame_counter,
		input  wire [31:0]	reg_overflow_counter,
		input  wire [31:0]  reg_diagnostics,
		input  wire	[31:0]	reg_control_clear_mask
	);

	wire clk   = S_AXI_ACLK;
	wire reset = !S_AXI_ARESETN;

	reg write_en;
	wire [`DATA_WIDTH-1:0] write_data = S_AXI_WDATA;
	wire [`ADDR_WIDTH-1:0] write_addr = S_AXI_AWADDR;
	assign S_AXI_AWREADY = write_en;
	assign S_AXI_WREADY  = write_en;

	reg [1:0] axi_bresp;
	reg	      axi_bvalid;
	assign S_AXI_BRESP  = axi_bresp;
	assign S_AXI_BVALID = axi_bvalid;

	always @(posedge clk)
	begin
		if (reset)
			write_en <= 0;
		else
			write_en <= (~write_en) && S_AXI_AWVALID
						&& S_AXI_WVALID && (~axi_bvalid);
	end

	always @(posedge clk)
	begin
		if (reset)
		begin
			axi_bresp <= 2'b0;
			axi_bvalid <= 0;
		end
		else begin
			if (write_en)
			begin
				axi_bresp <= 2'b0; // OKAY
				axi_bvalid <= 1;
			end
			else begin
				if (axi_bvalid && S_AXI_BREADY)
					axi_bvalid <= 0;
			end
		end
	end

	reg read_en;
	wire [`ADDR_WIDTH-1:0] read_addr;
	reg  [`DATA_WIDTH-1:0] read_data;
	assign S_AXI_ARREADY = read_en;
	assign read_addr     = S_AXI_ARADDR;
	assign S_AXI_RDATA   = read_data;

	reg [1:0] axi_rresp;
	reg       axi_rvalid;
	assign S_AXI_RRESP  = axi_rresp;
	assign S_AXI_RVALID = axi_rvalid;

	always @(posedge clk)
	begin
		if (reset)
			read_en <= 0;
		else
			read_en <= (~read_en) && S_AXI_ARVALID && (~axi_rvalid);
	end

	always @(posedge clk)
	begin
		if (reset)
		begin
			axi_rresp <= 2'b0;
			axi_rvalid <= 0;
		end
		else begin
			if (read_en)
			begin
				axi_rresp <= 2'b0; // OKAY
				axi_rvalid <= 1;
			end
			else begin
				if (axi_rvalid && S_AXI_RREADY)
					axi_rvalid <= 0;
			end
		end
	end

	// The Registers Themselves

	always @(posedge clk)
	begin
		if (reset)
		begin
			reg_control <= 0;
		end
		else begin
			if (write_en)
			begin
				case (write_addr)
					4:			reg_control <= write_data;
					default:	reg_control <= reg_control;
				endcase
			end
			else begin
				// TODO: merge with write_addr!=4
				reg_control <= reg_control & ~reg_control_clear_mask;
			end
		end
	end

	always @(posedge clk)
	begin
		case (read_addr)
			0:			read_data <= {16'hd517, 16'h0003};
			4:			read_data <= reg_control;
			8:			read_data <= reg_frame_counter;
			12:			read_data <= reg_overflow_counter;
			16:			read_data <= reg_diagnostics;
			20:			read_data <= 32'h00000010;
			default:	read_data <= 32'h00000000;
		endcase
	end
endmodule
