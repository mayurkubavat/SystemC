# DMA Engine Implementation Plan

> **For Claude:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task.

**Goal:** Build a single-channel DMA engine (SystemC + Verilog) with full UVM-SystemC verification and Verilator co-simulation, reusing the existing APB agent for register programming and introducing a new AXI4-Lite slave agent for memory simulation.

**Architecture:** APB slave interface for CPU register programming (5 control/status registers), AXI4-Lite master interface for memory-to-memory data transfers. Dual-model DUT (SystemC behavioral + Verilog RTL) verified by the same UVM testbench, switchable via CMake `-DUSE_RTL=ON`.

**Tech Stack:** SystemC 3.0+, UVM-SystemC 1.0-beta6, Verilator 5+, CMake 3.14+, C++17

---

### Task 1: Create DMA project directory structure and CMake skeleton

**Files:**
- Create: `projects/dma/CMakeLists.txt`
- Create: `projects/dma/rtl/.gitkeep` (placeholder — removed when real files added)
- Create: `projects/dma/rtl/verilog/.gitkeep`
- Create: `projects/dma/env/.gitkeep`
- Create: `projects/dma/agents/.gitkeep`
- Create: `projects/dma/sequences/.gitkeep`
- Create: `projects/dma/test/.gitkeep`
- Create: `projects/dma/top/.gitkeep`
- Modify: `projects/CMakeLists.txt`

**Step 1: Create directory structure**

```bash
mkdir -p projects/dma/{rtl/verilog,env,agents,sequences,test,top}
```

**Step 2: Create CMakeLists.txt for DMA project**

Follow the exact pattern from `projects/apb/CMakeLists.txt`. The DMA project includes both APB (reused) and AXI4-Lite (new) agent directories.

```cmake
option(USE_RTL "Use RTL (Verilog) instead of SystemC model" OFF)

add_executable(dma_sim top/top.cpp)

target_include_directories(dma_sim PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/rtl
    ${CMAKE_CURRENT_SOURCE_DIR}/env
    ${CMAKE_CURRENT_SOURCE_DIR}/agents
    ${CMAKE_CURRENT_SOURCE_DIR}/sequences
    ${CMAKE_CURRENT_SOURCE_DIR}/test
    # Reuse APB components from sibling project
    ${CMAKE_SOURCE_DIR}/projects/apb/rtl
    ${CMAKE_SOURCE_DIR}/projects/apb/env
    ${CMAKE_SOURCE_DIR}/projects/apb/agents
    ${CMAKE_SOURCE_DIR}/projects/apb/sequences
)

target_link_libraries(dma_sim PRIVATE
    SystemC::systemc
    SystemC::uvm
)

if(USE_RTL)
    find_package(verilator REQUIRED)

    verilate(dma_sim
        SYSTEMC
        SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/rtl/verilog/dma_engine.v
    )

    target_compile_definitions(dma_sim PRIVATE USE_RTL=1)
    message(STATUS "DMA: using RTL DUT (via Verilator)")
else()
    message(STATUS "DMA: using SystemC model DUT")
endif()
```

**Step 3: Register DMA subdirectory in projects/CMakeLists.txt**

Add to existing file:

```cmake
add_subdirectory(apb)
add_subdirectory(dma)
```

**Step 4: Verify CMake configuration**

Run: `cd build && cmake .. -DSYSTEMC_HOME=$SYSTEMC_HOME -DUVM_SYSTEMC_HOME=$UVM_SYSTEMC_HOME`

Expected: Should configure without errors. The `dma_sim` target won't build yet (no top.cpp), but CMake should process the file.

**Step 5: Commit**

```bash
git add projects/dma/CMakeLists.txt projects/CMakeLists.txt
git commit -m "feat(dma): add project directory structure and CMake skeleton"
```

---

### Task 2: Create AXI4-Lite signal interface and DMA constants

**Files:**
- Create: `projects/dma/rtl/axi_lite_if.h`
- Create: `projects/dma/env/dma_constants.h`

**Step 1: Create AXI4-Lite signal interface**

Follow the exact pattern of `projects/apb/rtl/apb_if.h` — an `sc_module` wrapping `sc_signal` members. This is the "virtual interface" for AXI4-Lite.

```cpp
// ----------------------------------------------------------------------------
// axi_lite_if — AXI4-Lite signal-level interface
//
// Groups all AXI4-Lite bus signals into one sc_module, following the same
// "virtual interface" pattern as apb_if.h. Passed through uvm_config_db.
//
// AXI4-Lite has 5 channels: Write Address (AW), Write Data (W),
// Write Response (B), Read Address (AR), Read Data (R).
// No burst, no IDs, no cache/prot — just address + data + handshake.
// ----------------------------------------------------------------------------
#ifndef AXI_LITE_IF_H
#define AXI_LITE_IF_H

#include <systemc>
#include <cstdint>

class axi_lite_if : public sc_core::sc_module {
public:
  // Write Address channel
  sc_core::sc_signal<uint32_t> AWADDR;
  sc_core::sc_signal<bool>     AWVALID;
  sc_core::sc_signal<bool>     AWREADY;

  // Write Data channel
  sc_core::sc_signal<uint32_t> WDATA;
  sc_core::sc_signal<uint32_t> WSTRB;
  sc_core::sc_signal<bool>     WVALID;
  sc_core::sc_signal<bool>     WREADY;

  // Write Response channel
  sc_core::sc_signal<uint32_t> BRESP;
  sc_core::sc_signal<bool>     BVALID;
  sc_core::sc_signal<bool>     BREADY;

  // Read Address channel
  sc_core::sc_signal<uint32_t> ARADDR;
  sc_core::sc_signal<bool>     ARVALID;
  sc_core::sc_signal<bool>     ARREADY;

  // Read Data channel
  sc_core::sc_signal<uint32_t> RDATA;
  sc_core::sc_signal<uint32_t> RRESP;
  sc_core::sc_signal<bool>     RVALID;
  sc_core::sc_signal<bool>     RREADY;

  axi_lite_if(const sc_core::sc_module_name& name)
    : sc_module(name),
      AWADDR("AWADDR"), AWVALID("AWVALID"), AWREADY("AWREADY"),
      WDATA("WDATA"), WSTRB("WSTRB"), WVALID("WVALID"), WREADY("WREADY"),
      BRESP("BRESP"), BVALID("BVALID"), BREADY("BREADY"),
      ARADDR("ARADDR"), ARVALID("ARVALID"), ARREADY("ARREADY"),
      RDATA("RDATA"), RRESP("RRESP"), RVALID("RVALID"), RREADY("RREADY") {}
};

#endif // AXI_LITE_IF_H
```

**Step 2: Create DMA constants**

Follow pattern from `projects/apb/env/apb_constants.h`.

```cpp
// ----------------------------------------------------------------------------
// dma_constants — Register offsets, field positions, and AXI response codes
// ----------------------------------------------------------------------------
#ifndef DMA_CONSTANTS_H
#define DMA_CONSTANTS_H

#include <cstdint>

// DMA register offsets (APB address space)
static const uint32_t DMA_SRC_ADDR  = 0x00;
static const uint32_t DMA_DST_ADDR  = 0x04;
static const uint32_t DMA_XFER_LEN  = 0x08;
static const uint32_t DMA_CTRL      = 0x0C;
static const uint32_t DMA_STATUS    = 0x10;

// DMA_CTRL bit positions
static const uint32_t DMA_CTRL_START  = (1 << 0);
static const uint32_t DMA_CTRL_IRQ_EN = (1 << 1);

// DMA_STATUS bit positions
static const uint32_t DMA_STATUS_BUSY  = (1 << 0);
static const uint32_t DMA_STATUS_DONE  = (1 << 1);
static const uint32_t DMA_STATUS_ERROR = (1 << 2);

// AXI response codes
static const uint32_t AXI_RESP_OKAY   = 0x0;
static const uint32_t AXI_RESP_SLVERR = 0x2;

#endif // DMA_CONSTANTS_H
```

