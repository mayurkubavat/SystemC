// ----------------------------------------------------------------------------
// apb_slave — Verilog RTL equivalent of the SystemC model (apb_slave.h)
//
// This file is compiled by Verilator into a C++ class (Vapb_slave) that
// plugs into the same UVM-SystemC testbench. Build with -DUSE_RTL=ON.
// ----------------------------------------------------------------------------
module apb_slave (
  input wire PCLK,
  input wire PRESETn,

  // APB3 interface
  input wire PSEL,
  input wire PENABLE,
  input wire PWRITE,
  input wire [31:0] PADDR,
  input wire [31:0] PWDATA,

  output reg [31:0] PRDATA,
  output wire PREADY,
  output reg PSLVERR
);

  // 16 x 32-bit register file
  reg [31:0] regs [0:15];

  // Single-cycle access — always ready
  assign PREADY = 1'b1;

  // Address bounds
  localparam ADDR_MAX = 15 * 4; // 0x3C

  integer i;

  always @(posedge PCLK or negedge PRESETn) begin
    if (!PRESETn) begin
      PRDATA <= 32'h0;
      PSLVERR <= 1'b0;
      for (i = 0; i < 16; i = i + 1)
        regs[i] <= 32'h0;
    end else begin
      PSLVERR <= 1'b0;

      if (PSEL && PENABLE) begin
        if (PADDR[1:0] != 2'b00 || PADDR > ADDR_MAX) begin
          PSLVERR <= 1'b1;
          PRDATA <= 32'h0;
        end else if (PWRITE) begin
          regs[PADDR[5:2]] <= PWDATA;
        end else begin
          PRDATA <= regs[PADDR[5:2]];
        end
      end
    end
  end

endmodule
