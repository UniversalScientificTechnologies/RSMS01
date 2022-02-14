`timescale 1 ns / 1 ps

`define DATA_WIDTH 32
`define ADDR_WIDTH 6

module our_periph
	(
		input  wire clk,
		input  wire resetn,

		input  wire [`ADDR_WIDTH-1:0]	S_AXI_AWADDR,
		input  wire							S_AXI_AWVALID,
		output wire							S_AXI_AWREADY,

		input  wire [`DATA_WIDTH-1:0]	S_AXI_WDATA,
		input  wire [`DATA_WIDTH/8-1:0]	S_AXI_WSTRB,
		input  wire							S_AXI_WVALID,
		output wire							S_AXI_WREADY,

		output wire [1:0]					S_AXI_BRESP,
		output wire							S_AXI_BVALID,
		input  wire							S_AXI_BREADY,

		input  wire [`ADDR_WIDTH-1:0]	S_AXI_ARADDR,
		input  wire							S_AXI_ARVALID,
		output wire							S_AXI_ARREADY,

		output wire [`DATA_WIDTH-1:0]	S_AXI_RDATA,
		output wire [1:0]					S_AXI_RRESP,
		output wire							S_AXI_RVALID,
		input  wire							S_AXI_RREADY,

		output wire [31:0] control,
		input  wire [31:0] frame_counter,
		input  wire [31:0] overflow_counter,
		input  wire [31:0] err_conds,
		input  wire [31:0] sync_reg,

		output wire pl_enable,
		output wire pl_resetn
	);

	reg  [31:0] reg_control;
	wire [31:0] reg_control_clear_mask;
	reg  [31:0] reg_err_conds;
	wire [31:0] reg_err_conds_set_mask;

	wire soft_reset     = !resetn || reg_control[1];
	wire soft_enable    = reg_control[0] && !soft_reset;
	wire mock_data_mode = reg_control[2];

    assign pl_resetn = resetn && !reg_control[1];
    assign pl_enable = pl_resetn && reg_control[0];
    assign control = reg_control;

	reg [3:0] _s;
	always @(posedge clk)
	begin
		_s <= {_s[2], _s[1], _s[0], soft_reset};
	end
	assign reg_control_clear_mask = {30'b0, _s[3], 1'b0};

	assign reg_err_conds_set_mask = err_conds;

    // --- AXI4-Lite access to registers implemented below

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
		if (!resetn)
			write_en <= 0;
		else
			write_en <= (~write_en) && S_AXI_AWVALID
						&& S_AXI_WVALID && (~axi_bvalid);
	end

	always @(posedge clk)
	begin
		if (!resetn)
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
		if (!resetn)
			read_en <= 0;
		else
			read_en <= (~read_en) && S_AXI_ARVALID && (~axi_rvalid);
	end

	always @(posedge clk)
	begin
		if (!resetn)
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
		if (!resetn)
		begin
			reg_control <= 0;
			reg_err_conds <= 0;
		end
		else begin
			reg_control   <= (write_en && write_addr == 4)  ? write_data : (reg_control & ~reg_control_clear_mask);
			reg_err_conds <= (write_en && write_addr == 16) ? write_data : (reg_err_conds | reg_err_conds_set_mask);
		end
	end

	always @(posedge clk)
	begin
		case (read_addr)
			0:			read_data <= {16'hd517, 16'h0006};
			4:			read_data <= reg_control;
			8:			read_data <= frame_counter;
			12:			read_data <= overflow_counter;
			16:			read_data <= reg_err_conds;
			20:			read_data <= sync_reg;
			default:	read_data <= 32'h00000000;
		endcase
	end
endmodule
