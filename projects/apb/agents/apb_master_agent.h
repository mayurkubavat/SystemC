// ----------------------------------------------------------------------------
// apb_master_agent — Bundles driver + monitor + sequencer
//
// The agent is a standard UVM container that groups the three components
// needed for bus communication. It fetches the virtual interface from
// config_db in build_phase (top-down), then passes it to driver/monitor
// in connect_phase (bottom-up).
//
// Phase ordering gotcha (common UVM-SystemC pitfall):
//   build_phase runs top-down:    test → env → agent → driver/monitor
//   connect_phase runs bottom-up: driver/monitor → agent → env → test
//
// This means the agent must fetch vif in build_phase (not connect_phase),
// because connect_phase of child components runs BEFORE the parent's.
// ----------------------------------------------------------------------------
#ifndef APB_MASTER_AGENT_H
#define APB_MASTER_AGENT_H

#include <uvm>
#include "apb_master_driver.h"
#include "apb_monitor.h"
#include "apb_sequencer.h"
#include "apb_if.h"

class apb_master_agent : public uvm::uvm_agent {
public:
  UVM_COMPONENT_UTILS(apb_master_agent);

  apb_master_driver* driver;
  apb_monitor* monitor;
  apb_sequencer* sequencer;
  apb_if* vif;

  apb_master_agent(uvm::uvm_component_name name)
    : uvm::uvm_agent(name), driver(nullptr), monitor(nullptr),
      sequencer(nullptr), vif(nullptr) {}

  void build_phase(uvm::uvm_phase& phase) {
    uvm::uvm_agent::build_phase(phase);

    // Fetch virtual interface from config_db (set in sc_main)
    if (!uvm::uvm_config_db<apb_if*>::get(this, "", "vif", vif))
      UVM_FATAL(get_name(), "No virtual interface specified for agent");

    // Create sub-components via factory
    monitor = apb_monitor::type_id::create("monitor", this);
    sequencer = apb_sequencer::type_id::create("sequencer", this);
    driver = apb_master_driver::type_id::create("driver", this);
  }

  void connect_phase(uvm::uvm_phase& phase) {
    // TLM connection: sequencer → driver
    driver->seq_item_port.connect(sequencer->seq_item_export);
    // Pass virtual interface to driver and monitor
    driver->vif = vif;
    monitor->vif = vif;
  }
};

#endif // APB_MASTER_AGENT_H
