// ----------------------------------------------------------------------------
// axi_lite_monitor — Passively observes AXI4-Lite bus activity
//
// The monitor watches for completed AXI4-Lite transfers (handshake completions
// on the response channels) and broadcasts observed transactions through an
// analysis port. The scoreboard subscribes to this port to check data integrity.
//
// UVM-SV equivalent:
//   class axi_lite_monitor extends uvm_monitor;
//     uvm_analysis_port #(axi_lite_transaction) ap;
//     task run_phase(uvm_phase phase);
//       // sample signals, then: ap.write(txn);
//     endtask
//   endclass
//
// Detection strategy:
//   Write complete: BVALID && BREADY  — sample AWADDR, WDATA, BRESP
//   Read complete:  RVALID && RREADY  — sample ARADDR, RDATA, RRESP
//
// The monitor is purely passive — it never drives signals.
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

  // Analysis port — the publish side of the observer pattern.
  // Any number of subscribers (scoreboards, coverage collectors, loggers) can
  // connect to this port. When we call ap.write(txn), all subscribers receive
  // a copy of the transaction. This is UVM's primary data-flow mechanism.
  //
  // SV equivalent: uvm_analysis_port #(axi_lite_transaction) ap;
  uvm::uvm_analysis_port<axi_lite_transaction> ap;
  axi_lite_if* vif;

  axi_lite_monitor(uvm::uvm_component_name name)
    : uvm::uvm_monitor(name), ap("ap"), vif(nullptr) {}

  void run_phase(uvm::uvm_phase& phase) {
    while (true) {
      sc_core::wait(10, sc_core::SC_NS);

      // *** CRITICAL SystemC gotcha: delta cycle alignment ***
      //
      // SystemC processes at the same simulation time execute in delta cycles:
      //   Delta 0: SC_THREADs resume from wait(time), sc_clock signal updates
      //   Delta 1: SC_METHODs triggered by clock events (posedge) fire
      //
      // The slave driver (SC_THREAD) writes BVALID=true at delta 0.
      // The DMA FSM (SC_METHOD) reads BVALID and writes BREADY=false at delta 1.
      //
      // If we sample at delta 0, we see stale signal values (before the
      // current cycle's updates). By advancing one extra delta with
      // wait(SC_ZERO_TIME), we sample at delta 1+ where the signals
      // reflect the completed handshake from the current clock edge.
      //
      // This is one of the most common pitfalls when mixing SC_METHOD and
      // SC_THREAD in the same testbench. In production, you'd typically use
      // a clocking block (SV) or event-based sampling to avoid this issue.
      sc_core::wait(sc_core::SC_ZERO_TIME);

      // Detect completed write: both BVALID and BREADY high = write response
      if (vif->BVALID.read() && vif->BREADY.read()) {
        axi_lite_transaction txn;
        txn.direction = AXI_WRITE;
        txn.addr      = vif->AWADDR.read();  // Still holds address from AW phase
        txn.data      = vif->WDATA.read();   // Still holds data from W phase
        txn.resp      = vif->BRESP.read();

        UVM_INFO(get_name(), "Observed: " + txn.convert2string(), uvm::UVM_MEDIUM);
        ap.write(txn);  // Broadcast to all connected subscribers
      }

      // Detect completed read: both RVALID and RREADY high = read data transfer
      if (vif->RVALID.read() && vif->RREADY.read()) {
        axi_lite_transaction txn;
        txn.direction = AXI_READ;
        txn.addr      = vif->ARADDR.read();
        txn.data      = vif->RDATA.read();
        txn.resp      = vif->RRESP.read();

        UVM_INFO(get_name(), "Observed: " + txn.convert2string(), uvm::UVM_MEDIUM);
        ap.write(txn);
      }
    }
  }
};

#endif // AXI_LITE_MONITOR_H
