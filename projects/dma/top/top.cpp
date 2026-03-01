// ----------------------------------------------------------------------------
// top.cpp — DMA engine testbench top-level
//
// Wires together: clock, reset, DUT (SystemC or Verilated), APB interface,
// AXI4-Lite interface, VCD tracing, and UVM test infrastructure.
//
// DUT selection:
//   -DUSE_RTL=ON  → Verilator-compiled Verilog RTL (Vdma_engine)
//   default       → SystemC behavioral model (dma_engine)
// ----------------------------------------------------------------------------
#include <systemc>
#include <uvm>

#ifdef USE_RTL
#include "Vdma_engine.h"
#else
#include "dma_engine.h"
#endif

#include "apb_if.h"
#include "axi_lite_if.h"
#include "dma_simple_xfer_test.h"

// Reset generator — holds PRESETn low for 2 clock cycles, then releases.
// Uses SC_THREAD (not SC_METHOD) because it needs wait() to span clock cycles.
// SC_THREAD runs once and can suspend/resume; SC_METHOD is called repeatedly.
SC_MODULE(reset_gen) {
  sc_core::sc_in<bool> clk;
  sc_core::sc_out<bool> resetn;

  SC_CTOR(reset_gen) {
    SC_THREAD(run);
    sensitive << clk.pos();  // Start on first posedge
  }

  void run() {
    resetn.write(false);   // Assert reset (active low)
    wait();                // Wait one posedge
    wait();                // Wait another posedge (2 cycles total)
    resetn.write(true);    // Release reset — DUT begins normal operation
  }
};

