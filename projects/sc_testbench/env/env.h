
#include <systemc>
#include <uvm>

#include "master_agent.h"
#include "slave_agent.h"

using namespace uvm;
using namespace sc_core;

class env : public uvm::uvm_env
{

 public:
  UVM_COMPONENT_UTILS(env);
    
  env(uvm::uvm_component_name name) : uvm::uvm_env(name)
  {
  }

  void build_phase(uvm_phase& phase)
  {
    std::cout << sc_time_stamp() << ": " << get_full_name()
              << " phase: " << phase.get_name() << std::endl;

    //Instantiate environment using the factory
    master_agent_h = master_agent::type_id::create("master_agent_h", this);

    //Instantiate environment using the factory
    slave_agent_h = slave_agent::type_id::create("slave_agent_h", this);
  }

  void connect_phase(uvm_phase& phase)
  {
    std::cout << sc_time_stamp() << ": " << get_full_name()
              << " phase: " << phase.get_name() << std::endl;
  }

  void end_of_elaboration_phase(uvm_phase& phase)
  {
    std::cout << sc_time_stamp() << ": " << get_full_name()
              << " phase: " << phase.get_name() << std::endl;
  }

  void start_of_simulation_phase(uvm_phase& phase)
  {
    std::cout << sc_time_stamp() << ": " << get_full_name()
              << " phase: " << phase.get_name() << std::endl;
  }

  void extract_phase(uvm_phase& phase)
  {
    std::cout << sc_time_stamp() << ": " << get_full_name()
              << " phase: " << phase.get_name() << std::endl;
  }

  void check_phase(uvm_phase& phase)
  {
    std::cout << sc_time_stamp() << ": " << get_full_name()
              << " phase: " << phase.get_name() << std::endl;
  }

  master_agent* master_agent_h;
  slave_agent* slave_agent_h;

};