**Step 3: Commit**

```bash
git add projects/dma/rtl/axi_lite_if.h projects/dma/env/dma_constants.h
git commit -m "feat(dma): add AXI4-Lite signal interface and DMA constants"
```

---

### Task 3: Create AXI4-Lite transaction class

**Files:**
- Create: `projects/dma/env/axi_lite_transaction.h`

**Step 1: Create AXI4-Lite transaction**

Follow the exact pattern of `projects/apb/env/apb_transaction.h` — extends `uvm_sequence_item` with `UVM_OBJECT_UTILS`, `do_copy`, `do_compare`, `do_print`, `convert2string`.

```cpp
// ----------------------------------------------------------------------------
// axi_lite_transaction — UVM sequence item for AXI4-Lite transfers
//
// Represents a single AXI4-Lite read or write. Used by the AXI slave agent
// to communicate observed transactions to the scoreboard.
// ----------------------------------------------------------------------------
#ifndef AXI_LITE_TRANSACTION_H
#define AXI_LITE_TRANSACTION_H

#include <systemc>
#include <uvm>
#include <cstdint>
#include <iomanip>
#include <sstream>

#include "dma_constants.h"

enum axi_direction_t { AXI_READ, AXI_WRITE };

class axi_lite_transaction : public uvm::uvm_sequence_item {
public:
  uint32_t addr;
  uint32_t data;
  axi_direction_t direction;
  uint32_t resp;  // AXI response code

  axi_lite_transaction(const std::string& name = "axi_lite_transaction")
    : uvm::uvm_sequence_item(name), addr(0), data(0),
      direction(AXI_READ), resp(AXI_RESP_OKAY) {}

  UVM_OBJECT_UTILS(axi_lite_transaction);

  virtual void do_copy(const uvm::uvm_object& rhs) {
    const axi_lite_transaction* rhs_ = dynamic_cast<const axi_lite_transaction*>(&rhs);
    if (!rhs_) UVM_FATAL("do_copy", "cast failed");
    uvm_sequence_item::do_copy(rhs);
    addr = rhs_->addr;
    data = rhs_->data;
    direction = rhs_->direction;
    resp = rhs_->resp;
  }

  virtual bool do_compare(const uvm::uvm_object& rhs,
    const uvm::uvm_comparer* comparer) {
    const axi_lite_transaction* rhs_ = dynamic_cast<const axi_lite_transaction*>(&rhs);
    if (!rhs_) UVM_FATAL("do_compare", "cast failed");
    return (addr == rhs_->addr && data == rhs_->data &&
      direction == rhs_->direction);
  }

  void do_print(const uvm::uvm_printer& printer) const {
    printer.print_string("dir", (direction == AXI_WRITE) ? "WRITE" : "READ");
    printer.print_field_int("addr", addr);
    printer.print_field_int("data", data);
    printer.print_field_int("resp", resp);
  }

  std::string convert2string() {
    std::ostringstream ss;
    ss << "AXI " << (direction == AXI_WRITE ? "WR" : "RD")
       << " addr=0x" << std::hex << std::setw(8) << std::setfill('0') << addr
       << " data=0x" << std::setw(8) << std::setfill('0') << data;
    if (resp != AXI_RESP_OKAY) ss << " [ERR]";
    return ss.str();
  }
};

#endif // AXI_LITE_TRANSACTION_H
```

**Step 2: Commit**

```bash
git add projects/dma/env/axi_lite_transaction.h
git commit -m "feat(dma): add AXI4-Lite transaction class"
```

---

### Task 4: Create SystemC DMA engine behavioral model

**Files:**
- Create: `projects/dma/rtl/dma_engine.h`

This is the core DUT. It has APB slave ports for register access and AXI4-Lite master ports for data transfers.

**Step 1: Create DMA engine SystemC model**