// sc_main — entry point for SystemC simulation (like `main` for C++).
// In SV, the simulator handles elaboration automatically. In SystemC,
// sc_main is where you manually instantiate modules, bind ports, set up
// tracing, and launch the UVM test.
int sc_main(int argc, char* argv[]) {
  // sc_clock is a built-in SystemC module that generates a periodic signal.
  // Period=10ns → 100MHz. Default duty cycle is 50% (high 5ns, low 5ns).
  // Posedges at: 0ns, 10ns, 20ns, 30ns, ...
  sc_core::sc_clock clk("clk", 10, sc_core::SC_NS);
  sc_core::sc_signal<bool> resetn;

  // APB signal interface (DMA register programming)
  apb_if apb("apb");

  // AXI4-Lite signal interface (DMA data transfers)
  axi_lite_if axi("axi");

  // Interrupt signal
  sc_core::sc_signal<bool> irq;

  // DUT — swap between SystemC model and RTL via -DUSE_RTL=ON
#ifdef USE_RTL
  Vdma_engine dut("dut");
#else
  dma_engine dut("dut");
#endif

  // Port binding — connects DUT ports to signal-level interfaces.
  // This is the SystemC equivalent of SV's `dut.PCLK(clk)` in a testbench
  // module. Each sc_in/sc_out port binds to an sc_signal (or sc_clock).
  // The syntax `port(signal)` calls the port's bind() method.
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

  // Port binding — AXI4-Lite master interface
  dut.AWADDR(axi.AWADDR);
  dut.AWVALID(axi.AWVALID);
  dut.AWREADY(axi.AWREADY);
  dut.WDATA(axi.WDATA);
  dut.WSTRB(axi.WSTRB);
  dut.WVALID(axi.WVALID);
  dut.WREADY(axi.WREADY);
  dut.BRESP(axi.BRESP);
  dut.BVALID(axi.BVALID);
  dut.BREADY(axi.BREADY);
  dut.ARADDR(axi.ARADDR);
  dut.ARVALID(axi.ARVALID);
  dut.ARREADY(axi.ARREADY);
  dut.RDATA(axi.RDATA);
  dut.RRESP(axi.RRESP);
  dut.RVALID(axi.RVALID);
  dut.RREADY(axi.RREADY);
  dut.IRQ(irq);

  // Reset generator
  reset_gen rst("rst");
  rst.clk(clk);
  rst.resetn(resetn);

  // VCD waveform tracing — creates dma_engine.vcd file for viewing in
  // GTKWave, Surfer, or any VCD viewer. Essential for debugging signal timing.
  // sc_trace() must be called BEFORE simulation starts (in sc_main, not in
  // a module). You can only trace sc_signals, not sc_in/sc_out ports directly.
  sc_core::sc_trace_file* wave = sc_core::sc_create_vcd_trace_file("dma_engine");
  wave->set_time_unit(1, sc_core::SC_NS);

  // Trace clock, reset, interrupt
  sc_core::sc_trace(wave, clk, "clk");
  sc_core::sc_trace(wave, resetn, "resetn");
  sc_core::sc_trace(wave, irq, "IRQ");

  // Trace APB signals
  sc_core::sc_trace(wave, apb.PSEL, "PSEL");
  sc_core::sc_trace(wave, apb.PENABLE, "PENABLE");
  sc_core::sc_trace(wave, apb.PWRITE, "PWRITE");
  sc_core::sc_trace(wave, apb.PADDR, "PADDR");
  sc_core::sc_trace(wave, apb.PWDATA, "PWDATA");
  sc_core::sc_trace(wave, apb.PRDATA, "PRDATA");
  sc_core::sc_trace(wave, apb.PREADY, "PREADY");

  // Trace AXI signals
  sc_core::sc_trace(wave, axi.ARADDR, "ARADDR");
  sc_core::sc_trace(wave, axi.ARVALID, "ARVALID");
  sc_core::sc_trace(wave, axi.ARREADY, "ARREADY");
  sc_core::sc_trace(wave, axi.RDATA, "RDATA");
  sc_core::sc_trace(wave, axi.RVALID, "RVALID");
  sc_core::sc_trace(wave, axi.RREADY, "RREADY");
  sc_core::sc_trace(wave, axi.AWADDR, "AWADDR");
  sc_core::sc_trace(wave, axi.AWVALID, "AWVALID");
  sc_core::sc_trace(wave, axi.AWREADY, "AWREADY");
  sc_core::sc_trace(wave, axi.WDATA, "WDATA");
  sc_core::sc_trace(wave, axi.WVALID, "WVALID");
  sc_core::sc_trace(wave, axi.WREADY, "WREADY");
  sc_core::sc_trace(wave, axi.BVALID, "BVALID");
  sc_core::sc_trace(wave, axi.BREADY, "BREADY");

  // config_db — pass hardware interfaces from sc_main into the UVM world.
  //
  // In SV UVM, virtual interfaces are set in an initial block or top module:
  //   uvm_config_db#(virtual apb_if)::set(null, "*", "vif", apb_if_inst);
  //
  // In SystemC, we pass pointers to sc_modules containing signals.
  // The wildcard "*" path means any UVM component can retrieve these.
  //
  // Why config_db and not constructor arguments?
  //   UVM components are created by the factory, which only passes the name.
  //   config_db is the standard mechanism to pass additional configuration
  //   (interfaces, parameters, etc.) from the structural (sc_main) world
  //   into the behavioral (UVM component) world.
  uvm::uvm_config_db<apb_if*>::set(nullptr, "*", "vif", &apb);
  uvm::uvm_config_db<axi_lite_if*>::set(nullptr, "*", "axi_vif", &axi);
  uvm::uvm_config_db<sc_core::sc_signal<bool>*>::set(nullptr, "*", "resetn", &resetn);
  uvm::uvm_config_db<sc_core::sc_signal<bool>*>::set(nullptr, "*", "irq", &irq);

  // Pass clock to AXI slave driver and monitor so they can use posedge-event
  // waits instead of fragile time-based waits. This ensures proper delta-cycle
  // ordering and makes AXI handshakes visible in VCD waveforms.
  uvm::uvm_config_db<sc_core::sc_clock*>::set(nullptr, "*", "clk", &clk);

  // Launch UVM test — this starts the UVM phase machine:
  //   build_phase → connect_phase → run_phase → ... → report_phase
  // The test name must match a class registered with UVM_COMPONENT_UTILS.
  // Simulation runs until all objections are dropped (see dma_simple_xfer_test).
  uvm::run_test("dma_simple_xfer_test");

  sc_core::sc_close_vcd_trace_file(wave);
  return 0;
}
