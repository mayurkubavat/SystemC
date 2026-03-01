// ----------------------------------------------------------------------------
// dma_base_sequence — Base class for all DMA sequences
//
// DMA sequences program DMA registers via APB, so they are parameterized on
// apb_transaction (the sequencer drives APB transactions to the DUT's APB
// register interface).
//
// Helper methods reg_write() and reg_read() encapsulate the start_item /
// finish_item handshake for single-register accesses.
//
// UVM-SV equivalent:
//   class dma_base_sequence extends uvm_sequence #(apb_transaction);
// ----------------------------------------------------------------------------
#ifndef DMA_BASE_SEQUENCE_H
#define DMA_BASE_SEQUENCE_H

#include <uvm>
#include "apb_transaction.h"
#include "apb_constants.h"

// The template parameter <apb_transaction> tells UVM which transaction type
// this sequence produces. The sequencer must be parameterized on the same type.
// SV equivalent: class dma_base_sequence extends uvm_sequence #(apb_transaction);
class dma_base_sequence : public uvm::uvm_sequence<apb_transaction> {
public:
  // Note: sequences use UVM_OBJECT_UTILS (not COMPONENT), because sequences
  // are transient objects, not persistent components in the UVM hierarchy.
  UVM_OBJECT_UTILS(dma_base_sequence);

  dma_base_sequence(const std::string& name = "dma_base_sequence")
    : uvm::uvm_sequence<apb_transaction>(name) {}

  virtual ~dma_base_sequence() {}

  // reg_write — issue a single APB write to the given address.
  //
  // start_item / finish_item is the UVM sequence-driver handshake:
  //   start_item(txn)  — waits until the driver calls get_next_item()
  //   finish_item(txn) — sends the transaction to the driver
  //
  // SV equivalent:
  //   `uvm_do_with(txn, { txn.addr == addr; txn.data == data; })
  // but done explicitly here since UVM-SystemC has no `uvm_do macros.
  void reg_write(uint32_t addr, uint32_t data) {
    apb_transaction* txn = apb_transaction::type_id::create("txn");
    txn->direction = APB_WRITE;
    txn->addr = addr;
    txn->data = data;

    start_item(txn);   // Synchronize with driver (blocking)
    finish_item(txn);  // Driver executes the transaction on the bus
  }

  // reg_read — issue a single APB read.
  //
  // *** KNOWN LIMITATION in UVM-SystemC ***
  // In SV UVM, get_next_item() returns a handle to the SAME object, so the
  // driver can update txn->data and the sequence sees the change. In UVM-
  // SystemC, get_next_item() COPIES the transaction, so driver modifications
  // don't propagate back. This means txn->data is always 0 after the read.
  //
  // Workaround: Use the scoreboard (which monitors the bus directly) for
  // verification, rather than reading back register values in the sequence.
  uint32_t reg_read(uint32_t addr) {
    apb_transaction* txn = apb_transaction::type_id::create("txn");
    txn->direction = APB_READ;
    txn->addr = addr;
    txn->data = 0;

    start_item(txn);
    finish_item(txn);

    return txn->data;  // Always 0 due to UVM-SystemC copy semantics — see above
  }
};

#endif // DMA_BASE_SEQUENCE_H
