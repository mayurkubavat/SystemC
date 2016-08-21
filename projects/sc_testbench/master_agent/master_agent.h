
#include <systemc>
#include <uvm>

#include "master_driver.h"
#include "my_transaction.h"
#include "my_sequencer.h"

using namespace uvm;
using namespace sc_core;

class master_agent : public uvm::uvm_agent
{

 public:
  UVM_COMPONENT_UTILS(master_agent);
    
  master_agent(uvm::uvm_component_name name) : uvm::uvm_agent(name)
  {
  }

  void build_phase(uvm_phase& phase)
  {
    std::cout << sc_time_stamp() << ": " << get_full_name()
              << " phase: " << phase.get_name() << std::endl;

    master_driver_h = master_driver<my_req,my_rsp>::type_id::create("master_driver_h", this);
    sequencer_h = my_sequencer<my_req,my_rsp>::type_id::create("sequencer_h", this);
  }

  void connect_phase(uvm_phase& phase)
  {
    std::cout << sc_time_stamp() << ": " << get_full_name()
              << " phase: " << phase.get_name() << std::endl;

    master_driver_h->seq_item_port(sequencer_h->seq_item_export);
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

  master_driver<my_req,my_rsp>* master_driver_h;
  my_sequencer<my_req,my_rsp>* sequencer_h;

};

