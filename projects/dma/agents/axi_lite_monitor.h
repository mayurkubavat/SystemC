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
  axi_lite_if*       vif;
  sc_core::sc_clock* clk;

  axi_lite_monitor(uvm::uvm_component_name name)
    : uvm::uvm_monitor(name), ap("ap"), vif(nullptr), clk(nullptr) {}

  void run_phase(uvm::uvm_phase& phase) {
    // Retrieve clock for posedge-event waits
    if (!uvm::uvm_config_db<sc_core::sc_clock*>::get(this, "", "clk", clk))
      UVM_FATAL(get_name(), "No clock found in config_db");

    while (true) {
      // Using posedge_event() naturally wakes us AFTER the clock update
      // (delta 1+), so we see signal values that reflect the current clock
      // edge. This eliminates the need for the SC_ZERO_TIME hack that was
      // previously required with time-based waits.
      sc_core::wait(clk->posedge_event());

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
