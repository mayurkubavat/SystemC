// ----------------------------------------------------------------------------
// dma_watchdog — Hang detection via activity monitoring (UVM heartbeat equivalent)
//
// UVM-SystemC does not implement uvm_heartbeat. This component provides
// equivalent functionality: it monitors the AXI analysis port for activity
// and triggers UVM_FATAL if no transaction is observed within a configurable
// timeout window.
//
// How it works:
//   1. Subscribes to the AXI monitor's analysis port (same as scoreboard)
//   2. Every observed transaction sets an activity flag
//   3. A watchdog loop checks the flag each timeout window
//   4. If flag is still clear after timeout → hang detected → UVM_FATAL
//
// SystemC key concept:
//   sc_core::wait(time, unit, event) — waits for EITHER the event OR the
//   timeout, whichever comes first. This is the building block for all
//   watchdog/heartbeat patterns in SystemC.
//
// SV UVM equivalent:
//   uvm_heartbeat hb;
//   hb = new("hb", this, objection);
//   hb.set_heartbeat(timeout_window, component_list);
// ----------------------------------------------------------------------------
#ifndef DMA_WATCHDOG_H
#define DMA_WATCHDOG_H

#include <systemc>
#include <uvm>
#include "axi_lite_transaction.h"

class dma_watchdog : public uvm::uvm_subscriber<axi_lite_transaction> {
public:
  UVM_COMPONENT_UTILS(dma_watchdog);

  // Configurable timeout in nanoseconds — set by the test before run_phase.
  // If no AXI transaction is observed within this window, it's a hang.
  uint32_t timeout_ns;

  // Controls whether the watchdog is active. Disabled during reset and
  // before the DMA transfer starts (when no activity is expected).
  bool enabled;

  dma_watchdog(uvm::uvm_component_name name)
    : uvm::uvm_subscriber<axi_lite_transaction>(name),
      timeout_ns(500), enabled(false), activity_seen(false) {}

  // write() — called automatically when the AXI monitor publishes a transaction.
  // This is the "heartbeat pet" — it signals that activity occurred.
  //
  // uvm_subscriber<T> provides the analysis_export and routes incoming
  // transactions to this write() method. Same mechanism as the scoreboard,
  // but here we just set a flag instead of checking data.
  void write(const axi_lite_transaction& txn) override {
    activity_seen = true;
  }

  void run_phase(uvm::uvm_phase& phase) override {
    // Don't start watching until enabled (test sets this after DMA starts)
    while (!enabled)
      sc_core::wait(10, sc_core::SC_NS);

    UVM_INFO(get_name(), "Watchdog armed — monitoring for activity", uvm::UVM_LOW);

    // ---- Watchdog loop ----
    // Each iteration:
    //   1. Clear the activity flag
    //   2. Wait for the timeout window
    //   3. Check if write() was called during the window
    //   4. If not → hang → UVM_FATAL
    //
    // This is the simplest reliable hang detection pattern. It doesn't
    // require sc_event tricks — just a boolean flag set by the analysis
    // port callback and checked periodically.
    while (enabled) {
      activity_seen = false;
      sc_core::wait(timeout_ns, sc_core::SC_NS);

      if (!enabled)
        break;

      if (!activity_seen) {
        std::ostringstream ss;
        ss << "HANG DETECTED: No AXI activity for " << timeout_ns
           << " ns. Simulation appears stuck.";
        UVM_FATAL(get_name(), ss.str());
      }
    }
  }

private:
  bool activity_seen;
};

#endif // DMA_WATCHDOG_H
