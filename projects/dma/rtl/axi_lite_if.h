// ----------------------------------------------------------------------------
// axi_lite_if — AXI4-Lite signal-level interface
//
// In SystemVerilog UVM, you'd declare an `interface` with modports and a
// clocking block. SystemC has no native `interface` keyword for signals.
// Instead, we group the bus signals inside an sc_module and pass a pointer
// to it through uvm_config_db — this is the "virtual interface" pattern
// in UVM-SystemC.
//
// Key difference from SV:
//   SV:  virtual axi_lite_if vif;       // handle to an interface instance
//   SC:  axi_lite_if* vif;              // pointer to an sc_module with signals
//
// AXI4-Lite channels: AW (Write Address), W (Write Data), B (Write Response),
//                     AR (Read Address), R (Read Data)
// ----------------------------------------------------------------------------
#ifndef AXI_LITE_IF_H
#define AXI_LITE_IF_H

#include <systemc>
#include <cstdint>

// sc_module is the base for this class because SystemC requires sc_signals
// to live inside a module for proper elaboration and VCD tracing. We use
// sc_signal<T> (not sc_buffer<T>) because sc_signal only notifies on actual
// value changes, which is the standard choice for RTL-style modeling.
//
// Binding:
//   In top.cpp, the DUT's sc_out<T> ports bind directly to these sc_signals:
//     dut.AWADDR(axi.AWADDR);    // sc_out<uint32_t> binds to sc_signal<uint32_t>
//   The driver/monitor access them through a pointer (vif->AWADDR.read()).
//   This works because sc_in/sc_out and sc_signal share the same sc_interface.
class axi_lite_if : public sc_core::sc_module {
public:
  // AXI4-Lite has 5 independent channels. Each channel uses a valid/ready
  // handshake: the sender asserts VALID, the receiver asserts READY, and the
  // transfer completes when both are high on the same clock edge.

  // ---- Write Address channel (AW) ----
  // Master → Slave: "I want to write to this address"
  sc_core::sc_signal<uint32_t> AWADDR;
  sc_core::sc_signal<bool>     AWVALID;  // master asserts when address is valid
  sc_core::sc_signal<bool>     AWREADY;  // slave asserts when ready to accept

  // ---- Write Data channel (W) ----
  // Master → Slave: "Here is the data to write"
  sc_core::sc_signal<uint32_t> WDATA;
  sc_core::sc_signal<uint32_t> WSTRB;   // byte-lane strobes (0xF = all 4 bytes)
  sc_core::sc_signal<bool>     WVALID;
  sc_core::sc_signal<bool>     WREADY;

  // ---- Write Response channel (B) ----
  // Slave → Master: "Write completed (with status)"
  sc_core::sc_signal<uint32_t> BRESP;
  sc_core::sc_signal<bool>     BVALID;
  sc_core::sc_signal<bool>     BREADY;

  // ---- Read Address channel (AR) ----
  // Master → Slave: "I want to read from this address"
  sc_core::sc_signal<uint32_t> ARADDR;
  sc_core::sc_signal<bool>     ARVALID;
  sc_core::sc_signal<bool>     ARREADY;

  // ---- Read Data channel (R) ----
  // Slave → Master: "Here is your read data (with status)"
  sc_core::sc_signal<uint32_t> RDATA;
  sc_core::sc_signal<uint32_t> RRESP;
  sc_core::sc_signal<bool>     RVALID;
  sc_core::sc_signal<bool>     RREADY;

  // sc_module requires a constructor taking sc_module_name.
  // Each signal must be given a unique string name for VCD tracing —
  // without names, the VCD file would show unhelpful auto-generated IDs.
  axi_lite_if(const sc_core::sc_module_name& name)
    : sc_module(name),
      // Write Address channel
      AWADDR("AWADDR"), AWVALID("AWVALID"), AWREADY("AWREADY"),
      // Write Data channel
      WDATA("WDATA"), WSTRB("WSTRB"), WVALID("WVALID"), WREADY("WREADY"),
      // Write Response channel
      BRESP("BRESP"), BVALID("BVALID"), BREADY("BREADY"),
      // Read Address channel
      ARADDR("ARADDR"), ARVALID("ARVALID"), ARREADY("ARREADY"),
      // Read Data channel
      RDATA("RDATA"), RRESP("RRESP"), RVALID("RVALID"), RREADY("RREADY") {}
};

#endif // AXI_LITE_IF_H