```cpp
// ----------------------------------------------------------------------------
// dma_engine — SystemC behavioral model of a single-channel DMA engine
//
// Control plane: APB3 slave interface for register programming
// Data plane:    AXI4-Lite master interface for memory-to-memory transfers
//
// Register map:
//   0x00 DMA_SRC_ADDR  — source address (auto-incrementing)
//   0x04 DMA_DST_ADDR  — destination address (auto-incrementing)
//   0x08 DMA_XFER_LEN  — transfer length in words
//   0x0C DMA_CTRL      — [0] start, [1] irq_en
//   0x10 DMA_STATUS    — [0] busy, [1] done, [2] error (read-only)
//
// State machine: IDLE → READ_REQ → READ_RESP → WRITE_REQ → WRITE_RESP → loop
// ----------------------------------------------------------------------------
#ifndef DMA_ENGINE_H
#define DMA_ENGINE_H

#include <systemc>
#include <cstdint>
#include "dma_constants.h"

SC_MODULE(dma_engine) {

  // Clock and reset
  sc_core::sc_in<bool> PCLK;
  sc_core::sc_in<bool> PRESETn;

  // APB3 slave interface (control plane)
  sc_core::sc_in<bool>     PSEL;
  sc_core::sc_in<bool>     PENABLE;
  sc_core::sc_in<bool>     PWRITE;
  sc_core::sc_in<uint32_t> PADDR;
  sc_core::sc_in<uint32_t> PWDATA;
  sc_core::sc_out<uint32_t> PRDATA;
  sc_core::sc_out<bool>     PREADY;
  sc_core::sc_out<bool>     PSLVERR;

  // AXI4-Lite master interface (data plane)
  // Write Address channel
  sc_core::sc_out<uint32_t> AWADDR;
  sc_core::sc_out<bool>     AWVALID;
  sc_core::sc_in<bool>      AWREADY;
  // Write Data channel
  sc_core::sc_out<uint32_t> WDATA;
  sc_core::sc_out<uint32_t> WSTRB;
  sc_core::sc_out<bool>     WVALID;
  sc_core::sc_in<bool>      WREADY;
  // Write Response channel
  sc_core::sc_in<uint32_t>  BRESP;
  sc_core::sc_in<bool>      BVALID;
  sc_core::sc_out<bool>     BREADY;
  // Read Address channel
  sc_core::sc_out<uint32_t> ARADDR;
  sc_core::sc_out<bool>     ARVALID;
  sc_core::sc_in<bool>      ARREADY;
  // Read Data channel
  sc_core::sc_in<uint32_t>  RDATA;
  sc_core::sc_in<uint32_t>  RRESP;
  sc_core::sc_in<bool>      RVALID;
  sc_core::sc_out<bool>     RREADY;

  // Interrupt output
  sc_core::sc_out<bool> IRQ;

  // Internal registers
  uint32_t src_addr;
  uint32_t dst_addr;
  uint32_t xfer_len;
  uint32_t ctrl;
  uint32_t status;

  // DMA state machine
  enum dma_state_t { IDLE, READ_REQ, READ_RESP, WRITE_REQ, WRITE_RESP };
  dma_state_t state;
  uint32_t words_remaining;
  uint32_t cur_src;
  uint32_t cur_dst;
  uint32_t read_data_buf;  // holds data between read and write phases

  SC_CTOR(dma_engine) {
    // APB register access — combinational, fires every posedge
    SC_METHOD(apb_handler);
    sensitive << PCLK.pos();
    dont_initialize();

    // DMA transfer engine — sequential state machine
    SC_METHOD(dma_fsm);
    sensitive << PCLK.pos();
    dont_initialize();
  }

  // ── APB register access ──────────────────────────────────────────────
  void apb_handler() {
    if (!PRESETn.read()) {
      src_addr = 0; dst_addr = 0; xfer_len = 0;
      ctrl = 0; status = 0;
      PRDATA.write(0);
      PREADY.write(false);
      PSLVERR.write(false);
      return;
    }

    PREADY.write(true);
    PSLVERR.write(false);

    if (PSEL.read() && PENABLE.read()) {
      uint32_t addr = PADDR.read();

      // Only allow register writes when DMA is not busy
      if (PWRITE.read()) {
        bool busy = (status & DMA_STATUS_BUSY) != 0;
        switch (addr) {
          case DMA_SRC_ADDR:  if (!busy) src_addr = PWDATA.read(); break;
          case DMA_DST_ADDR:  if (!busy) dst_addr = PWDATA.read(); break;
          case DMA_XFER_LEN:  if (!busy) xfer_len = PWDATA.read() & 0xFFFF; break;
          case DMA_CTRL:
            if (!busy) {
              ctrl = PWDATA.read();
              // Start bit triggers the transfer
              if (ctrl & DMA_CTRL_START) {
                status = DMA_STATUS_BUSY;
                // Clear done from previous transfer
              }
            }
            break;
          case DMA_STATUS: break; // read-only, ignore writes
          default: PSLVERR.write(true); break;
        }
      } else {
        // Read
        switch (addr) {
          case DMA_SRC_ADDR:  PRDATA.write(src_addr); break;
          case DMA_DST_ADDR:  PRDATA.write(dst_addr); break;
          case DMA_XFER_LEN:  PRDATA.write(xfer_len); break;
          case DMA_CTRL:      PRDATA.write(ctrl); break;
          case DMA_STATUS:    PRDATA.write(status); break;
          default: PSLVERR.write(true); PRDATA.write(0); break;
        }
      }
    }
  }

  // ── DMA transfer state machine ──────────────────────────────────────
  void dma_fsm() {
    if (!PRESETn.read()) {
      state = IDLE;
      words_remaining = 0;
      cur_src = 0; cur_dst = 0;
      read_data_buf = 0;
      // De-assert all AXI master outputs
      ARVALID.write(false); ARADDR.write(0);
      RREADY.write(false);
      AWVALID.write(false); AWADDR.write(0);
      WVALID.write(false); WDATA.write(0); WSTRB.write(0);
      BREADY.write(false);
      IRQ.write(false);
      return;
    }

    switch (state) {
      case IDLE:
        // Check if APB handler set the start bit
        if ((status & DMA_STATUS_BUSY) && (ctrl & DMA_CTRL_START)) {
          cur_src = src_addr;
          cur_dst = dst_addr;
          words_remaining = xfer_len;
          ctrl &= ~DMA_CTRL_START;  // clear start bit
          if (words_remaining > 0) {
            state = READ_REQ;
          } else {
            // Zero-length transfer: immediately done
            status = DMA_STATUS_DONE;
            if (ctrl & DMA_CTRL_IRQ_EN) IRQ.write(true);
          }
        }
        break;

      case READ_REQ:
        // Issue AXI read address
        ARADDR.write(cur_src);
        ARVALID.write(true);
        if (ARREADY.read()) {
          ARVALID.write(false);
          RREADY.write(true);
          state = READ_RESP;
        }
        break;

      case READ_RESP:
        // Wait for read data
        if (RVALID.read()) {
          read_data_buf = RDATA.read();
          RREADY.write(false);
          if (RRESP.read() != AXI_RESP_OKAY) {
            // Error: abort transfer
            status = DMA_STATUS_DONE | DMA_STATUS_ERROR;
            if (ctrl & DMA_CTRL_IRQ_EN) IRQ.write(true);
            state = IDLE;
          } else {
            cur_src += 4;
            state = WRITE_REQ;
          }
        }
        break;

      case WRITE_REQ:
        // Issue AXI write address + data simultaneously
        AWADDR.write(cur_dst);
        AWVALID.write(true);
        WDATA.write(read_data_buf);
        WSTRB.write(0xF);  // all byte lanes
        WVALID.write(true);
        if (AWREADY.read() && WREADY.read()) {
          AWVALID.write(false);
          WVALID.write(false);
          BREADY.write(true);
          state = WRITE_RESP;
        }
        break;

      case WRITE_RESP:
        // Wait for write response
        if (BVALID.read()) {
          BREADY.write(false);
          if (BRESP.read() != AXI_RESP_OKAY) {
            status = DMA_STATUS_DONE | DMA_STATUS_ERROR;
            if (ctrl & DMA_CTRL_IRQ_EN) IRQ.write(true);
            state = IDLE;
          } else {
            cur_dst += 4;
            words_remaining--;
            if (words_remaining == 0) {
              status = DMA_STATUS_DONE;
              if (ctrl & DMA_CTRL_IRQ_EN) IRQ.write(true);
              state = IDLE;
            } else {
              state = READ_REQ;
            }
          }
        }
        break;
    }
  }
};

#endif // DMA_ENGINE_H
```

**Step 2: Commit**

```bash
git add projects/dma/rtl/dma_engine.h
git commit -m "feat(dma): add SystemC behavioral DMA engine model"
```

---

### Task 5: Create Verilog DMA engine RTL

**Files:**
- Create: `projects/dma/rtl/verilog/dma_engine.v`

This is the Verilog RTL equivalent of the SystemC model, following the same pattern as `projects/apb/rtl/verilog/apb_slave.v`.

**Step 1: Create Verilog DMA engine**

