// ----------------------------------------------------------------------------
// axi_lite_slave_driver — Reactive slave that responds to AXI4-Lite requests
//
// Unlike the APB master driver (which initiates transactions from a sequencer),
// this driver is a **reactive slave** — it watches for AXI requests from the
// DMA engine and responds with read data or write acknowledgements.
//
// Because there is no sequencer driving it, it extends uvm_component rather
// than uvm_driver. The run_phase loop continuously monitors the AXI signals
// and services read/write requests using the backing memory model.
//
// UVM-SV equivalent:
//   class axi_lite_slave_driver extends uvm_component;
//     virtual axi_lite_if vif;
//     memory_model mem;
//     task run_phase(uvm_phase phase);
//       // react to ARVALID / AWVALID+WVALID...
//     endtask
//   endclass
//
// Key difference from a master driver:
//   Master driver: pulls transactions from sequencer, drives onto bus
//   Slave driver:  watches bus for requests, responds with data/ack
// ----------------------------------------------------------------------------
#ifndef AXI_LITE_SLAVE_DRIVER_H
#define AXI_LITE_SLAVE_DRIVER_H

#include <systemc>
#include <uvm>
#include "axi_lite_if.h"
#include "memory_model.h"
#include "dma_constants.h"

// Extends uvm_component (not uvm_driver) because there's no sequencer.
// In UVM-SV, you might extend uvm_driver and use `seq_item_port` to pull
// transactions. A reactive slave doesn't pull — it reacts to what's on the bus.
class axi_lite_slave_driver : public uvm::uvm_component {
public:
  // UVM_COMPONENT_UTILS registers this with the UVM factory.
  // Components use UVM_COMPONENT_UTILS; data objects use UVM_OBJECT_UTILS.
  UVM_COMPONENT_UTILS(axi_lite_slave_driver);

  axi_lite_if*   vif;  // Pointer to AXI signal-level interface (virtual interface)
  memory_model*  mem;  // Pointer to backing memory (shared with scoreboard)

  // UVM-SystemC constructors take uvm_component_name (not string).
  // This is different from SV where you pass `string name, uvm_component parent`.
  // In UVM-SystemC, the parent is implicit from factory creation context.
  axi_lite_slave_driver(uvm::uvm_component_name name)
    : uvm::uvm_component(name), vif(nullptr), mem(nullptr) {}

  void build_phase(uvm::uvm_phase& phase) {
    uvm::uvm_component::build_phase(phase);
  }

  // run_phase — the main simulation-time process.
  // In UVM-SystemC, run_phase runs as an SC_THREAD (it can call wait()).
  // This is different from build_phase/connect_phase which are regular functions.
  void run_phase(uvm::uvm_phase& phase) {
    // Initialize all slave-driven signals to idle state.
    // In AXI, the slave drives: ARREADY, RVALID, RDATA, RRESP,
    //                            AWREADY, WREADY, BVALID, BRESP
    vif->ARREADY.write(false);
    vif->RVALID.write(false);
    vif->RDATA.write(0);
    vif->RRESP.write(AXI_RESP_OKAY);
    vif->AWREADY.write(false);
    vif->WREADY.write(false);
    vif->BVALID.write(false);
    vif->BRESP.write(AXI_RESP_OKAY);

    // Main loop — polls every clock cycle (10ns = one period of 100MHz clock).
    // A more robust approach would wait on signal events, but polling at clock
    // frequency is simpler and sufficient for this learning example.
    while (true) {
      sc_core::wait(10, sc_core::SC_NS);

      // ---- Read channel handling ----
      // Respond when master asserts ARVALID and we're not already serving a read
      if (vif->ARVALID.read() && !vif->RVALID.read()) {
        handle_read();
      }

      // ---- Write channel handling ----
      // Respond when both AWVALID and WVALID are asserted (address + data ready)
      if (vif->AWVALID.read() && vif->WVALID.read()) {
        handle_write();
      }
    }
  }

private:
  // Service a single AXI read request.
  //
  // AXI4-Lite read handshake (2 phases):
  //   Phase 1 — Address:  Master drives ARADDR+ARVALID, slave asserts ARREADY
  //   Phase 2 — Data:     Slave drives RDATA+RVALID, master asserts RREADY
  //
  // Timeline (each step = one wait(10ns) = one clock cycle):
  //   [1] Slave sees ARVALID → captures address, asserts ARREADY
  //   [2] Slave deasserts ARREADY, drives RDATA+RVALID from memory
  //   [3] Master asserts RREADY → data transfer complete
  //   [4] Slave deasserts RVALID
  void handle_read() {
    uint32_t addr = vif->ARADDR.read();

    // Accept the address (ARREADY high for 1 cycle)
    vif->ARREADY.write(true);
    sc_core::wait(10, sc_core::SC_NS);
    vif->ARREADY.write(false);

    // Read from backing memory and present data
    uint32_t data = mem->read(addr);
    vif->RDATA.write(data);
    vif->RRESP.write(AXI_RESP_OKAY);
    vif->RVALID.write(true);

    // Wait for master's RREADY (may already be asserted)
    while (!vif->RREADY.read())
      sc_core::wait(10, sc_core::SC_NS);

    // Hold RVALID for 1 more cycle so the monitor can observe it
    sc_core::wait(10, sc_core::SC_NS);
    vif->RVALID.write(false);
  }

  // Service a single AXI write request.
  //
  // AXI4-Lite write handshake (3 phases):
  //   Phase 1 — Address+Data: Master drives AW+W channels, slave asserts READY
  //   Phase 2 — Response:     Slave drives BVALID+BRESP, master asserts BREADY
  //
  // Note: AXI4-Lite allows AW and W to complete simultaneously (unlike full
  // AXI4 where they can be independent). We accept both in the same cycle.
  void handle_write() {
    uint32_t addr = vif->AWADDR.read();
    uint32_t data = vif->WDATA.read();

    // Accept address + data (both READY signals high for 1 cycle)
    vif->AWREADY.write(true);
    vif->WREADY.write(true);
    sc_core::wait(10, sc_core::SC_NS);
    vif->AWREADY.write(false);
    vif->WREADY.write(false);

    // Commit write to backing memory
    mem->write(addr, data);

    // Send write response
    vif->BRESP.write(AXI_RESP_OKAY);
    vif->BVALID.write(true);

    // Wait for master's BREADY (may already be asserted)
    while (!vif->BREADY.read())
      sc_core::wait(10, sc_core::SC_NS);

    // Hold BVALID for 1 more cycle so the monitor can observe it
    sc_core::wait(10, sc_core::SC_NS);
    vif->BVALID.write(false);
  }
};

#endif // AXI_LITE_SLAVE_DRIVER_H
