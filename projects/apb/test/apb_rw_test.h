// ----------------------------------------------------------------------------
// apb_rw_test — Write-then-read data integrity test
//
// This test writes a unique pattern to all 16 registers, then reads them
// all back. The scoreboard automatically checks each read against the
// expected value.
//
// UVM test anatomy:
//   1. build_phase (inherited) — creates env, gets vif from config_db
//   2. run_phase — waits for reset, then starts sequences on the sequencer
//   3. report_phase (in scoreboard) — prints pass/fail summary
//
// Objections:
//   raise_objection() / drop_objection() control simulation lifetime.
//   The simulation ends when all objections are dropped — same as UVM-SV.
// ----------------------------------------------------------------------------
#ifndef APB_RW_TEST_H
#define APB_RW_TEST_H

#include <uvm>
#include "apb_base_test.h"
#include "apb_write_sequence.h"
#include "apb_read_sequence.h"

class apb_rw_test : public apb_base_test {
public:
  UVM_COMPONENT_UTILS(apb_rw_test);

  apb_rw_test(uvm::uvm_component_name name)
    : apb_base_test(name) {}

  void run_phase(uvm::uvm_phase& phase) {
    phase.raise_objection(this, "rw_test");

    // Wait for reset to deassert before driving transactions
    sc_core::sc_signal<bool>* resetn = nullptr;
    if (uvm::uvm_config_db<sc_core::sc_signal<bool>*>::get(this, "", "resetn", resetn)) {
      while (!resetn->read())
        sc_core::wait(10, sc_core::SC_NS);
      sc_core::wait(10, sc_core::SC_NS);  // one extra cycle after reset
    }

    // Phase 1: Write unique pattern to all registers
    apb_write_sequence* wr_seq = apb_write_sequence::type_id::create("wr_seq");
    wr_seq->start(env->agent->sequencer);

    // Phase 2: Read back and verify (scoreboard checks automatically)
    apb_read_sequence* rd_seq = apb_read_sequence::type_id::create("rd_seq");
    rd_seq->start(env->agent->sequencer);

    // Allow monitor to observe the last transaction
    sc_core::wait(20, sc_core::SC_NS);

    phase.drop_objection(this, "rw_test");
  }
};

#endif // APB_RW_TEST_H
