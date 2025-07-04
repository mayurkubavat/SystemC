
#include <systemc>
#include <uvm>

#include "completer_driver.h"
#include "my_sequencer.h"
#include "my_transaction.h"

using namespace uvm;
using namespace sc_core;

class completer_agent : public uvm::uvm_agent {
 public:
  UVM_COMPONENT_UTILS(completer_agent);

  explicit completer_agent(uvm::uvm_component_name name)
      : uvm::uvm_agent(name) {}

  void build_phase(uvm_phase& phase) {
    std::cout << sc_time_stamp() << ": " << get_full_name()
              << " phase: " << phase.get_name() << std::endl;

    completer_driver_h = completer_driver<my_req, my_rsp>::type_id::create(
        "completer_driver_h", this);
    sequencer_h =
        my_sequencer<my_req, my_rsp>::type_id::create("sequencer_h", this);
  }

  void connect_phase(uvm_phase& phase) {
    std::cout << sc_time_stamp() << ": " << get_full_name()
              << " phase: " << phase.get_name() << std::endl;

    completer_driver_h->seq_item_port(sequencer_h->seq_item_export);
  }

  void end_of_elaboration_phase(uvm_phase& phase) {
    std::cout << sc_time_stamp() << ": " << get_full_name()
              << " phase: " << phase.get_name() << std::endl;
  }

  void start_of_simulation_phase(uvm_phase& phase) {
    std::cout << sc_time_stamp() << ": " << get_full_name()
              << " phase: " << phase.get_name() << std::endl;
  }

  void extract_phase(uvm_phase& phase) {
    std::cout << sc_time_stamp() << ": " << get_full_name()
              << " phase: " << phase.get_name() << std::endl;
  }

  void check_phase(uvm_phase& phase) {
    std::cout << sc_time_stamp() << ": " << get_full_name()
              << " phase: " << phase.get_name() << std::endl;
  }

  completer_driver<my_req, my_rsp>* completer_driver_h;
  my_sequencer<my_req, my_rsp>* sequencer_h;
};
