// ----------------------------------------------------------------------------
// apb_env — Top-level UVM environment
//
// The environment assembles the agent and scoreboard, and wires the
// monitor's analysis port to the scoreboard. This is the same pattern
// as UVM-SV: env contains agents + scoreboards, and connect_phase
// hooks up the TLM connections.
//
// Phase ordering (important for UVM-SystemC newcomers):
//   build_phase  — top-down:  test → env → agent → driver/monitor
//   connect_phase — bottom-up: driver/monitor → agent → env → test
// ----------------------------------------------------------------------------
#ifndef APB_ENV_H
#define APB_ENV_H

#include <uvm>
#include "apb_master_agent.h"
#include "apb_scoreboard.h"
#include "apb_if.h"

class apb_env : public uvm::uvm_env {
public:
  UVM_COMPONENT_UTILS(apb_env);

  apb_master_agent* agent;
  apb_scoreboard* scoreboard;
  apb_if* vif;

  apb_env(uvm::uvm_component_name name)
    : uvm::uvm_env(name), agent(nullptr), scoreboard(nullptr),
      vif(nullptr) {}

  void build_phase(uvm::uvm_phase& phase) {
    uvm::uvm_env::build_phase(phase);
    agent = apb_master_agent::type_id::create("agent", this);
    scoreboard = apb_scoreboard::type_id::create("scoreboard", this);
  }

  // Wire monitor → scoreboard via analysis port
  void connect_phase(uvm::uvm_phase& phase) {
    agent->monitor->ap.connect(scoreboard->analysis_export);
  }
};

#endif // APB_ENV_H
