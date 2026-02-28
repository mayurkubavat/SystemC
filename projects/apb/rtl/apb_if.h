// ----------------------------------------------------------------------------
// apb_if — APB signal-level interface
//
// In SystemVerilog UVM, you'd declare an `interface` with modports and a
// clocking block. SystemC has no native `interface` keyword for signals.
// Instead, we group the bus signals inside an sc_module and pass a pointer
// to it through uvm_config_db — this is the "virtual interface" pattern
// in UVM-SystemC.
//
// Key difference from SV:
//   SV:  virtual apb_if vif;           // handle to an interface instance
//   SC:  apb_if* vif;                  // pointer to an sc_module with signals
// ----------------------------------------------------------------------------
#ifndef APB_IF_H
#define APB_IF_H

#include <systemc>
#include <cstdint>

class apb_if : public sc_core::sc_module {
public:
  // APB bus signals — each is an sc_signal (like a wire/reg in SV).
  // Using uint32_t (not sc_uint<32>) so the interface is compatible with
  // both the SystemC model and Verilator-generated wrappers.
  sc_core::sc_signal<bool> PSEL;
  sc_core::sc_signal<bool> PENABLE;
  sc_core::sc_signal<bool> PWRITE;
  sc_core::sc_signal<uint32_t> PADDR;
  sc_core::sc_signal<uint32_t> PWDATA;
  sc_core::sc_signal<uint32_t> PRDATA;
  sc_core::sc_signal<bool> PREADY;
  sc_core::sc_signal<bool> PSLVERR;

  // sc_module requires a constructor taking sc_module_name.
  // Each signal must be given a unique string name for VCD tracing.
  apb_if(const sc_core::sc_module_name& name)
    : sc_module(name),
      PSEL("PSEL"), PENABLE("PENABLE"), PWRITE("PWRITE"),
      PADDR("PADDR"), PWDATA("PWDATA"), PRDATA("PRDATA"),
      PREADY("PREADY"), PSLVERR("PSLVERR") {}
};

#endif // APB_IF_H
