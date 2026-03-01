// ----------------------------------------------------------------------------
// dma_base_test — Base class for all DMA tests
//
// Creates the DMA environment and retrieves virtual interfaces from config_db.
// Concrete tests (like dma_simple_xfer_test) extend this and add stimulus
// in run_phase.
//
// UVM test hierarchy (top to bottom):
//   uvm_test          — top of the component tree, selected by run_test()
//     └─ uvm_env      — contains agents, scoreboards, coverage
//          ├─ agent    — bundles driver + monitor + sequencer
//          └─ scoreboard
//
// In SV UVM, the test name is passed as a +UVM_TESTNAME=... plusarg.
// In UVM-SystemC, it's passed directly to run_test("test_name") in sc_main.
// ----------------------------------------------------------------------------
#ifndef DMA_BASE_TEST_H
#define DMA_BASE_TEST_H

#include <systemc>
#include <uvm>
#include "dma_env.h"
#include "apb_if.h"
#include "axi_lite_if.h"

class dma_base_test : public uvm::uvm_test {
public:
  UVM_COMPONENT_UTILS(dma_base_test);

  dma_env* env;
  apb_if* apb_vif;
  axi_lite_if* axi_vif;

  dma_base_test(uvm::uvm_component_name name)
    : uvm::uvm_test(name), env(nullptr), apb_vif(nullptr), axi_vif(nullptr) {}

  void build_phase(uvm::uvm_phase& phase) {
    uvm::uvm_test::build_phase(phase);

    // Factory-create the entire environment. This single call triggers a
    // cascade of build_phases down the hierarchy (env → agents → drivers...).
    env = dma_env::type_id::create("env", this);

    // Retrieve virtual interfaces from config_db. These were stored in sc_main
    // (top.cpp) before UVM phases started. config_db is a global key-value
    // store that lets sc_main pass hardware signals to UVM components.
    //
    // Arguments: get(context, instance_path, key, value)
    //   context=""  — search from this component
    //   key="vif"   — must match the key used in config_db::set()
    if (!uvm::uvm_config_db<apb_if*>::get(this, "", "vif", apb_vif))
      UVM_FATAL(get_name(), "Failed to get APB interface from config_db");

    if (!uvm::uvm_config_db<axi_lite_if*>::get(this, "", "axi_vif", axi_vif))
      UVM_FATAL(get_name(), "Failed to get AXI interface from config_db");
  }
};

#endif // DMA_BASE_TEST_H
