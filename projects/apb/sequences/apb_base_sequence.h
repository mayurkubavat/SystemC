// ----------------------------------------------------------------------------
// apb_base_sequence — Base class for all APB sequences
//
// UVM sequences generate stimulus by creating transactions and sending
// them to the driver through the sequencer. This base class is
// parameterized on apb_transaction.
//
// UVM-SV equivalent:
//   class apb_base_sequence extends uvm_sequence #(apb_transaction);
// ----------------------------------------------------------------------------
#ifndef APB_BASE_SEQUENCE_H
#define APB_BASE_SEQUENCE_H

#include <uvm>
#include "apb_transaction.h"

class apb_base_sequence : public uvm::uvm_sequence<apb_transaction> {
public:
  UVM_OBJECT_UTILS(apb_base_sequence);

  apb_base_sequence(const std::string& name = "apb_base_sequence")
    : uvm::uvm_sequence<apb_transaction>(name) {}

  virtual ~apb_base_sequence() {}
};

#endif // APB_BASE_SEQUENCE_H
