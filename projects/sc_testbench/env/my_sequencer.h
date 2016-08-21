
#ifndef MY_SEQUENCER_H_
#define MY_SEQUENCER_H_

#include <systemc>
#include <tlm.h>
#include <uvm>

template <typename REQ = uvm::uvm_sequence_item, typename RSP = REQ>
class my_sequencer : public uvm::uvm_sequencer<REQ,RSP>
{
 public:
  my_sequencer( uvm::uvm_component_name name ) : uvm::uvm_sequencer<REQ,RSP>( name )
  {
  }

  UVM_COMPONENT_PARAM_UTILS(my_sequencer<REQ,RSP>);

};

#endif /* MY_SEQUENCER_H_ */