```verilog
// ----------------------------------------------------------------------------
// dma_engine — Verilog RTL equivalent of the SystemC model (dma_engine.h)
//
// Single-channel DMA: APB3 slave (config) + AXI4-Lite master (data).
// Compiled by Verilator into Vdma_engine class. Build with -DUSE_RTL=ON.
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

  // AXI4-Lite master interface
  output reg  [31:0] AWADDR,
  output reg         AWVALID,
  input  wire        AWREADY,
  output reg  [31:0] WDATA,
  output reg  [31:0] WSTRB,
  output reg         WVALID,
  input  wire        WREADY,
  input  wire [31:0] BRESP,
  input  wire        BVALID,
  output reg         BREADY,
  output reg  [31:0] ARADDR,
  output reg         ARVALID,
  input  wire        ARREADY,
  input  wire [31:0] RDATA,
  input  wire [31:0] RRESP,
  input  wire        RVALID,
  output reg         RREADY,

  // Interrupt
  output reg         IRQ
);

  // Register offsets
  localparam REG_SRC_ADDR  = 32'h00;
  localparam REG_DST_ADDR  = 32'h04;
  localparam REG_XFER_LEN  = 32'h08;
  localparam REG_CTRL      = 32'h0C;
  localparam REG_STATUS    = 32'h10;

  // Control bits
  localparam CTRL_START  = 0;
  localparam CTRL_IRQ_EN = 1;

  // Status bits
  localparam STAT_BUSY  = 0;
  localparam STAT_DONE  = 1;
  localparam STAT_ERROR = 2;

  // FSM states
  localparam S_IDLE       = 3'd0;
  localparam S_READ_REQ   = 3'd1;
  localparam S_READ_RESP  = 3'd2;
  localparam S_WRITE_REQ  = 3'd3;
  localparam S_WRITE_RESP = 3'd4;

  // Registers
  reg [31:0] src_addr;
  reg [31:0] dst_addr;
  reg [15:0] xfer_len;
  reg [31:0] ctrl;
  reg [31:0] status;

  // FSM state
  reg [2:0]  state;
  reg [15:0] words_remaining;
  reg [31:0] cur_src;
  reg [31:0] cur_dst;
  reg [31:0] read_data_buf;

  // APB always ready
  assign PREADY = 1'b1;

  // ── APB register access ──────────────────────────────────────────
  always @(posedge PCLK or negedge PRESETn) begin
    if (!PRESETn) begin
      src_addr <= 32'h0;
      dst_addr <= 32'h0;
      xfer_len <= 16'h0;
      ctrl     <= 32'h0;
      status   <= 32'h0;
      PRDATA   <= 32'h0;
      PSLVERR  <= 1'b0;
    end else begin
      PSLVERR <= 1'b0;

      // FSM sets status — clear start bit after launch
      if (state == S_READ_REQ && ctrl[CTRL_START]) begin
        ctrl[CTRL_START] <= 1'b0;
      end

      if (PSEL && PENABLE) begin
        if (PWRITE) begin
          if (!status[STAT_BUSY]) begin
            case (PADDR)
              REG_SRC_ADDR: src_addr <= PWDATA;
              REG_DST_ADDR: dst_addr <= PWDATA;
              REG_XFER_LEN: xfer_len <= PWDATA[15:0];
              REG_CTRL: begin
                ctrl <= PWDATA;
                if (PWDATA[CTRL_START]) begin
                  status <= 32'h1; // set busy
                end
              end
              REG_STATUS: ; // read-only
              default: PSLVERR <= 1'b1;
            endcase
          end
        end else begin
          case (PADDR)
            REG_SRC_ADDR: PRDATA <= src_addr;
            REG_DST_ADDR: PRDATA <= dst_addr;
            REG_XFER_LEN: PRDATA <= {16'h0, xfer_len};
            REG_CTRL:     PRDATA <= ctrl;
            REG_STATUS:   PRDATA <= status;
            default: begin PSLVERR <= 1'b1; PRDATA <= 32'h0; end
          endcase
        end
      end
    end
  end

  // ── DMA transfer FSM ────────────────────────────────────────────
  always @(posedge PCLK or negedge PRESETn) begin
    if (!PRESETn) begin
      state          <= S_IDLE;
      words_remaining <= 16'h0;
      cur_src        <= 32'h0;
      cur_dst        <= 32'h0;
      read_data_buf  <= 32'h0;
      ARVALID <= 1'b0; ARADDR <= 32'h0;
      RREADY  <= 1'b0;
      AWVALID <= 1'b0; AWADDR <= 32'h0;
      WVALID  <= 1'b0; WDATA  <= 32'h0; WSTRB <= 32'h0;
      BREADY  <= 1'b0;
      IRQ     <= 1'b0;
    end else begin
      case (state)
        S_IDLE: begin
          IRQ <= 1'b0;
          if (status[STAT_BUSY] && ctrl[CTRL_START]) begin
            cur_src         <= src_addr;
            cur_dst         <= dst_addr;
            words_remaining <= xfer_len;
            if (xfer_len > 0) begin
              state <= S_READ_REQ;
            end else begin
              status <= 32'h2; // done
              if (ctrl[CTRL_IRQ_EN]) IRQ <= 1'b1;
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
            RREADY <= 1'b0;
            if (RRESP != 32'h0) begin
              status <= 32'h6; // done + error
              if (ctrl[CTRL_IRQ_EN]) IRQ <= 1'b1;
              state <= S_IDLE;
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
          WSTRB   <= 32'hF;
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
            if (BRESP != 32'h0) begin
              status <= 32'h6; // done + error
              if (ctrl[CTRL_IRQ_EN]) IRQ <= 1'b1;
              state <= S_IDLE;
            end else begin
              cur_dst         <= cur_dst + 32'd4;
              words_remaining <= words_remaining - 16'd1;
              if (words_remaining == 16'd1) begin
                status <= 32'h2; // done
                if (ctrl[CTRL_IRQ_EN]) IRQ <= 1'b1;
                state <= S_IDLE;
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
```

**Step 2: Commit**

```bash
git add projects/dma/rtl/verilog/dma_engine.v
git commit -m "feat(dma): add Verilog RTL DMA engine for Verilator co-simulation"
```

---

### Task 6: Create AXI4-Lite slave driver (memory responder)

**Files:**
- Create: `projects/dma/agents/axi_lite_slave_driver.h`
- Create: `projects/dma/env/memory_model.h`

The AXI slave driver simulates memory — it responds to the DMA engine's read/write requests using a backing memory array.

**Step 1: Create memory model**

```cpp
// ----------------------------------------------------------------------------
// memory_model — Simple memory backing store for the AXI slave agent
//
// Pre-loadable array that serves read data and stores write data.
// Used by the AXI slave driver and the scoreboard for verification.
// ----------------------------------------------------------------------------
#ifndef MEMORY_MODEL_H
#define MEMORY_MODEL_H

#include <cstdint>
#include <map>

class memory_model {
public:
  std::map<uint32_t, uint32_t> mem;

  void write(uint32_t addr, uint32_t data) {
    mem[addr] = data;
  }

  uint32_t read(uint32_t addr) const {
    auto it = mem.find(addr);
    return (it != mem.end()) ? it->second : 0;
  }

  // Pre-load a range of memory with a pattern
  void load_pattern(uint32_t base_addr, uint32_t count, uint32_t pattern_base) {
    for (uint32_t i = 0; i < count; i++) {
      mem[base_addr + i * 4] = pattern_base + i;
    }
  }

  void clear() { mem.clear(); }
};

#endif // MEMORY_MODEL_H
```

**Step 2: Create AXI4-Lite slave driver**

This driver responds to the DMA master's AXI requests. It watches handshake signals and provides read data from / stores write data to the memory model.

```cpp
// ----------------------------------------------------------------------------
// axi_lite_slave_driver — Responds to AXI4-Lite master requests
//
// Unlike the APB master driver (which initiates transactions), this is a
// slave driver — it reacts to requests from the DMA engine's AXI master
// interface. It uses a memory_model for backing storage.
//
// Handshake protocol:
//   Master asserts xVALID, slave asserts xREADY when it can accept.
//   Transfer occurs on the cycle when both VALID and READY are high.
// ----------------------------------------------------------------------------
#ifndef AXI_LITE_SLAVE_DRIVER_H
#define AXI_LITE_SLAVE_DRIVER_H

#include <systemc>
#include <uvm>
#include "axi_lite_if.h"
#include "memory_model.h"
#include "dma_constants.h"

class axi_lite_slave_driver : public uvm::uvm_component {
public:
  UVM_COMPONENT_UTILS(axi_lite_slave_driver);

  axi_lite_if* vif;
  memory_model* mem;

  axi_lite_slave_driver(uvm::uvm_component_name name)
    : uvm::uvm_component(name), vif(nullptr), mem(nullptr) {}

  void build_phase(uvm::uvm_phase& phase) {
    uvm::uvm_component::build_phase(phase);
  }

  void run_phase(uvm::uvm_phase& phase) {
    // Initialize all slave-driven ready/valid signals
    vif->ARREADY.write(false);
    vif->RVALID.write(false);
    vif->RDATA.write(0);
    vif->RRESP.write(AXI_RESP_OKAY);
    vif->AWREADY.write(false);
    vif->WREADY.write(false);
    vif->BVALID.write(false);
    vif->BRESP.write(AXI_RESP_OKAY);

    // Spawn read and write handlers as parallel SC_THREADs
    // In UVM-SystemC, we handle both in the same loop by polling
    while (true) {
      sc_core::wait(10, sc_core::SC_NS);  // 1 clock cycle

      // ── Handle read requests ──
      if (vif->ARVALID.read() && !vif->RVALID.read()) {
        uint32_t addr = vif->ARADDR.read();
        vif->ARREADY.write(true);
        sc_core::wait(10, sc_core::SC_NS);
        vif->ARREADY.write(false);

        // Provide read data
        vif->RDATA.write(mem->read(addr));
        vif->RRESP.write(AXI_RESP_OKAY);
        vif->RVALID.write(true);

        // Wait for master to accept
        while (!vif->RREADY.read())
          sc_core::wait(10, sc_core::SC_NS);
        sc_core::wait(10, sc_core::SC_NS);
        vif->RVALID.write(false);
      }

      // ── Handle write requests ──
      if (vif->AWVALID.read() && vif->WVALID.read()) {
        uint32_t addr = vif->AWADDR.read();
        uint32_t data = vif->WDATA.read();
        vif->AWREADY.write(true);
        vif->WREADY.write(true);
        sc_core::wait(10, sc_core::SC_NS);
        vif->AWREADY.write(false);
        vif->WREADY.write(false);

        // Store data
        mem->write(addr, data);

        // Send write response
        vif->BRESP.write(AXI_RESP_OKAY);
        vif->BVALID.write(true);

        while (!vif->BREADY.read())
          sc_core::wait(10, sc_core::SC_NS);
        sc_core::wait(10, sc_core::SC_NS);
        vif->BVALID.write(false);
      }
    }
  }
};

#endif // AXI_LITE_SLAVE_DRIVER_H
```

