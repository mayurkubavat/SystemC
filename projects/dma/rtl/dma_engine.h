// ----------------------------------------------------------------------------
// dma_engine — SystemC behavioral model of a simple DMA engine
//
// This is the "golden model" DUT: a single-word-at-a-time DMA controller
// with an APB3 slave interface for register access and an AXI4-Lite master
// interface for memory transfers. It behaves identically to the Verilog RTL
// in rtl/verilog/dma_engine.v and can be swapped at compile time via USE_RTL.
//
// SystemC / SystemVerilog mapping:
//   SC_MODULE(...)        ~  module ... endmodule
//   sc_in<T> / sc_out<T>  ~  input / output ports
//   SC_METHOD(fn)         ~  always @(...) — combinational or single-edge
//   sensitive << clk.pos()~  always @(posedge clk)
//   dont_initialize()     ~  prevents the method from firing at time 0
// ----------------------------------------------------------------------------
#ifndef DMA_ENGINE_H
#define DMA_ENGINE_H

#include <systemc>
#include <cstdint>
#include "dma_constants.h"

SC_MODULE(dma_engine) {

  // ---- Clock and reset ----
  sc_core::sc_in<bool> PCLK;
  sc_core::sc_in<bool> PRESETn;

  // ---- APB3 slave interface ----
  sc_core::sc_in<bool>      PSEL;
  sc_core::sc_in<bool>      PENABLE;
  sc_core::sc_in<bool>      PWRITE;
  sc_core::sc_in<uint32_t>  PADDR;
  sc_core::sc_in<uint32_t>  PWDATA;

  sc_core::sc_out<uint32_t> PRDATA;
  sc_core::sc_out<bool>     PREADY;
  sc_core::sc_out<bool>     PSLVERR;

  // ---- AXI4-Lite master interface ----
  // Write Address channel (AW)
  sc_core::sc_out<uint32_t> AWADDR;
  sc_core::sc_out<bool>     AWVALID;
  sc_core::sc_in<bool>      AWREADY;

  // Write Data channel (W)
  sc_core::sc_out<uint32_t> WDATA;
  sc_core::sc_out<uint32_t> WSTRB;
  sc_core::sc_out<bool>     WVALID;
  sc_core::sc_in<bool>      WREADY;

  // Write Response channel (B)
  sc_core::sc_in<uint32_t>  BRESP;
  sc_core::sc_in<bool>      BVALID;
  sc_core::sc_out<bool>     BREADY;

  // Read Address channel (AR)
  sc_core::sc_out<uint32_t> ARADDR;
  sc_core::sc_out<bool>     ARVALID;
  sc_core::sc_in<bool>      ARREADY;

  // Read Data channel (R)
  sc_core::sc_in<uint32_t>  RDATA;
  sc_core::sc_in<uint32_t>  RRESP;
  sc_core::sc_in<bool>      RVALID;
  sc_core::sc_out<bool>     RREADY;

  // ---- Interrupt output ----
  sc_core::sc_out<bool>     IRQ;

  // ---- Internal registers (plain C++ variables, NOT sc_signals) ----
  // These are shared between apb_handler and dma_fsm. Since both are
  // SC_METHODs on the same clock edge, they execute in the same delta
  // cycle and can read/write the same C++ variables directly.
  //
  // GOTCHA: The execution ORDER of two SC_METHODs on the same event is
  // non-deterministic in SystemC. Here it's safe because apb_handler
  // only modifies `status` and `ctrl` when starting a new transfer,
  // and dma_fsm only reads them when entering S_IDLE → S_READ_REQ.
  // If the FSM misses the update by one cycle, it catches it the next.
  uint32_t src_addr;
  uint32_t dst_addr;
  uint32_t xfer_len;
  uint32_t ctrl;
  uint32_t status;

  // ---- FSM state encoding ----
  // The DMA transfers one word at a time in a 4-state loop:
  //
  //   IDLE → READ_REQ → READ_RESP → WRITE_REQ → WRITE_RESP → (loop or IDLE)
  //          ├─ assert ARVALID ─┤  ├─ assert AW/WVALID ──┤
  //          └─ wait ARREADY  ──┘  └─ wait AW/WREADY  ───┘
  //                    ├─ wait RVALID ─┤          ├─ wait BVALID ─┤
  //
  enum fsm_state_t {
    S_IDLE       = 0,
    S_READ_REQ   = 1,  // Assert ARVALID, wait for ARREADY
    S_READ_RESP  = 2,  // Assert RREADY, wait for RVALID
    S_WRITE_REQ  = 3,  // Assert AWVALID+WVALID, wait for AWREADY+WREADY
    S_WRITE_RESP = 4   // Assert BREADY, wait for BVALID
  };

  fsm_state_t state;

  // ---- FSM working registers ----
  uint32_t cur_src;          // Current source address (increments by 4 each word)
  uint32_t cur_dst;          // Current destination address
  uint16_t words_remaining;  // Countdown: 0 means transfer complete
  uint32_t read_data_buf;    // Holds data between AXI read and AXI write

  // SC_CTOR — SystemC constructor macro. Registers process callbacks.
  //
  // SC_METHOD vs SC_THREAD:
  //   SC_METHOD: Called once per event, runs to completion (no wait() allowed).
  //              Like SV `always @(posedge clk)` — fast, synthesizable style.
  //   SC_THREAD: Runs continuously with wait() calls to suspend/resume.
  //              Like SV `task` — more flexible but slightly slower.
  //
  // We use SC_METHOD here because the DUT models clocked RTL behavior:
  // one evaluation per clock edge, no blocking waits.
  SC_CTOR(dma_engine) {
    SC_METHOD(apb_handler);
    sensitive << PCLK.pos();    // Trigger on rising edge of PCLK
    dont_initialize();          // Don't call at time 0 (wait for first edge)

    SC_METHOD(dma_fsm);
    sensitive << PCLK.pos();
    dont_initialize();
  }

  // APB register read/write handler — called every posedge PCLK
  void apb_handler() {
    // Active-low reset: clear all registers
    if (!PRESETn.read()) {
      src_addr = 0;
      dst_addr = 0;
      xfer_len = 0;
      ctrl     = 0;
      status   = 0;
      PRDATA.write(0);
      PREADY.write(false);
      PSLVERR.write(false);
      IRQ.write(false);
      return;
    }

    // Single-cycle slave — always ready
    PREADY.write(true);
    PSLVERR.write(false);

    // APB ACCESS phase: PSEL && PENABLE both high
    if (PSEL.read() && PENABLE.read()) {
      uint32_t addr = PADDR.read();

      if (PWRITE.read()) {
        // ---- Write path ----
        // Writes are only allowed when the DMA is not busy
        if (status & DMA_STATUS_BUSY) {
          // Silently ignore writes while busy
          return;
        }
        switch (addr) {
          case DMA_SRC_ADDR:  src_addr = PWDATA.read(); break;
          case DMA_DST_ADDR:  dst_addr = PWDATA.read(); break;
          case DMA_XFER_LEN:  xfer_len = PWDATA.read(); break;
          case DMA_CTRL:
            ctrl = PWDATA.read();
            if (ctrl & DMA_CTRL_START) {
              status = DMA_STATUS_BUSY;
            }
            break;
          case DMA_STATUS:
            // Status register is read-only — ignore writes
            break;
          default:
            PSLVERR.write(true);
            break;
        }
      } else {
        // ---- Read path ----
        switch (addr) {
          case DMA_SRC_ADDR:  PRDATA.write(src_addr); break;
          case DMA_DST_ADDR:  PRDATA.write(dst_addr); break;
          case DMA_XFER_LEN:  PRDATA.write(xfer_len); break;
          case DMA_CTRL:      PRDATA.write(ctrl);     break;
          case DMA_STATUS:    PRDATA.write(status);   break;
          default:
            PSLVERR.write(true);
            PRDATA.write(0);
            break;
        }
      }
    }
  }

  // DMA transfer FSM — called every posedge PCLK
  //
  // This implements a simple single-word-at-a-time copy engine:
  //   For each word: AXI read from src → buffer → AXI write to dst
  //
  // Each AXI channel uses a valid/ready handshake. The FSM asserts valid
  // and waits until the slave asserts ready. In AXI terminology, this DMA
  // is a "master" — it initiates reads and writes to external memory.
  void dma_fsm() {
    // Active-low reset: return to idle, deassert all AXI signals
    if (!PRESETn.read()) {
      state           = S_IDLE;
      cur_src         = 0;
      cur_dst         = 0;
      words_remaining = 0;
      read_data_buf   = 0;

      ARADDR.write(0);
      ARVALID.write(false);
      RREADY.write(false);

      AWADDR.write(0);
      AWVALID.write(false);
      WDATA.write(0);
      WSTRB.write(0);
      WVALID.write(false);
      BREADY.write(false);
      return;
    }

    switch (state) {

      case S_IDLE:
        if ((status & DMA_STATUS_BUSY) && (ctrl & DMA_CTRL_START)) {
          cur_src         = src_addr;
          cur_dst         = dst_addr;
          words_remaining = static_cast<uint16_t>(xfer_len & 0xFFFF);
          ctrl           &= ~DMA_CTRL_START;  // clear start bit

          if (words_remaining == 0) {
            // Zero-length transfer — done immediately
            status = DMA_STATUS_DONE;
            if (ctrl & DMA_CTRL_IRQ_EN) IRQ.write(true);
          } else {
            state = S_READ_REQ;
          }
        }
        break;

      case S_READ_REQ:
        // Drive read address onto AR channel and assert ARVALID.
        // We keep driving every cycle until the slave accepts (ARREADY=1).
        // In sc_signal semantics, the write() takes effect NEXT delta cycle,
        // so the slave sees ARVALID one delta after we write it.
        ARADDR.write(cur_src);
        ARVALID.write(true);

        // AXI handshake: transfer occurs when VALID && READY on same edge
        if (ARREADY.read()) {
          ARVALID.write(false);   // Address accepted — deassert
          RREADY.write(true);     // Tell slave we're ready for read data
          state = S_READ_RESP;
        }
        break;

      case S_READ_RESP:
        if (RVALID.read()) {
          read_data_buf = RDATA.read();
          RREADY.write(false);

          if (RRESP.read() != AXI_RESP_OKAY) {
            // Read error — abort transfer
            status = DMA_STATUS_DONE | DMA_STATUS_ERROR;
            if (ctrl & DMA_CTRL_IRQ_EN) IRQ.write(true);
            state = S_IDLE;
          } else {
            cur_src += 4;
            state = S_WRITE_REQ;
          }
        }
        break;

      case S_WRITE_REQ:
        // AXI4-Lite allows AW and W channels to handshake simultaneously.
        // We assert both AWVALID and WVALID together and wait for both
        // AWREADY and WREADY before proceeding.
        AWADDR.write(cur_dst);
        AWVALID.write(true);
        WDATA.write(read_data_buf);  // Data we read in S_READ_RESP
        WSTRB.write(0xF);            // All 4 byte lanes active
        WVALID.write(true);

        if (AWREADY.read() && WREADY.read()) {
          AWVALID.write(false);
          WVALID.write(false);
          BREADY.write(true);   // Ready to receive write response
          state = S_WRITE_RESP;
        }
        break;

      case S_WRITE_RESP:
        if (BVALID.read()) {
          BREADY.write(false);

          if (BRESP.read() != AXI_RESP_OKAY) {
            // Write error — abort transfer
            status = DMA_STATUS_DONE | DMA_STATUS_ERROR;
            if (ctrl & DMA_CTRL_IRQ_EN) IRQ.write(true);
            state = S_IDLE;
          } else {
            cur_dst += 4;
            words_remaining--;

            if (words_remaining == 0) {
              // Transfer complete
              status = DMA_STATUS_DONE;
              if (ctrl & DMA_CTRL_IRQ_EN) IRQ.write(true);
              state = S_IDLE;
            } else {
              state = S_READ_REQ;
            }
          }
        }
        break;
    }
  }
};

#endif // DMA_ENGINE_H
