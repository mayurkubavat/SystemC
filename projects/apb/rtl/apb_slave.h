// ----------------------------------------------------------------------------
// apb_slave — SystemC behavioral model of an APB3 slave
//
// This is the "golden model" DUT: a 16 x 32-bit register file accessible
// over the AMBA APB3 bus. It behaves identically to the Verilog RTL in
// rtl/verilog/apb_slave.v and can be swapped at compile time via USE_RTL.
//
// SystemC ↔ SystemVerilog mapping:
//   SC_MODULE(...)        ≈  module ... endmodule
//   sc_in<T> / sc_out<T>  ≈  input / output ports
//   SC_METHOD(fn)         ≈  always @(...) — combinational or single-edge
//   sensitive << clk.pos()≈  always @(posedge clk)
//   dont_initialize()     ≈  prevents the method from firing at time 0
// ----------------------------------------------------------------------------
#ifndef APB_SLAVE_H
#define APB_SLAVE_H

#include <systemc>
#include <cstdint>

SC_MODULE(apb_slave) {

  // Clock and reset
  sc_core::sc_in<bool> PCLK;
  sc_core::sc_in<bool> PRESETn;

  // APB3 interface ports
  sc_core::sc_in<bool> PSEL;
  sc_core::sc_in<bool> PENABLE;
  sc_core::sc_in<bool> PWRITE;
  sc_core::sc_in<uint32_t> PADDR;
  sc_core::sc_in<uint32_t> PWDATA;

  sc_core::sc_out<uint32_t> PRDATA;
  sc_core::sc_out<bool> PREADY;
  sc_core::sc_out<bool> PSLVERR;

  // Internal storage
  static const int NUM_REGS = 16;
  static const int ADDR_MAX = (NUM_REGS - 1) * 4;
  uint32_t regs[NUM_REGS];

  // SC_CTOR is shorthand for the module constructor.
  // SC_METHOD registers `transfer` as a method process (like always @).
  SC_CTOR(apb_slave) {
    SC_METHOD(transfer);
    sensitive << PCLK.pos();  // triggered on rising edge of PCLK
    dont_initialize();        // don't call transfer() at time 0
  }

  // APB transfer handler — called every posedge PCLK
  void transfer() {
    // Active-low reset: clear all registers
    if (!PRESETn.read()) {
      for (int i = 0; i < NUM_REGS; i++)
        regs[i] = 0;
      PRDATA.write(0);
      PREADY.write(false);
      PSLVERR.write(false);
      return;
    }

    // Single-cycle slave — always ready
    PREADY.write(true);
    PSLVERR.write(false);

    // APB ACCESS phase: PSEL && PENABLE both high
    if (PSEL.read() && PENABLE.read()) {
      uint32_t addr = PADDR.read();

      // Check 4-byte alignment and address range
      if ((addr & 0x3) != 0 || addr > ADDR_MAX) {
        PSLVERR.write(true);
        PRDATA.write(0);
        return;
      }

      unsigned int reg_idx = addr >> 2;

      if (PWRITE.read()) {
        regs[reg_idx] = PWDATA.read();
      } else {
        PRDATA.write(regs[reg_idx]);
      }
    }
  }
};

#endif // APB_SLAVE_H
