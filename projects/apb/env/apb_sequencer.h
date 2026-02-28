// ----------------------------------------------------------------------------
// apb_sequencer — Routes transactions from sequences to the driver
//
// In UVM-SV: class apb_sequencer extends uvm_sequencer #(apb_transaction);
// UVM-SystemC is identical in concept — the sequencer is parameterized
// on the transaction type and connects to the driver via TLM ports.
// ----------------------------------------------------------------------------
#ifndef APB_SEQUENCER_H
#define APB_SEQUENCER_H

#include <uvm>
#include "apb_transaction.h"

class apb_sequencer : public uvm::uvm_sequencer<apb_transaction> {
public:
  UVM_COMPONENT_UTILS(apb_sequencer);

  apb_sequencer(uvm::uvm_component_name name)
    : uvm::uvm_sequencer<apb_transaction>(name) {}
};

#endif // APB_SEQUENCER_H
