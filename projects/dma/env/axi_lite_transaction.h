// ----------------------------------------------------------------------------
// axi_lite_transaction — UVM sequence item for AXI4-Lite transfers
//
// In UVM-SV you'd extend uvm_sequence_item and use `uvm_object_utils`.
// UVM-SystemC is nearly identical:
//   SV:  class axi_lite_transaction extends uvm_sequence_item;
//   SC:  class axi_lite_transaction : public uvm::uvm_sequence_item { ... };
//
// The `do_copy`, `do_compare`, and `do_print` methods correspond to the
// SV field macros (`uvm_field_int`, etc.) but give you explicit control.
// ----------------------------------------------------------------------------
#ifndef AXI_LITE_TRANSACTION_H
#define AXI_LITE_TRANSACTION_H

#include <systemc>
#include <uvm>
#include <cstdint>
#include <iomanip>
#include <sstream>

#include "dma_constants.h"

class axi_lite_transaction : public uvm::uvm_sequence_item {
public:
  // Transaction fields — these represent one AXI4-Lite transfer.
  // In SV UVM, you might use `rand` fields here for constrained-random;
  // UVM-SystemC doesn't have built-in constraint solving, so randomization
  // would need a separate C++ library (e.g., CRAVE or manual std::random).
  uint32_t         addr;
  uint32_t         data;
  axi_direction_t  direction;
  uint32_t         resp;

  axi_lite_transaction(const std::string& name = "axi_lite_transaction")
    : uvm::uvm_sequence_item(name), addr(0), data(0),
      direction(AXI_READ), resp(AXI_RESP_OKAY) {}

  // UVM_OBJECT_UTILS registers this class with the UVM factory, enabling:
  //   - type_id::create("name") — factory construction (like SV's `uvm_object_utils`)
  //   - Automatic type name registration for printing / debugging
  // Without this macro, create() won't work and you'll get linker errors.
  UVM_OBJECT_UTILS(axi_lite_transaction);

  // In SV UVM, `uvm_field_int` macros auto-generate copy/compare/print.
  // UVM-SystemC requires you to implement these manually. It's more verbose
  // but gives you explicit control — no hidden automation surprises.

  // do_copy — deep copy of transaction fields
  virtual void do_copy(const uvm::uvm_object& rhs) {
    const axi_lite_transaction* rhs_ =
        dynamic_cast<const axi_lite_transaction*>(&rhs);
    if (!rhs_) UVM_FATAL("do_copy", "cast failed");
    uvm_sequence_item::do_copy(rhs);
    addr      = rhs_->addr;
    data      = rhs_->data;
    direction = rhs_->direction;
    resp      = rhs_->resp;
  }

  // do_compare — field-by-field comparison for scoreboard checks
  virtual bool do_compare(const uvm::uvm_object& rhs,
      const uvm::uvm_comparer* comparer) {
    const axi_lite_transaction* rhs_ =
        dynamic_cast<const axi_lite_transaction*>(&rhs);
    if (!rhs_) UVM_FATAL("do_compare", "cast failed");
    return (addr == rhs_->addr && data == rhs_->data &&
            direction == rhs_->direction && resp == rhs_->resp);
  }

  // do_print — formatted output for UVM print/sprint
  void do_print(const uvm::uvm_printer& printer) const {
    printer.print_string("dir", (direction == AXI_WRITE) ? "WRITE" : "READ");
    printer.print_field_int("addr", addr);
    printer.print_field_int("data", data);
    printer.print_field_int("resp", resp);
  }

  // convert2string — human-readable one-liner for UVM_INFO messages
  std::string convert2string() {
    std::ostringstream ss;
    ss << (direction == AXI_WRITE ? "WRITE" : "READ ")
       << " addr=0x" << std::hex << std::setw(2) << std::setfill('0') << addr
       << " data=0x" << std::setw(8) << std::setfill('0') << data
       << " resp=" << std::dec << resp;
    if (resp != AXI_RESP_OKAY) ss << " [ERR]";
    return ss.str();
  }
};

#endif // AXI_LITE_TRANSACTION_H