**Step 3: Commit**

```bash
git add projects/dma/env/memory_model.h projects/dma/agents/axi_lite_slave_driver.h
git commit -m "feat(dma): add AXI4-Lite slave driver and memory model"
```

---

### Task 7: Create AXI4-Lite monitor

**Files:**
- Create: `projects/dma/agents/axi_lite_monitor.h`

Follow the exact pattern of `projects/apb/agents/apb_monitor.h` — passively observe, broadcast via analysis port.

**Step 1: Create AXI4-Lite monitor**

```cpp
// ----------------------------------------------------------------------------
// axi_lite_monitor — Passively observes AXI4-Lite bus activity
//
// Watches for completed AXI4-Lite read and write transfers and broadcasts
// observed transactions through an analysis port to the scoreboard.
// Follows the same pattern as apb_monitor.h.
// ----------------------------------------------------------------------------
#ifndef AXI_LITE_MONITOR_H
#define AXI_LITE_MONITOR_H

#include <systemc>
#include <uvm>
#include "axi_lite_transaction.h"
#include "axi_lite_if.h"
#include "dma_constants.h"

class axi_lite_monitor : public uvm::uvm_monitor {
public:
  UVM_COMPONENT_UTILS(axi_lite_monitor);

  uvm::uvm_analysis_port<axi_lite_transaction> ap;
  axi_lite_if* vif;

  axi_lite_monitor(uvm::uvm_component_name name)
    : uvm::uvm_monitor(name), ap("ap"), vif(nullptr) {}

  void run_phase(uvm::uvm_phase& phase) {
    while (true) {
      sc_core::wait(10, sc_core::SC_NS);

      // Detect completed write: BVALID && BREADY handshake
      if (vif->BVALID.read() && vif->BREADY.read()) {
        axi_lite_transaction txn;
        txn.direction = AXI_WRITE;
        txn.addr = vif->AWADDR.read();
        txn.data = vif->WDATA.read();
        txn.resp = vif->BRESP.read();

        UVM_INFO(get_name(), "AXI Observed: " + txn.convert2string(), uvm::UVM_MEDIUM);
        ap.write(txn);
      }

      // Detect completed read: RVALID && RREADY handshake
      if (vif->RVALID.read() && vif->RREADY.read()) {
        axi_lite_transaction txn;
        txn.direction = AXI_READ;
        txn.addr = vif->ARADDR.read();
        txn.data = vif->RDATA.read();
        txn.resp = vif->RRESP.read();

        UVM_INFO(get_name(), "AXI Observed: " + txn.convert2string(), uvm::UVM_MEDIUM);
        ap.write(txn);
      }
    }
  }
};

#endif // AXI_LITE_MONITOR_H
```

**Step 2: Commit**

```bash
git add projects/dma/agents/axi_lite_monitor.h
git commit -m "feat(dma): add AXI4-Lite monitor"
```

---

### Task 8: Create AXI4-Lite slave agent

**Files:**
- Create: `projects/dma/agents/axi_lite_slave_agent.h`

Follow pattern of `projects/apb/agents/apb_master_agent.h` — wires driver + monitor, passes vif.

**Step 1: Create AXI4-Lite slave agent**

```cpp
// ----------------------------------------------------------------------------
// axi_lite_slave_agent — Bundles AXI slave driver + monitor
//
// Unlike the APB master agent (which has a sequencer for driving stimulus),
// the AXI slave agent is reactive — the driver responds to DMA requests
// rather than generating them. No sequencer needed.
// ----------------------------------------------------------------------------
#ifndef AXI_LITE_SLAVE_AGENT_H
#define AXI_LITE_SLAVE_AGENT_H

#include <uvm>
#include "axi_lite_slave_driver.h"
#include "axi_lite_monitor.h"
#include "axi_lite_if.h"
#include "memory_model.h"

class axi_lite_slave_agent : public uvm::uvm_agent {
public:
  UVM_COMPONENT_UTILS(axi_lite_slave_agent);

  axi_lite_slave_driver* driver;
  axi_lite_monitor* monitor;
  axi_lite_if* vif;
  memory_model* mem;

  axi_lite_slave_agent(uvm::uvm_component_name name)
    : uvm::uvm_agent(name), driver(nullptr), monitor(nullptr),
      vif(nullptr), mem(nullptr) {}

  void build_phase(uvm::uvm_phase& phase) {
    uvm::uvm_agent::build_phase(phase);

    if (!uvm::uvm_config_db<axi_lite_if*>::get(this, "", "axi_vif", vif))
      UVM_FATAL(get_name(), "No AXI virtual interface specified for agent");

    // Memory model is shared — get from config_db
    if (!uvm::uvm_config_db<memory_model*>::get(this, "", "mem", mem))
      UVM_FATAL(get_name(), "No memory model specified for agent");

    monitor = axi_lite_monitor::type_id::create("monitor", this);
    driver = axi_lite_slave_driver::type_id::create("driver", this);
  }

  void connect_phase(uvm::uvm_phase& phase) {
    driver->vif = vif;
    driver->mem = mem;
    monitor->vif = vif;
  }
};

#endif // AXI_LITE_SLAVE_AGENT_H
```

**Step 2: Commit**

```bash
git add projects/dma/agents/axi_lite_slave_agent.h
git commit -m "feat(dma): add AXI4-Lite slave agent"
```

---

### Task 9: Create DMA scoreboard

**Files:**
- Create: `projects/dma/env/dma_scoreboard.h`

The scoreboard receives AXI transactions from the monitor and verifies that each write matches the expected source data.

**Step 1: Create DMA scoreboard**

```cpp
// ----------------------------------------------------------------------------
// dma_scoreboard — Verifies DMA transfer correctness
//
// Subscribes to the AXI monitor's analysis port. For each AXI write
// transaction (DMA writing to destination), checks that the written data
// matches what was in the source memory region.
//
// The scoreboard uses its own reference copy of source memory to predict
// expected destination contents.
// ----------------------------------------------------------------------------
#ifndef DMA_SCOREBOARD_H
#define DMA_SCOREBOARD_H

#include <uvm>
#include <sstream>
#include <iomanip>
#include "axi_lite_transaction.h"
#include "memory_model.h"

class dma_scoreboard : public uvm::uvm_scoreboard {
public:
  UVM_COMPONENT_UTILS(dma_scoreboard);

  uvm::uvm_analysis_imp<axi_lite_transaction, dma_scoreboard> analysis_export;

  // Expected transfer parameters (set by test before starting DMA)
  uint32_t exp_src_addr;
  uint32_t exp_dst_addr;
  uint32_t exp_xfer_len;
  memory_model* src_mem;  // reference copy of source memory

  int pass_count;
  int fail_count;
  int write_index;  // tracks which word of the transfer we're on

  dma_scoreboard(uvm::uvm_component_name name)
    : uvm::uvm_scoreboard(name), analysis_export("analysis_export", this),
      exp_src_addr(0), exp_dst_addr(0), exp_xfer_len(0), src_mem(nullptr),
      pass_count(0), fail_count(0), write_index(0) {}

  void write(const axi_lite_transaction& txn) {
    if (txn.direction == AXI_WRITE) {
      // DMA is writing to destination — verify data matches source
      uint32_t expected_addr = exp_dst_addr + write_index * 4;
      uint32_t src_addr_for_word = exp_src_addr + write_index * 4;
      uint32_t expected_data = src_mem ? src_mem->read(src_addr_for_word) : 0;

      std::ostringstream ss;
      ss << "DMA WR[" << write_index << "] addr=0x" << std::hex
         << std::setw(8) << std::setfill('0') << txn.addr
         << " data=0x" << std::setw(8) << std::setfill('0') << txn.data;

      if (txn.addr == expected_addr && txn.data == expected_data) {
        pass_count++;
        UVM_INFO(get_name(), "PASS: " + ss.str(), uvm::UVM_MEDIUM);
      } else {
        fail_count++;
        ss << " | expected addr=0x" << std::setw(8) << std::setfill('0') << expected_addr
           << " data=0x" << std::setw(8) << std::setfill('0') << expected_data;
        UVM_ERROR(get_name(), "FAIL: " + ss.str());
      }
      write_index++;
    }
    // Read transactions are DMA reading from source — we just observe
  }

  void report_phase(uvm::uvm_phase& phase) {
    std::ostringstream ss;
    ss << "DMA Scoreboard: " << pass_count << " passed, " << fail_count << " failed"
       << " (expected " << exp_xfer_len << " writes)";
    if (fail_count > 0 || (uint32_t)pass_count != exp_xfer_len)
      UVM_ERROR(get_name(), ss.str());
    else
      UVM_INFO(get_name(), ss.str(), uvm::UVM_LOW);
  }
};

#endif // DMA_SCOREBOARD_H
```

