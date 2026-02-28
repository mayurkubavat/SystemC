// ----------------------------------------------------------------------------
// apb_scoreboard — Reference model and checker
//
// The scoreboard maintains a shadow copy of the register file. It receives
// observed transactions from the monitor via an analysis port (TLM) and
// checks read data against expected values.
//
// UVM-SV equivalent:
//   class apb_scoreboard extends uvm_scoreboard;
//     uvm_analysis_imp #(apb_transaction, apb_scoreboard) analysis_export;
//     function void write(apb_transaction t); ... endfunction
//   endclass
//
// In UVM-SystemC, `uvm_analysis_imp` works the same way — it calls the
// `write()` method whenever the monitor broadcasts a transaction.
// ----------------------------------------------------------------------------
#ifndef APB_SCOREBOARD_H
#define APB_SCOREBOARD_H

#include <uvm>
#include "apb_transaction.h"

class apb_scoreboard : public uvm::uvm_scoreboard {
public:
  UVM_COMPONENT_UTILS(apb_scoreboard);

  // Analysis imp — connects to monitor's analysis port
  uvm::uvm_analysis_imp<apb_transaction, apb_scoreboard> analysis_export;

  // Shadow register file (reference model)
  static const int NUM_REGS = 16;
  uint32_t expected_regs[NUM_REGS];
  int pass_count;
  int fail_count;

  apb_scoreboard(uvm::uvm_component_name name)
    : uvm::uvm_scoreboard(name), analysis_export("analysis_export", this),
      pass_count(0), fail_count(0) {
    for (int i = 0; i < NUM_REGS; i++)
      expected_regs[i] = 0;
  }

  // write() — called automatically each time the monitor broadcasts a txn
  void write(const apb_transaction& txn) {
    unsigned int addr = txn.addr;

    if (txn.error) {
      UVM_INFO(get_name(), "Error response observed (expected for bad addr)", uvm::UVM_MEDIUM);
      return;
    }

    if ((addr & 0x3) != 0 || addr > (NUM_REGS - 1) * 4) {
      UVM_WARNING(get_name(), "Out-of-range address without error — unexpected");
      return;
    }

    unsigned int idx = addr >> 2;

    if (txn.direction == APB_WRITE) {
      // Update shadow model on writes
      expected_regs[idx] = txn.data;
      UVM_INFO(get_name(), "Model write: " + const_cast<apb_transaction&>(txn).convert2string(), uvm::UVM_HIGH);
    } else {
      // Compare read data against expected
      if (txn.data == expected_regs[idx]) {
        pass_count++;
        UVM_INFO(get_name(), "PASS: " + const_cast<apb_transaction&>(txn).convert2string(), uvm::UVM_MEDIUM);
      } else {
        fail_count++;
        std::ostringstream ss;
        ss << "FAIL: " << const_cast<apb_transaction&>(txn).convert2string()
           << " expected=0x" << std::hex << expected_regs[idx];
        UVM_ERROR(get_name(), ss.str());
      }
    }
  }

  // report_phase — prints final pass/fail summary at end of simulation
  void report_phase(uvm::uvm_phase& phase) {
    std::ostringstream ss;
    ss << "Scoreboard: " << pass_count << " passed, " << fail_count << " failed";
    if (fail_count > 0)
      UVM_ERROR(get_name(), ss.str());
    else
      UVM_INFO(get_name(), ss.str(), uvm::UVM_LOW);
  }
};

#endif // APB_SCOREBOARD_H
