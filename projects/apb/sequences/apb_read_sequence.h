// ----------------------------------------------------------------------------
// apb_read_sequence — Reads back all 16 registers
//
// Run after apb_write_sequence to verify data integrity. The scoreboard
// compares the read-back values against its shadow register model.
// ----------------------------------------------------------------------------
#ifndef APB_READ_SEQUENCE_H
#define APB_READ_SEQUENCE_H

#include <uvm>
#include "apb_base_sequence.h"

class apb_read_sequence : public apb_base_sequence {
public:
  UVM_OBJECT_UTILS(apb_read_sequence);

  apb_read_sequence(const std::string& name = "apb_read_sequence")
    : apb_base_sequence(name) {}

  void body() {
    for (int i = 0; i < 16; i++) {
      apb_transaction* txn = apb_transaction::type_id::create("txn");
      txn->direction = APB_READ;
      txn->addr = i * 4;

      start_item(txn);
      finish_item(txn);
    }
  }
};

#endif // APB_READ_SEQUENCE_H
