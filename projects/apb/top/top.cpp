// ----------------------------------------------------------------------------
// top.cpp — SystemC entry point (sc_main) and testbench top-level
//
// This file wires together the DUT, APB interface, reset generator, VCD
// tracing, and UVM test infrastructure.
//
// sc_main is the SystemC equivalent of Verilog's top-level module + initial
// block. Unlike SV where the simulator manages the hierarchy, in SystemC
// you explicitly instantiate modules and bind ports in sc_main.
//
// DUT selection:
//   -DUSE_RTL=ON  → Verilator-compiled Verilog RTL (Vapb_slave)
//   default       → SystemC behavioral model (apb_slave)
// Both produce identical behavior; the testbench is unchanged.
// ----------------------------------------------------------------------------
#include <systemc>
#include <uvm>

#ifdef USE_RTL
#include "Vapb_slave.h"   // Verilator-generated SystemC wrapper
#else
#include "apb_slave.h"    // SystemC behavioral model
#endif

#include "apb_if.h"
#include "apb_rw_test.h"

// Simple reset generator — holds PRESETn low for 2 clock cycles, then releases.
// In SV you'd write: initial begin resetn = 0; repeat(2) @(posedge clk); resetn = 1; end
// In SystemC, SC_THREAD with wait() serves the same purpose.
SC_MODULE(reset_gen) {
  sc_core::sc_in<bool> clk;
  sc_core::sc_out<bool> resetn;

  SC_CTOR(reset_gen) {
    SC_THREAD(run);
    sensitive << clk.pos();
  }

  void run() {
    resetn.write(false);
    wait(); // 1 clock
    wait(); // 2 clocks
    resetn.write(true);
  }
};

int sc_main(int argc, char* argv[]) {
  // Clock: 10ns period (100 MHz)
  sc_core::sc_clock clk("clk", 10, sc_core::SC_NS);
  sc_core::sc_signal<bool> resetn;

  // APB signal interface — groups all bus signals into one sc_module.
  // This pointer is passed through config_db as the "virtual interface".
  apb_if apb("apb");

  // DUT — swap between SystemC model and RTL via -DUSE_RTL=ON
#ifdef USE_RTL
  Vapb_slave dut("dut");
#else
  apb_slave dut("dut");
#endif

  // Port binding — connect DUT ports to APB interface signals.
  // In SV you'd write: apb_slave dut(.PCLK(clk), .PRESETn(resetn), ...);
  // In SystemC, each port is bound with the () operator.
  dut.PCLK(clk);
  dut.PRESETn(resetn);
  dut.PSEL(apb.PSEL);
  dut.PENABLE(apb.PENABLE);
  dut.PWRITE(apb.PWRITE);
  dut.PADDR(apb.PADDR);
  dut.PWDATA(apb.PWDATA);
  dut.PRDATA(apb.PRDATA);
  dut.PREADY(apb.PREADY);
  dut.PSLVERR(apb.PSLVERR);

  // Reset generator
  reset_gen rst("rst");
  rst.clk(clk);
  rst.resetn(resetn);

  // VCD waveform tracing — open with GTKWave: gtkwave apb_slave.vcd
  sc_core::sc_trace_file* wave = sc_core::sc_create_vcd_trace_file("apb_slave");
  wave->set_time_unit(1, sc_core::SC_NS);
  sc_core::sc_trace(wave, clk, "clk");
  sc_core::sc_trace(wave, resetn, "resetn");
  sc_core::sc_trace(wave, apb.PSEL, "PSEL");
  sc_core::sc_trace(wave, apb.PENABLE, "PENABLE");
  sc_core::sc_trace(wave, apb.PWRITE, "PWRITE");
  sc_core::sc_trace(wave, apb.PADDR, "PADDR");
  sc_core::sc_trace(wave, apb.PWDATA, "PWDATA");
  sc_core::sc_trace(wave, apb.PRDATA, "PRDATA");
  sc_core::sc_trace(wave, apb.PREADY, "PREADY");
  sc_core::sc_trace(wave, apb.PSLVERR, "PSLVERR");

  // Pass interface and reset signal to UVM components via config_db.
  // This is how the testbench (C++ side) communicates the signal-level
  // interface to the UVM hierarchy. Components call config_db::get()
  // in their build_phase to retrieve these.
  uvm::uvm_config_db<apb_if*>::set(nullptr, "*", "vif", &apb);
  uvm::uvm_config_db<sc_core::sc_signal<bool>*>::set(nullptr, "*", "resetn", &resetn);

  // run_test() creates the test class by name (factory lookup), builds
  // the UVM hierarchy, then starts the SystemC simulation. The simulation
  // runs until all objections are dropped.
  uvm::run_test("apb_rw_test");

  sc_core::sc_close_vcd_trace_file(wave);
  return 0;
}