**Step 2: Commit**

```bash
git add projects/dma/env/dma_scoreboard.h
git commit -m "feat(dma): add DMA scoreboard with transfer verification"
```

---

### Task 10: Create DMA environment

**Files:**
- Create: `projects/dma/env/dma_env.h`

Wires together: APB master agent (reused), AXI slave agent (new), DMA scoreboard.

**Step 1: Create DMA environment**

```cpp
// ----------------------------------------------------------------------------
// dma_env — Top-level UVM environment for DMA verification
//
// Assembles:
//   - APB master agent (reused from projects/apb) — programs DMA registers
//   - AXI4-Lite slave agent (new) — simulates memory for DMA data transfers
//   - DMA scoreboard — verifies transfer correctness
//
// Connects AXI monitor → scoreboard via analysis port.
// ----------------------------------------------------------------------------
#ifndef DMA_ENV_H
#define DMA_ENV_H

#include <uvm>
#include "apb_master_agent.h"
#include "axi_lite_slave_agent.h"
#include "dma_scoreboard.h"
#include "apb_if.h"
#include "axi_lite_if.h"
#include "memory_model.h"

class dma_env : public uvm::uvm_env {
public:
  UVM_COMPONENT_UTILS(dma_env);

  apb_master_agent* apb_agent;
  axi_lite_slave_agent* axi_agent;
  dma_scoreboard* scoreboard;
  memory_model* mem;

  dma_env(uvm::uvm_component_name name)
    : uvm::uvm_env(name), apb_agent(nullptr), axi_agent(nullptr),
      scoreboard(nullptr), mem(nullptr) {}

  void build_phase(uvm::uvm_phase& phase) {
    uvm::uvm_env::build_phase(phase);

    // Create shared memory model
    mem = new memory_model();

    // Pass memory to AXI agent via config_db
    uvm::uvm_config_db<memory_model*>::set(this, "axi_agent.*", "mem", mem);

    apb_agent = apb_master_agent::type_id::create("apb_agent", this);
    axi_agent = axi_lite_slave_agent::type_id::create("axi_agent", this);
    scoreboard = dma_scoreboard::type_id::create("scoreboard", this);
  }

  void connect_phase(uvm::uvm_phase& phase) {
    // Wire AXI monitor → scoreboard
    axi_agent->monitor->ap.connect(scoreboard->analysis_export);
    // Give scoreboard access to source memory for verification
    scoreboard->src_mem = mem;
  }
};

#endif // DMA_ENV_H
```

**Step 2: Commit**

```bash
git add projects/dma/env/dma_env.h
git commit -m "feat(dma): add DMA environment wiring APB agent, AXI agent, and scoreboard"
```

---

### Task 11: Create DMA sequences

**Files:**
- Create: `projects/dma/sequences/dma_base_sequence.h`
- Create: `projects/dma/sequences/dma_xfer_sequence.h`

The DMA sequence programs registers via APB, then triggers and polls the transfer.

**Step 1: Create base sequence**

```cpp
// ----------------------------------------------------------------------------
// dma_base_sequence — Base class for DMA sequences
//
// DMA sequences use the APB sequencer to program DMA registers. They reuse
// apb_transaction as the sequence item type.
// ----------------------------------------------------------------------------
#ifndef DMA_BASE_SEQUENCE_H
#define DMA_BASE_SEQUENCE_H

#include <uvm>
#include "apb_transaction.h"

class dma_base_sequence : public uvm::uvm_sequence<apb_transaction> {
public:
  UVM_OBJECT_UTILS(dma_base_sequence);

  dma_base_sequence(const std::string& name = "dma_base_sequence")
    : uvm::uvm_sequence<apb_transaction>(name) {}

  virtual ~dma_base_sequence() {}

protected:
  // Helper: write a value to a DMA register via APB
  void reg_write(uint32_t addr, uint32_t data) {
    apb_transaction* txn = apb_transaction::type_id::create("txn");
    txn->direction = APB_WRITE;
    txn->addr = addr;
    txn->data = data;
    start_item(txn);
    finish_item(txn);
  }

  // Helper: read a DMA register via APB, returns read data
  uint32_t reg_read(uint32_t addr) {
    apb_transaction* txn = apb_transaction::type_id::create("txn");
    txn->direction = APB_READ;
    txn->addr = addr;
    start_item(txn);
    finish_item(txn);
    return txn->data;
  }
};

#endif // DMA_BASE_SEQUENCE_H
```

**Step 2: Create transfer sequence**

```cpp
// ----------------------------------------------------------------------------
// dma_xfer_sequence — Programs DMA and executes a memory-to-memory transfer
//
// 1. Programs SRC_ADDR, DST_ADDR, XFER_LEN registers
// 2. Writes CTRL with start bit to trigger transfer
// 3. Polls STATUS register until done
// ----------------------------------------------------------------------------
#ifndef DMA_XFER_SEQUENCE_H
#define DMA_XFER_SEQUENCE_H

#include <uvm>
#include <sstream>
#include <iomanip>
#include "dma_base_sequence.h"
#include "dma_constants.h"

class dma_xfer_sequence : public dma_base_sequence {
public:
  UVM_OBJECT_UTILS(dma_xfer_sequence);

  // Transfer parameters — set by the test before starting
  uint32_t src_addr;
  uint32_t dst_addr;
  uint32_t xfer_len;
  bool     irq_en;

  dma_xfer_sequence(const std::string& name = "dma_xfer_sequence")
    : dma_base_sequence(name), src_addr(0), dst_addr(0),
      xfer_len(0), irq_en(false) {}

  void body() {
    std::ostringstream ss;
    ss << "DMA transfer: src=0x" << std::hex << src_addr
       << " dst=0x" << dst_addr << " len=" << std::dec << xfer_len;
    UVM_INFO("DMA_SEQ", ss.str(), uvm::UVM_LOW);

    // Program registers
    reg_write(DMA_SRC_ADDR, src_addr);
    reg_write(DMA_DST_ADDR, dst_addr);
    reg_write(DMA_XFER_LEN, xfer_len);

    // Trigger transfer
    uint32_t ctrl_val = DMA_CTRL_START;
    if (irq_en) ctrl_val |= DMA_CTRL_IRQ_EN;
    reg_write(DMA_CTRL, ctrl_val);

    // Poll status until done
    uint32_t status = 0;
    int timeout = 1000;  // max poll iterations
    do {
      status = reg_read(DMA_STATUS);
      timeout--;
    } while (!(status & DMA_STATUS_DONE) && timeout > 0);

    if (timeout == 0) {
      UVM_ERROR("DMA_SEQ", "Transfer timed out — DMA never completed");
    } else if (status & DMA_STATUS_ERROR) {
      UVM_ERROR("DMA_SEQ", "Transfer completed with error");
    } else {
      UVM_INFO("DMA_SEQ", "Transfer completed successfully", uvm::UVM_LOW);
    }
  }
};

#endif // DMA_XFER_SEQUENCE_H
```

