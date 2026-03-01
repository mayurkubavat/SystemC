// ----------------------------------------------------------------------------
// dma_env — Top-level UVM environment for DMA verification
//
// The environment assembles two agents (APB master for register programming,
// AXI4-Lite slave for memory-side traffic), a shared memory model, and the
// DMA scoreboard. connect_phase wires the AXI monitor's analysis port to
// the scoreboard.
//
// Phase ordering (important for UVM-SystemC newcomers):
//   build_phase  — top-down:  test -> env -> agent -> driver/monitor
//   connect_phase — bottom-up: driver/monitor -> agent -> env -> test
// ----------------------------------------------------------------------------
#ifndef DMA_ENV_H
#define DMA_ENV_H

#include <uvm>
#include "apb_master_agent.h"
#include "axi_lite_slave_agent.h"
#include "dma_scoreboard.h"
#include "memory_model.h"

// Testbench architecture (data flow):
//
//   ┌─────────────────────────────────────────────────────────────┐
//   │  dma_env                                                    │
//   │                                                             │
//   │  ┌───────────────┐   APB bus   ┌──────────────────────────┐ │
//   │  │ apb_agent     │◄───────────►│ DUT (dma_engine)         │ │
//   │  │  sequencer    │  registers  │                          │ │
//   │  │  driver       │             │  APB slave  AXI master   │ │
//   │  │  monitor      │             └──────────┬───────────────┘ │
//   │  └───────────────┘                        │ AXI bus         │
//   │                                           ▼                 │
//   │  ┌───────────────┐            ┌──────────────────────────┐  │
//   │  │ scoreboard    │◄───────────│ axi_agent                │  │
//   │  │  (checks data)│  analysis  │  driver (reactive slave) │  │
//   │  └───────────────┘   port     │  monitor (passive)       │  │
//   │                               └──────────────────────────┘  │
//   │                                           │                 │
//   │  ┌───────────────┐                        │                 │
//   │  │ memory_model  │◄───────────────────────┘                 │
//   │  │  (shared)     │  backing store for AXI reads/writes      │
//   │  └───────────────┘                                          │
//   └─────────────────────────────────────────────────────────────┘
//
class dma_env : public uvm::uvm_env {
public:
  UVM_COMPONENT_UTILS(dma_env);

  apb_master_agent*      apb_agent;   // Drives APB register transactions
  axi_lite_slave_agent*  axi_agent;   // Responds to DMA's AXI requests
  dma_scoreboard*        scoreboard;  // Verifies DMA write correctness
  memory_model*          mem;         // Shared backing store

  dma_env(uvm::uvm_component_name name)
    : uvm::uvm_env(name), apb_agent(nullptr), axi_agent(nullptr),
      scoreboard(nullptr), mem(nullptr) {}

  // build_phase runs TOP-DOWN: test → env → agent → driver/monitor.
  // This means env's build_phase runs BEFORE its children's build_phases.
  // We use this ordering to create shared resources (mem) and pass them
  // to children via direct pointer assignment.
  void build_phase(uvm::uvm_phase& phase) {
    uvm::uvm_env::build_phase(phase);

    // Create shared memory model. Both the AXI slave driver (for responding
    // to DMA reads) and the scoreboard (for golden data comparison) need
    // access to the same memory contents.
    mem = new memory_model();

    // Factory create — like SV's `component::type_id::create("name", this)`.
    // Using the factory (instead of `new`) enables type overrides in tests.
    apb_agent  = apb_master_agent::type_id::create("apb_agent", this);
    axi_agent  = axi_lite_slave_agent::type_id::create("axi_agent", this);
    scoreboard = dma_scoreboard::type_id::create("scoreboard", this);

    // *** GOTCHA: Phase ordering matters for shared resources ***
    // We assign `mem` here in build_phase (top-down) rather than in
    // connect_phase (bottom-up). Why? Because the agent's connect_phase
    // passes `mem` to its driver — but connect_phase runs bottom-up,
    // meaning the agent's connect_phase runs BEFORE the env's connect_phase.
    // If we waited until env's connect_phase to set axi_agent->mem, the
    // agent would pass nullptr to its driver, causing a crash.
    axi_agent->mem = mem;
    scoreboard->src_mem = mem;
  }

  // connect_phase runs BOTTOM-UP: driver/monitor → agent → env → test.
  // By the time we get here, all child components are fully built and
  // their internal connections are wired. We now make cross-component
  // connections like monitor → scoreboard.
  void connect_phase(uvm::uvm_phase& phase) {
    // Wire the analysis port: monitor publishes, scoreboard subscribes.
    // Every transaction the monitor observes will trigger scoreboard.write().
    axi_agent->monitor->ap.connect(scoreboard->analysis_export);
  }
};

#endif // DMA_ENV_H
