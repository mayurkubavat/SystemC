
#include <systemc>
#include <uvm>

#include "env.h"

using namespace uvm;
using namespace sc_core;

class test : public uvm::uvm_test {
 public:
  UVM_COMPONENT_UTILS(test);

  explicit test(uvm::uvm_component_name name) : uvm::uvm_test(name) {
    delay = sc_time(1.1, SC_MS);
  }

  void build_phase(uvm_phase& phase) {
    std::cout << sc_time_stamp() << ": " << get_full_name()
              << " phase: " << phase.get_name() << std::endl;

    // Instantiate environment using the factory
    env_h = env::type_id::create("env_h", this);
  }

  void connect_phase(uvm_phase& phase) {
    std::cout << sc_time_stamp() << ": " << get_full_name()
              << " phase: " << phase.get_name() << std::endl;
  }

  void end_of_elaboration_phase(uvm_phase& phase) {
    std::cout << sc_time_stamp() << ": " << get_full_name()
              << " phase: " << phase.get_name() << std::endl;

    // Print topology
    // uvm_default_printer->knobs.reference = 0;
    uvm::uvm_root::get()->print_topology();
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

  void report_phase(uvm_phase& phase) {
    std::cout << sc_time_stamp() << ": " << get_full_name()
              << " phase: " << phase.get_name() << std::endl;
  }

  void run_phase(uvm_phase& phase) {
    std::cout << sc_time_stamp() << ": " << get_full_name()
              << " start phase: " << phase.get_name() << std::endl;
    wait(delay);
    std::cout << sc_time_stamp() << ": " << get_full_name()
              << " end phase: " << phase.get_name() << std::endl;
  }

  // member data objects
  sc_time delay;

  env* env_h;
};
