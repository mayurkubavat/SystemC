// ----------------------------------------------------------------------------
// dma_engine — Verilog RTL equivalent of the SystemC model (dma_engine.h)
//
// This file is compiled by Verilator into a C++ class (Vdma_engine) that
// plugs into the same UVM-SystemC testbench. Build with -DUSE_RTL=ON.
// ----------------------------------------------------------------------------
module dma_engine (
  input wire PCLK,
  input wire PRESETn,

  // APB3 slave interface
  input  wire        PSEL,
  input  wire        PENABLE,
  input  wire        PWRITE,
  input  wire [31:0] PADDR,
  input  wire [31:0] PWDATA,

  output reg  [31:0] PRDATA,
  output wire        PREADY,
  output reg         PSLVERR,

  // AXI4-Lite master interface — Write Address channel (AW)
  output reg  [31:0] AWADDR,
  output reg         AWVALID,
  input  wire        AWREADY,

  // AXI4-Lite master interface — Write Data channel (W)
  output reg  [31:0] WDATA,
  output reg  [31:0] WSTRB,
  output reg         WVALID,
  input  wire        WREADY,

  // AXI4-Lite master interface — Write Response channel (B)
  input  wire [31:0] BRESP,
  input  wire        BVALID,
  output reg         BREADY,

  // AXI4-Lite master interface — Read Address channel (AR)
  output reg  [31:0] ARADDR,
  output reg         ARVALID,
  input  wire        ARREADY,

  // AXI4-Lite master interface — Read Data channel (R)
  input  wire [31:0] RDATA,
  input  wire [31:0] RRESP,
  input  wire        RVALID,
  output reg         RREADY,

  // Interrupt output
  output reg         IRQ
);

  // Single-cycle access — always ready
  assign PREADY = 1'b1;

  // ---- DMA register offsets ----
  localparam REG_SRC_ADDR = 32'h00;
  localparam REG_DST_ADDR = 32'h04;
  localparam REG_XFER_LEN = 32'h08;
  localparam REG_CTRL     = 32'h0C;
  localparam REG_STATUS   = 32'h10;

  // ---- Control register bits ----
  localparam CTRL_START  = 32'h1;  // bit 0
  localparam CTRL_IRQ_EN = 32'h2;  // bit 1

  // ---- Status register encoding ----
  localparam STATUS_IDLE  = 32'h0;
  localparam STATUS_BUSY  = 32'h1;  // bit 0
  localparam STATUS_DONE  = 32'h2;  // bit 1
  localparam STATUS_ERROR = 32'h4;  // bit 2

  // ---- FSM state encoding ----
  localparam S_IDLE       = 3'd0;
  localparam S_READ_REQ   = 3'd1;
  localparam S_READ_RESP  = 3'd2;
  localparam S_WRITE_REQ  = 3'd3;
  localparam S_WRITE_RESP = 3'd4;

  // ---- AXI response codes ----
  localparam RESP_OKAY = 32'h0;

  // ---- Internal registers ----
  reg [31:0] src_addr;
  reg [31:0] dst_addr;
  reg [31:0] xfer_len;
  reg [31:0] ctrl;
  reg [31:0] status;

  // ---- FSM working registers ----
  reg [2:0]  state;
  reg [31:0] cur_src;
  reg [31:0] cur_dst;
  reg [15:0] words_remaining;
  reg [31:0] read_data_buf;

  // -----------------------------------------------------------------------
  // APB register read/write handler
  // -----------------------------------------------------------------------
  always @(posedge PCLK or negedge PRESETn) begin
    if (!PRESETn) begin
      src_addr <= 32'h0;
      dst_addr <= 32'h0;
      xfer_len <= 32'h0;
      ctrl     <= 32'h0;
      status   <= 32'h0;
      PRDATA   <= 32'h0;
      PSLVERR  <= 1'b0;
      IRQ      <= 1'b0;
    end else begin
      PSLVERR <= 1'b0;

      if (PSEL && PENABLE) begin
        if (PWRITE) begin
          // Writes only allowed when DMA is not busy
          if (!(|(status & STATUS_BUSY))) begin
            case (PADDR)
              REG_SRC_ADDR: src_addr <= PWDATA;
              REG_DST_ADDR: dst_addr <= PWDATA;
              REG_XFER_LEN: xfer_len <= PWDATA;
              REG_CTRL: begin
                ctrl <= PWDATA;
                if (|(PWDATA & CTRL_START))
                  status <= STATUS_BUSY;
              end
              REG_STATUS: begin
                // Read-only — ignore writes
              end
              default: PSLVERR <= 1'b1;
            endcase
          end
        end else begin
          case (PADDR)
            REG_SRC_ADDR: PRDATA <= src_addr;
            REG_DST_ADDR: PRDATA <= dst_addr;
            REG_XFER_LEN: PRDATA <= xfer_len;
            REG_CTRL:     PRDATA <= ctrl;
            REG_STATUS:   PRDATA <= status;
            default: begin
              PSLVERR <= 1'b1;
              PRDATA  <= 32'h0;
            end
          endcase
        end
      end
    end
  end

  // -----------------------------------------------------------------------
  // DMA transfer FSM
  // -----------------------------------------------------------------------
  always @(posedge PCLK or negedge PRESETn) begin
    if (!PRESETn) begin
      state           <= S_IDLE;
      cur_src         <= 32'h0;
      cur_dst         <= 32'h0;
      words_remaining <= 16'h0;
      read_data_buf   <= 32'h0;

      ARADDR  <= 32'h0;
      ARVALID <= 1'b0;
      RREADY  <= 1'b0;

      AWADDR  <= 32'h0;
      AWVALID <= 1'b0;
      WDATA   <= 32'h0;
      WSTRB   <= 32'h0;
      WVALID  <= 1'b0;
      BREADY  <= 1'b0;
    end else begin
      case (state)

        S_IDLE: begin
          if (|(status & STATUS_BUSY) && |(ctrl & CTRL_START)) begin
            cur_src         <= src_addr;
            cur_dst         <= dst_addr;
            words_remaining <= xfer_len[15:0];
            ctrl            <= ctrl & ~CTRL_START;  // clear start bit

            if (xfer_len[15:0] == 16'h0) begin
              // Zero-length transfer — done immediately
              status <= STATUS_DONE;
              if (|(ctrl & CTRL_IRQ_EN)) IRQ <= 1'b1;
              state  <= S_IDLE;
            end else begin
              state <= S_READ_REQ;
            end
          end
        end

        S_READ_REQ: begin
          ARADDR  <= cur_src;
          ARVALID <= 1'b1;

          if (ARREADY) begin
            ARVALID <= 1'b0;
            RREADY  <= 1'b1;
            state   <= S_READ_RESP;
          end
        end

        S_READ_RESP: begin
          if (RVALID) begin
            read_data_buf <= RDATA;
            RREADY        <= 1'b0;

            if (RRESP != RESP_OKAY) begin
              // Read error — abort transfer
              status <= STATUS_DONE | STATUS_ERROR;
              if (|(ctrl & CTRL_IRQ_EN)) IRQ <= 1'b1;
              state  <= S_IDLE;
            end else begin
              cur_src <= cur_src + 32'd4;
              state   <= S_WRITE_REQ;
            end
          end
        end

        S_WRITE_REQ: begin
          AWADDR  <= cur_dst;
          AWVALID <= 1'b1;
          WDATA   <= read_data_buf;
          WSTRB   <= 32'h0000_000F;
          WVALID  <= 1'b1;

          if (AWREADY && WREADY) begin
            AWVALID <= 1'b0;
            WVALID  <= 1'b0;
            BREADY  <= 1'b1;
            state   <= S_WRITE_RESP;
          end
        end

        S_WRITE_RESP: begin
          if (BVALID) begin
            BREADY <= 1'b0;

            if (BRESP != RESP_OKAY) begin
              // Write error — abort transfer
              status <= STATUS_DONE | STATUS_ERROR;
              if (|(ctrl & CTRL_IRQ_EN)) IRQ <= 1'b1;
              state  <= S_IDLE;
            end else begin
              cur_dst         <= cur_dst + 32'd4;
              words_remaining <= words_remaining - 16'd1;

              if (words_remaining == 16'd1) begin
                // Last word transferred — done
                status <= STATUS_DONE;
                if (|(ctrl & CTRL_IRQ_EN)) IRQ <= 1'b1;
                state  <= S_IDLE;
              end else begin
                state <= S_READ_REQ;
              end
            end
          end
        end

        default: state <= S_IDLE;

      endcase
    end
  end

endmodule
