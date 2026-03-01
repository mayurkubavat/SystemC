// ----------------------------------------------------------------------------
// axi_lite_slave_agent — Bundles slave driver + monitor (no sequencer)
//
// This agent packages the reactive slave driver and passive monitor for the
// AXI4-Lite interface. Unlike the APB master agent, there is no sequencer
// because the slave driver reacts to bus requests rather than initiating them.
//
// Phase ordering (same gotcha as the APB agent):
//   build_phase runs top-down:    test -> env -> agent -> driver/monitor
//   connect_phase runs bottom-up: driver/monitor -> agent -> env -> test
//
// The agent fetches the virtual interface and memory model from config_db
// in build_phase, then passes them to the driver and monitor in connect_phase.
//
// UVM-SV equivalent:
//   class axi_lite_slave_agent extends uvm_agent;
//     axi_lite_slave_driver driver;
//     axi_lite_monitor      monitor;
//   endclass
// ----------------------------------------------------------------------------
#ifndef AXI_LITE_SLAVE_AGENT_H
#define AXI_LITE_SLAVE_AGENT_H

#include <uvm>
#include "axi_lite_slave_driver.h"
#include "axi_lite_monitor.h"
#include "axi_lite_if.h"
#include "memory_model.h"

// Agent architecture comparison:
//
//   APB Master Agent (active):         AXI Slave Agent (reactive):
//   ┌──────────────────────┐           ┌──────────────────────┐
//   │ sequencer → driver   │           │ driver (no sequencer) │
//   │ monitor              │           │ monitor               │
//   └──────────────────────┘           └───────────────────────┘
//   Sequencer feeds txns to driver.    Driver watches bus and reacts.
//   Test starts sequences on sqr.      No test interaction needed.
//
class axi_lite_slave_agent : public uvm::uvm_agent {
public:
  UVM_COMPONENT_UTILS(axi_lite_slave_agent);

  axi_lite_slave_driver* driver;   // Reactive: responds to AXI requests
  axi_lite_monitor*      monitor;  // Passive: observes completed handshakes
  axi_lite_if*           vif;      // Virtual interface (signal bundle)
  memory_model*          mem;      // Set by parent env in build_phase

  axi_lite_slave_agent(uvm::uvm_component_name name)
    : uvm::uvm_agent(name), driver(nullptr), monitor(nullptr),
      vif(nullptr), mem(nullptr) {}

  void build_phase(uvm::uvm_phase& phase) {
    uvm::uvm_agent::build_phase(phase);

    // config_db::get retrieves the virtual interface pointer that was
    // stored in sc_main via config_db::set. The wildcard "*" path in
    // set() means any component can retrieve it.
    //
    // SV equivalent: uvm_config_db#(virtual axi_lite_if)::get(this, "", ...)
    if (!uvm::uvm_config_db<axi_lite_if*>::get(this, "", "axi_vif", vif))
      UVM_FATAL(get_name(), "No virtual interface specified for AXI slave agent");

    // No sequencer needed — slave drivers react to bus activity, they
    // don't initiate transactions. Compare with the APB master agent
    // which has a sequencer that feeds transactions to its driver.
    monitor = axi_lite_monitor::type_id::create("monitor", this);
    driver  = axi_lite_slave_driver::type_id::create("driver", this);
  }

  // connect_phase passes the vif and mem pointers to sub-components.
  // Note: `mem` is set by the parent env in build_phase (which runs
  // before this connect_phase because build is top-down).
  void connect_phase(uvm::uvm_phase& phase) {
    driver->vif = vif;
    driver->mem = mem;
    monitor->vif = vif;
  }
};

#endif // AXI_LITE_SLAVE_AGENT_H