**Step 3: Commit**

```bash
git add projects/dma/sequences/dma_base_sequence.h projects/dma/sequences/dma_xfer_sequence.h
git commit -m "feat(dma): add DMA transfer sequences with register helpers"
```

---

### Task 12: Create DMA tests

**Files:**
- Create: `projects/dma/test/dma_base_test.h`
- Create: `projects/dma/test/dma_simple_xfer_test.h`

**Step 1: Create base test**

```cpp
// ----------------------------------------------------------------------------
// dma_base_test — Base class for all DMA tests
//
// Creates the DMA environment, retrieves virtual interfaces from config_db.
// Concrete tests extend this and add stimulus in run_phase.
// ----------------------------------------------------------------------------
#ifndef DMA_BASE_TEST_H
#define DMA_BASE_TEST_H

#include <systemc>
#include <uvm>
#include "dma_env.h"
#include "apb_if.h"
#include "axi_lite_if.h"

class dma_base_test : public uvm::uvm_test {
public:
  UVM_COMPONENT_UTILS(dma_base_test);

  dma_env* env;
  apb_if* apb_vif;
  axi_lite_if* axi_vif;

  dma_base_test(uvm::uvm_component_name name)
    : uvm::uvm_test(name), env(nullptr), apb_vif(nullptr), axi_vif(nullptr) {}

  void build_phase(uvm::uvm_phase& phase) {
    uvm::uvm_test::build_phase(phase);
    env = dma_env::type_id::create("env", this);

    if (!uvm::uvm_config_db<apb_if*>::get(this, "", "vif", apb_vif))
      UVM_FATAL(get_name(), "Failed to get APB interface from config_db");

    if (!uvm::uvm_config_db<axi_lite_if*>::get(this, "", "axi_vif", axi_vif))
      UVM_FATAL(get_name(), "Failed to get AXI interface from config_db");
  }
};

#endif // DMA_BASE_TEST_H
```

**Step 2: Create simple transfer test**

```cpp
// ----------------------------------------------------------------------------
// dma_simple_xfer_test — Basic memory-to-memory DMA transfer test
//
// 1. Pre-loads source memory with known pattern (0xDEAD0000 + i)
// 2. Programs DMA: src=0x1000, dst=0x2000, len=8
// 3. Triggers transfer and waits for completion
// 4. Scoreboard automatically verifies each AXI write
// ----------------------------------------------------------------------------
#ifndef DMA_SIMPLE_XFER_TEST_H
#define DMA_SIMPLE_XFER_TEST_H

#include <uvm>
#include "dma_base_test.h"
#include "dma_xfer_sequence.h"

class dma_simple_xfer_test : public dma_base_test {
public:
  UVM_COMPONENT_UTILS(dma_simple_xfer_test);

  dma_simple_xfer_test(uvm::uvm_component_name name)
    : dma_base_test(name) {}

  void run_phase(uvm::uvm_phase& phase) {
    phase.raise_objection(this, "dma_simple_xfer_test");

    // Wait for reset
    sc_core::sc_signal<bool>* resetn = nullptr;
    if (uvm::uvm_config_db<sc_core::sc_signal<bool>*>::get(this, "", "resetn", resetn)) {
      while (!resetn->read())
        sc_core::wait(10, sc_core::SC_NS);
      sc_core::wait(10, sc_core::SC_NS);
    }

    // Test parameters
    uint32_t src_addr = 0x1000;
    uint32_t dst_addr = 0x2000;
    uint32_t xfer_len = 8;

    // Pre-load source memory with test pattern
    env->mem->load_pattern(src_addr, xfer_len, 0xDEAD0000);

    // Configure scoreboard expectations
    env->scoreboard->exp_src_addr = src_addr;
    env->scoreboard->exp_dst_addr = dst_addr;
    env->scoreboard->exp_xfer_len = xfer_len;

    // Run DMA transfer sequence
    dma_xfer_sequence* seq = dma_xfer_sequence::type_id::create("seq");
    seq->src_addr = src_addr;
    seq->dst_addr = dst_addr;
    seq->xfer_len = xfer_len;
    seq->start(env->apb_agent->sequencer);

    // Allow monitor to observe final transactions
    sc_core::wait(50, sc_core::SC_NS);

    phase.drop_objection(this, "dma_simple_xfer_test");
  }
};

#endif // DMA_SIMPLE_XFER_TEST_H
```

**Step 3: Commit**

```bash
git add projects/dma/test/dma_base_test.h projects/dma/test/dma_simple_xfer_test.h
git commit -m "feat(dma): add DMA base test and simple transfer test"
```

---

### Task 13: Create top-level testbench

**Files:**
- Create: `projects/dma/top/top.cpp`

This is the sc_main that wires everything together: clock, reset, DUT, APB interface, AXI interface, VCD tracing, and UVM launch.

**Step 1: Create top.cpp**

```cpp
// ----------------------------------------------------------------------------
// top.cpp — DMA engine testbench top-level
//
// Wires together: clock, reset, DUT (SystemC or Verilated), APB interface,
// AXI4-Lite interface, VCD tracing, and UVM test infrastructure.
//
// DUT selection:
//   -DUSE_RTL=ON  → Verilator-compiled Verilog RTL (Vdma_engine)
//   default       → SystemC behavioral model (dma_engine)
// ----------------------------------------------------------------------------
#include <systemc>
#include <uvm>

#ifdef USE_RTL
#include "Vdma_engine.h"
#else
#include "dma_engine.h"
#endif

#include "apb_if.h"
#include "axi_lite_if.h"
#include "dma_simple_xfer_test.h"

// Reset generator — holds PRESETn low for 2 clock cycles
SC_MODULE(reset_gen) {
  sc_core::sc_in<bool> clk;
  sc_core::sc_out<bool> resetn;

  SC_CTOR(reset_gen) {
    SC_THREAD(run);
    sensitive << clk.pos();
  }

  void run() {
    resetn.write(false);
    wait();
    wait();
    resetn.write(true);
  }
};

int sc_main(int argc, char* argv[]) {
  // Clock: 10ns period (100 MHz)
  sc_core::sc_clock clk("clk", 10, sc_core::SC_NS);
  sc_core::sc_signal<bool> resetn;

  // APB signal interface (DMA register programming)
  apb_if apb("apb");

  // AXI4-Lite signal interface (DMA data transfers)
  axi_lite_if axi("axi");

  // Interrupt signal
  sc_core::sc_signal<bool> irq;

  // DUT — swap between SystemC model and RTL via -DUSE_RTL=ON
#ifdef USE_RTL
  Vdma_engine dut("dut");
#else
  dma_engine dut("dut");
#endif

  // Port binding — APB slave interface
  dut.PCLK(clk);
  dut.PRESETn(resetn);
  dut.PSEL(apb.PSEL);
  dut.PENABLE(apb.PENABLE);
  dut.PWRITE(apb.PWRITE);
  dut.PADDR(apb.PADDR);
  dut.PWDATA(apb.PWDATA);
  dut.PRDATA(apb.PRDATA);
  dut.PREADY(apb.PREADY);
  dut.PSLVERR(apb.PSLVERR);

  // Port binding — AXI4-Lite master interface
  dut.AWADDR(axi.AWADDR);
  dut.AWVALID(axi.AWVALID);
  dut.AWREADY(axi.AWREADY);
  dut.WDATA(axi.WDATA);
  dut.WSTRB(axi.WSTRB);
  dut.WVALID(axi.WVALID);
  dut.WREADY(axi.WREADY);
  dut.BRESP(axi.BRESP);
  dut.BVALID(axi.BVALID);
  dut.BREADY(axi.BREADY);
  dut.ARADDR(axi.ARADDR);
  dut.ARVALID(axi.ARVALID);
  dut.ARREADY(axi.ARREADY);
  dut.RDATA(axi.RDATA);
  dut.RRESP(axi.RRESP);
  dut.RVALID(axi.RVALID);
  dut.RREADY(axi.RREADY);
  dut.IRQ(irq);

  // Reset generator
  reset_gen rst("rst");
  rst.clk(clk);
  rst.resetn(resetn);

  // VCD waveform tracing
  sc_core::sc_trace_file* wave = sc_core::sc_create_vcd_trace_file("dma_engine");
  wave->set_time_unit(1, sc_core::SC_NS);

  // Trace clock and reset
  sc_core::sc_trace(wave, clk, "clk");
  sc_core::sc_trace(wave, resetn, "resetn");
  sc_core::sc_trace(wave, irq, "IRQ");

  // Trace APB signals
  sc_core::sc_trace(wave, apb.PSEL, "PSEL");
  sc_core::sc_trace(wave, apb.PENABLE, "PENABLE");
  sc_core::sc_trace(wave, apb.PWRITE, "PWRITE");
  sc_core::sc_trace(wave, apb.PADDR, "PADDR");
  sc_core::sc_trace(wave, apb.PWDATA, "PWDATA");
  sc_core::sc_trace(wave, apb.PRDATA, "PRDATA");
  sc_core::sc_trace(wave, apb.PREADY, "PREADY");

  // Trace AXI signals
  sc_core::sc_trace(wave, axi.ARADDR, "ARADDR");
  sc_core::sc_trace(wave, axi.ARVALID, "ARVALID");
  sc_core::sc_trace(wave, axi.ARREADY, "ARREADY");
  sc_core::sc_trace(wave, axi.RDATA, "RDATA");
  sc_core::sc_trace(wave, axi.RVALID, "RVALID");
  sc_core::sc_trace(wave, axi.RREADY, "RREADY");
  sc_core::sc_trace(wave, axi.AWADDR, "AWADDR");
  sc_core::sc_trace(wave, axi.AWVALID, "AWVALID");
  sc_core::sc_trace(wave, axi.AWREADY, "AWREADY");
  sc_core::sc_trace(wave, axi.WDATA, "WDATA");
  sc_core::sc_trace(wave, axi.WVALID, "WVALID");
  sc_core::sc_trace(wave, axi.WREADY, "WREADY");
  sc_core::sc_trace(wave, axi.BVALID, "BVALID");
  sc_core::sc_trace(wave, axi.BREADY, "BREADY");

  // Pass interfaces to UVM components via config_db
  uvm::uvm_config_db<apb_if*>::set(nullptr, "*", "vif", &apb);
  uvm::uvm_config_db<axi_lite_if*>::set(nullptr, "*", "axi_vif", &axi);
  uvm::uvm_config_db<sc_core::sc_signal<bool>*>::set(nullptr, "*", "resetn", &resetn);

  // Launch UVM test
  uvm::run_test("dma_simple_xfer_test");

  sc_core::sc_close_vcd_trace_file(wave);
  return 0;
}
```

