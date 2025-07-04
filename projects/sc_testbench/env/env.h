
#include <systemc>
#include <uvm>

#include "completer_agent.h"
#include "requester_agent.h"

using namespace uvm;
using namespace sc_core;

class env : public uvm::uvm_env {
 public:
  UVM_COMPONENT_UTILS(env);

  explicit env(uvm::uvm_component_name name) : uvm::uvm_env(name) {}

  void build_phase(uvm_phase& phase) {
    std::cout << sc_time_stamp() << ": " << get_full_name()
              << " phase: " << phase.get_name() << std::endl;

    // Instantiate environment using the factory
    requester_agent_h =
        requester_agent::type_id::create("requester_agent_h", this);

    // Instantiate environment using the factory
    completer_agent_h =
        completer_agent::type_id::create("completer_agent_h", this);
  }

  void connect_phase(uvm_phase& phase) {
    std::cout << sc_time_stamp() << ": " << get_full_name()
              << " phase: " << phase.get_name() << std::endl;
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

  requester_agent* requester_agent_h;
  completer_agent* completer_agent_h;
};
