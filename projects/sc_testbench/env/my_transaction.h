#ifndef MY_TRANSACTION_H_
#define MY_TRANSACTION_H_

#include <iomanip>
#include <sstream>
#include <string>
#include <systemc>
#include <uvm>

#include "constants.h"


class my_transaction : public uvm::uvm_sequence_item {
 public:
  my_transaction(const std::string& name = "my_transaction_seq_item",
                 int addr = 0, int data = 0, bus_op_t op = BUS_READ)
      : uvm::uvm_sequence_item(name) {
    this->addr = addr;
    this->data = data;
    this->op = op;
  }

  ~my_transaction() {}

  UVM_OBJECT_UTILS(my_transaction);

  virtual void do_copy(const uvm::uvm_object& rhs) {
    const my_transaction* rhs_ = dynamic_cast<const my_transaction*>(&rhs);
    if (rhs_ == NULL)
      UVM_ERROR("do_copy", "cast failed, check type compatability");

    uvm_sequence_item::do_copy(rhs);

    addr = rhs_->addr;
    data = rhs_->data;
    op = rhs_->op;
  }

  virtual bool do_compare(const uvm::uvm_object& rhs,
                          const uvm::uvm_comparer* comparer) {
    const my_transaction* rhs_ = dynamic_cast<const my_transaction*>(&rhs);
    if (rhs_ == NULL)
      UVM_FATAL("do_compare", "cast failed, check type compatibility");

    return ((op == rhs_->op) && (addr == rhs_->addr) && (data == rhs_->data));
  }

  void do_print(const uvm::uvm_printer& printer) const {
    printer.print_string("op", (op ? "BUS_WRITE" : "BUS_READ"));
    printer.print_field_int("addr", addr, 32, uvm::UVM_HEX);
    printer.print_field_int("data", data, 32, uvm::UVM_HEX);
  }

  std::string convert2string() {
    std::ostringstream str;
    str << "op " << (op ? "BUS_WRITE" : "BUS_READ");
    str << " addr: 0x" << std::hex << std::setw(3) << std::setfill('0') << addr;
    str << " data: 0x" << std::hex << std::setw(3) << std::setfill('0') << data;
    return str.str();
  }

  // data members
 public:
  int addr;
  int data;
  bus_op_t op;
};

class my_req : public my_transaction {
 public:
  my_req(const std::string& name = "my_req_seq_item") : my_transaction(name) {}
  ~my_req() {}

  UVM_OBJECT_UTILS(my_req);
};

class my_rsp : public my_transaction {
 public:
  my_rsp(const std::string& name = "my_rsp_seq_item") : my_transaction(name) {}

  ~my_rsp() {}

  UVM_OBJECT_UTILS(my_rsp);

  virtual void do_copy(const uvm::uvm_object& rhs) {
    const my_rsp* rhs_ = dynamic_cast<const my_rsp*>(&rhs);
    if (rhs_ == NULL)
      UVM_FATAL("do_copy", "cast failed, check type compatibility");

    my_transaction::do_copy(rhs);
    status = rhs_->status;
  }

  std::string convert2string() {
    std::string statusstr;

    if (status == STATUS_OK)
      statusstr = "STATUS_OK";
    else
      statusstr = "STATUS_NOT_OK";

    std::ostringstream str;
    str << my_transaction::convert2string() << " status: " << statusstr;
    return str.str();
  }

 private:
  status_t status;
};

#endif /* MY_TRANSACTION_H_ */
