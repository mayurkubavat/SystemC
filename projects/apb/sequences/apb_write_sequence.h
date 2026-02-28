// ----------------------------------------------------------------------------
// apb_write_sequence — Writes test data to all 16 registers
//
// The body() method is the sequence entry point (like SV's `task body()`).
// It creates transactions via the factory, fills in fields, and sends them
// to the driver using start_item/finish_item:
//
//   start_item(txn)  — request sequencer arbitration
//   finish_item(txn) — send to driver, block until driver calls item_done()
//
// This is identical to the UVM-SV flow.
// ----------------------------------------------------------------------------
#ifndef APB_WRITE_SEQUENCE_H
#define APB_WRITE_SEQUENCE_H

#include <uvm>
#include "apb_base_sequence.h"

class apb_write_sequence : public apb_base_sequence {
public:
  UVM_OBJECT_UTILS(apb_write_sequence);

  apb_write_sequence(const std::string& name = "apb_write_sequence")
    : apb_base_sequence(name) {}

  void body() {
    for (int i = 0; i < 16; i++) {
      apb_transaction* txn = apb_transaction::type_id::create("txn");
      txn->direction = APB_WRITE;
      txn->addr = i * 4;          // registers are word-aligned (0x00, 0x04, ...)
      txn->data = 0xA000 + i;     // unique pattern per register

      start_item(txn);
      finish_item(txn);
    }
  }
};

#endif // APB_WRITE_SEQUENCE_H
