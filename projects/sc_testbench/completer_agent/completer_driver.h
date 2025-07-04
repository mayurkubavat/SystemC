
#include <systemc>
#include <uvm>

using namespace uvm;
using namespace sc_core;

template <typename REQ = uvm::uvm_sequence_item, typename RSP = REQ>
class completer_driver : public uvm::uvm_driver<REQ, RSP> {
 public:
  UVM_COMPONENT_PARAM_UTILS(completer_driver<REQ, RSP>);

  completer_driver(uvm::uvm_component_name name)
      : uvm::uvm_driver<REQ, RSP>(name) {}

  void run_phase(uvm_phase& phase) {
    std::cout << sc_time_stamp() << ": " << " start phase: " << phase.get_name()
              << std::endl;
    std::cout << sc_time_stamp() << ": " << " end phase: " << phase.get_name()
              << std::endl;
  }
};
