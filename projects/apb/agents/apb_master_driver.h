// ----------------------------------------------------------------------------
// apb_master_driver — Drives APB protocol onto the bus signals
//
// The driver receives transactions from the sequencer via a TLM port
// (seq_item_port) and translates them into pin-level APB signal activity.
//
// UVM-SV equivalent:
//   class apb_master_driver extends uvm_driver #(apb_transaction);
//     virtual apb_if vif;
//     task run_phase(uvm_phase phase);
//       seq_item_port.get_next_item(txn);
//       // drive signals...
//       seq_item_port.item_done();
//     endtask
//   endclass
//
// Key SystemC differences:
//   - `run_phase` is a regular C++ method, not a task
//   - Timing uses sc_core::wait(time, unit) instead of `#delay` or `@(posedge)`
//   - Signals are driven with .write() and sampled with .read()
// ----------------------------------------------------------------------------
#ifndef APB_MASTER_DRIVER_H
#define APB_MASTER_DRIVER_H

#include <systemc>
#include <uvm>
#include "apb_transaction.h"
#include "apb_if.h"

class apb_master_driver : public uvm::uvm_driver<apb_transaction> {
public:
  UVM_COMPONENT_UTILS(apb_master_driver);

  apb_if* vif;  // pointer to signal interface (like SV virtual interface)

  apb_master_driver(uvm::uvm_component_name name)
    : uvm::uvm_driver<apb_transaction>(name), vif(nullptr) {}

  void build_phase(uvm::uvm_phase& phase) {
    uvm::uvm_driver<apb_transaction>::build_phase(phase);
  }

  void run_phase(uvm::uvm_phase& phase) {
    // Initialize bus to idle
    vif->PSEL.write(false);
    vif->PENABLE.write(false);

    // Main driver loop — get transactions from sequencer, drive them
    while (true) {
      apb_transaction txn;
      seq_item_port->get_next_item(txn);
      drive_transfer(txn);
      seq_item_port->item_done();
    }
  }

private:
  // Drive a single APB transfer (SETUP → ACCESS → IDLE)
  void drive_transfer(apb_transaction& txn) {
    // SETUP phase: assert PSEL, set address/direction/data
    vif->PSEL.write(true);
    vif->PENABLE.write(false);
    vif->PADDR.write(txn.addr);
    vif->PWRITE.write(txn.direction == APB_WRITE);
    if (txn.direction == APB_WRITE)
      vif->PWDATA.write(txn.data);

    sc_core::wait(10, sc_core::SC_NS);  // 1 clock cycle

    // ACCESS phase: assert PENABLE for one cycle
    vif->PENABLE.write(true);
    sc_core::wait(10, sc_core::SC_NS);

    // Capture response from slave
    if (txn.direction == APB_READ)
      txn.data = vif->PRDATA.read();
    txn.error = vif->PSLVERR.read();

    // Return bus to IDLE
    vif->PSEL.write(false);
    vif->PENABLE.write(false);
  }
};

#endif // APB_MASTER_DRIVER_H
