// ----------------------------------------------------------------------------
// apb_transaction — UVM sequence item for APB transfers
//
// In UVM-SV you'd extend uvm_sequence_item and use `uvm_object_utils`.
// UVM-SystemC is nearly identical:
//   SV:  class apb_transaction extends uvm_sequence_item;
//   SC:  class apb_transaction : public uvm::uvm_sequence_item { ... };
//
// The `do_copy`, `do_compare`, and `do_print` methods correspond to the
// SV field macros (`uvm_field_int`, etc.) but give you explicit control.
// ----------------------------------------------------------------------------
#ifndef APB_TRANSACTION_H
#define APB_TRANSACTION_H

#include <systemc>
#include <uvm>
#include <cstdint>
#include <iomanip>
#include <sstream>

#include "apb_constants.h"

class apb_transaction : public uvm::uvm_sequence_item {
public:
  uint32_t addr;
  uint32_t data;
  apb_direction_t direction;
  bool error;

  apb_transaction(const std::string& name = "apb_transaction")
    : uvm::uvm_sequence_item(name), addr(0), data(0),
      direction(APB_READ), error(false) {}

  // UVM_OBJECT_UTILS registers this class with the UVM factory,
  // enabling type_id::create() — same concept as SV's `uvm_object_utils`.
  UVM_OBJECT_UTILS(apb_transaction);

  // do_copy — deep copy of transaction fields (SV: copy method or field macros)
  virtual void do_copy(const uvm::uvm_object& rhs) {
    const apb_transaction* rhs_ = dynamic_cast<const apb_transaction*>(&rhs);
    if (!rhs_) UVM_FATAL("do_copy", "cast failed");
    uvm_sequence_item::do_copy(rhs);
    addr = rhs_->addr;
    data = rhs_->data;
    direction = rhs_->direction;
    error = rhs_->error;
  }

  // do_compare — field-by-field comparison for scoreboard checks
  virtual bool do_compare(const uvm::uvm_object& rhs,
    const uvm::uvm_comparer* comparer) {
    const apb_transaction* rhs_ = dynamic_cast<const apb_transaction*>(&rhs);
    if (!rhs_) UVM_FATAL("do_compare", "cast failed");
    return (addr == rhs_->addr && data == rhs_->data &&
      direction == rhs_->direction);
  }

  // do_print — formatted output for UVM print/sprint
  void do_print(const uvm::uvm_printer& printer) const {
    printer.print_string("dir", (direction == APB_WRITE) ? "WRITE" : "READ");
    printer.print_field_int("addr", addr);
    printer.print_field_int("data", data);
    printer.print_field_int("error", error);
  }

  // convert2string — human-readable one-liner for UVM_INFO messages
  std::string convert2string() {
    std::ostringstream ss;
    ss << (direction == APB_WRITE ? "WRITE" : "READ ")
       << " addr=0x" << std::hex << std::setw(2) << std::setfill('0') << addr
       << " data=0x" << std::setw(8) << std::setfill('0') << data;
    if (error) ss << " [ERR]";
    return ss.str();
  }
};

#endif // APB_TRANSACTION_H