**Step 2: Commit**

```bash
git add projects/dma/top/top.cpp
git commit -m "feat(dma): add top-level testbench with DUT binding and UVM launch"
```

---

### Task 14: Build and run with SystemC model

**Step 1: Clean .gitkeep placeholder files**

```bash
find projects/dma -name ".gitkeep" -delete
```

**Step 2: Build**

Run: `cd build && cmake .. -DSYSTEMC_HOME=$SYSTEMC_HOME -DUVM_SYSTEMC_HOME=$UVM_SYSTEMC_HOME && make dma_sim`

Expected: Clean compilation with no errors.

**Step 3: Run simulation**

Run: `./projects/dma/dma_sim`

Expected output should include:
- UVM_INFO messages showing APB register writes
- UVM_INFO messages showing AXI read/write transactions
- Scoreboard: "8 passed, 0 failed (expected 8 writes)"
- `dma_engine.vcd` waveform file generated

**Step 4: Fix any compilation or simulation errors**

If there are errors, diagnose and fix them. Common issues:
- Include path ordering in CMake
- Signal type mismatches between SystemC and Verilator
- Timing issues in the AXI handshake (adjust wait cycles)

**Step 5: Commit fixes if needed**

```bash
git add -u
git commit -m "fix(dma): resolve build/simulation issues"
```

---

### Task 15: Build and run with Verilator co-simulation

**Step 1: Configure with USE_RTL**

Run: `cd build && cmake .. -DSYSTEMC_HOME=$SYSTEMC_HOME -DUVM_SYSTEMC_HOME=$UVM_SYSTEMC_HOME -DUSE_RTL=ON && make dma_sim`

Expected: Verilator compiles `dma_engine.v` into `Vdma_engine.h`, then the testbench compiles and links.

**Step 2: Run co-simulation**

Run: `./projects/dma/dma_sim`

Expected: Identical pass/fail results as the SystemC model — scoreboard should show "8 passed, 0 failed".

**Step 3: Compare VCD waveforms**

Open both `dma_engine.vcd` files (from SystemC and Verilated runs) in GTKWave and verify the signal activity matches.

**Step 4: Fix any Verilator-specific issues**

Common issues:
- Verilator treats `reg [31:0]` differently than `sc_signal<uint32_t>` — may need type casts
- Clock/reset timing differences between behavioral and RTL
- Verilator may optimize away signals — check `--trace` flag

**Step 5: Commit**

```bash
git add -u
git commit -m "feat(dma): verify Verilator co-simulation passes all tests"
```

---

### Task 16: Update README and project documentation

**Files:**
- Modify: `README.md`

**Step 1: Add DMA section to README**

Add after the existing APB section in the learning progression:

```markdown
### DMA Engine (projects/dma/)

A single-channel DMA engine demonstrating multi-protocol IP design:
- **APB3 slave** for CPU register programming (5 control/status registers)
- **AXI4-Lite master** for memory-to-memory data transfers
- Full UVM-SystemC testbench with AXI slave agent and scoreboard
- Verilator co-simulation support

```bash
# Build and run with SystemC model
cd build
cmake .. -DSYSTEMC_HOME=/path/to/systemc -DUVM_SYSTEMC_HOME=/path/to/uvm-systemc
make dma_sim
./projects/dma/dma_sim

# Build and run with Verilator (RTL co-simulation)
cmake .. -DSYSTEMC_HOME=/path/to/systemc -DUVM_SYSTEMC_HOME=/path/to/uvm-systemc -DUSE_RTL=ON
make dma_sim
./projects/dma/dma_sim
```
```

**Step 2: Add DMA to project structure section in README**

**Step 3: Commit**

```bash
git add README.md
git commit -m "docs: add DMA engine section to README"
```

---

## Summary

| Task | Component | Files |
|------|-----------|-------|
| 1 | Directory + CMake | `CMakeLists.txt` x2 |
| 2 | AXI interface + constants | `axi_lite_if.h`, `dma_constants.h` |
| 3 | AXI transaction | `axi_lite_transaction.h` |
| 4 | SystemC DMA model | `dma_engine.h` |
| 5 | Verilog DMA RTL | `dma_engine.v` |
| 6 | AXI slave driver + memory | `axi_lite_slave_driver.h`, `memory_model.h` |
| 7 | AXI monitor | `axi_lite_monitor.h` |
| 8 | AXI slave agent | `axi_lite_slave_agent.h` |
| 9 | DMA scoreboard | `dma_scoreboard.h` |
| 10 | DMA environment | `dma_env.h` |
| 11 | DMA sequences | `dma_base_sequence.h`, `dma_xfer_sequence.h` |
| 12 | DMA tests | `dma_base_test.h`, `dma_simple_xfer_test.h` |
| 13 | Top-level testbench | `top.cpp` |
| 14 | Build + run (SystemC) | verify compilation and simulation |
| 15 | Build + run (Verilator) | verify co-simulation |
| 16 | Documentation | `README.md` |

**Total new files:** ~18 header/source files + 1 Verilog file
**Reused from APB:** 7 files (apb_if, apb_transaction, apb_constants, apb_master_driver, apb_monitor, apb_master_agent, apb_sequencer)
