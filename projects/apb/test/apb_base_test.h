// ----------------------------------------------------------------------------
// apb_base_test — Base class for all APB tests
//
// Creates the environment and retrieves the virtual interface from config_db.
// Concrete tests (like apb_rw_test) extend this and add stimulus in
// run_phase.
//
// UVM-SV equivalent:
//   class apb_base_test extends uvm_test;
//     apb_env env;
//     function void build_phase(uvm_phase phase);
//       env = apb_env::type_id::create("env", this);
//       uvm_config_db#(virtual apb_if)::get(this, "", "vif", vif);
//     endfunction
//   endclass
// ----------------------------------------------------------------------------
#ifndef APB_BASE_TEST_H
#define APB_BASE_TEST_H

#include <systemc>
#include <uvm>
#include "apb_env.h"
#include "apb_slave.h"
#include "apb_if.h"

class apb_base_test : public uvm::uvm_test {
public:
  UVM_COMPONENT_UTILS(apb_base_test);

  apb_env* env;
  apb_if* vif;

  apb_base_test(uvm::uvm_component_name name)
    : uvm::uvm_test(name), env(nullptr), vif(nullptr) {}

  void build_phase(uvm::uvm_phase& phase) {
    uvm::uvm_test::build_phase(phase);
    env = apb_env::type_id::create("env", this);

    if (!uvm::uvm_config_db<apb_if*>::get(this, "", "vif", vif))
      UVM_FATAL(get_name(), "Failed to get APB interface from config_db");
  }

  void connect_phase(uvm::uvm_phase& phase) {
    env->vif = vif;
  }
};

#endif // APB_BASE_TEST_H
