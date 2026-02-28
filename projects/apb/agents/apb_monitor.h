// ----------------------------------------------------------------------------
// apb_monitor — Passively observes APB bus activity
//
// The monitor watches for completed APB transfers (PSEL && PENABLE) and
// broadcasts observed transactions through an analysis port. The scoreboard
// subscribes to this port and checks data integrity.
//
// UVM-SV equivalent:
//   class apb_monitor extends uvm_monitor;
//     uvm_analysis_port #(apb_transaction) ap;
//     task run_phase(uvm_phase phase);
//       // sample signals, then: ap.write(txn);
//     endtask
//   endclass
//
// The monitor is purely passive — it never drives signals.
// ----------------------------------------------------------------------------
#ifndef APB_MONITOR_H
#define APB_MONITOR_H

#include <systemc>
#include <uvm>
#include "apb_transaction.h"
#include "apb_if.h"

class apb_monitor : public uvm::uvm_monitor {
public:
  UVM_COMPONENT_UTILS(apb_monitor);

  // Analysis port — broadcasts observed transactions to subscribers
  uvm::uvm_analysis_port<apb_transaction> ap;
  apb_if* vif;

  apb_monitor(uvm::uvm_component_name name)
    : uvm::uvm_monitor(name), ap("ap"), vif(nullptr) {}

  void run_phase(uvm::uvm_phase& phase) {
    while (true) {
      // Wait for ACCESS phase (PSEL && PENABLE both high)
      while (!(vif->PSEL.read() && vif->PENABLE.read()))
        sc_core::wait(10, sc_core::SC_NS);

      // Sample bus signals into a transaction
      apb_transaction txn;
      txn.addr = vif->PADDR.read();
      txn.direction = vif->PWRITE.read() ? APB_WRITE : APB_READ;
      txn.error = vif->PSLVERR.read();

      if (txn.direction == APB_WRITE)
        txn.data = vif->PWDATA.read();
      else
        txn.data = vif->PRDATA.read();

      UVM_INFO(get_name(), "Observed: " + txn.convert2string(), uvm::UVM_MEDIUM);

      // Broadcast to all subscribers (scoreboard, coverage, etc.)
      ap.write(txn);

      sc_core::wait(10, sc_core::SC_NS);
    }
  }
};

#endif // APB_MONITOR_H
