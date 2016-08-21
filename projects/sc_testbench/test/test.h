
#include <systemc>
#include <uvm>


class test : public uvm::uvm_test
{

 public:
  UVM_COMPONENT_UTILS(top);
    
  top(uvm::uvm_component_name name) : uvm::uvm_test(name){};

};

