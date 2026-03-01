// ----------------------------------------------------------------------------
// dma_simple_xfer_test — Basic memory-to-memory DMA transfer test
//
// 1. Pre-loads source memory with known pattern (0xDEAD0000 + i)
// 2. Programs DMA: src=0x1000, dst=0x2000, len=8 words
// 3. Triggers transfer and waits for completion
// 4. Scoreboard automatically verifies each AXI write
// ----------------------------------------------------------------------------
#ifndef DMA_SIMPLE_XFER_TEST_H
#define DMA_SIMPLE_XFER_TEST_H

#include <uvm>
#include "dma_base_test.h"
#include "dma_xfer_sequence.h"

class dma_simple_xfer_test : public dma_base_test {
public:
  UVM_COMPONENT_UTILS(dma_simple_xfer_test);

  dma_simple_xfer_test(uvm::uvm_component_name name)
    : dma_base_test(name) {}

  // run_phase — where the actual test stimulus happens.
  // This runs as an SC_THREAD, so we can call wait() and sc_core::wait().
  void run_phase(uvm::uvm_phase& phase) {
    // *** Objection mechanism ***
    // UVM simulation ends when ALL objections are dropped. If we don't raise
    // an objection, run_phase ends immediately (before our test runs!).
    // raise_objection = "I'm not done yet, keep simulating"
    // drop_objection  = "I'm done, you can end simulation"
    //
    // SV equivalent: phase.raise_objection(this);
    phase.raise_objection(this, "dma_simple_xfer_test");

    // Wait for reset to deassert before programming DMA registers.
    // We read the resetn signal directly from config_db rather than
    // through a virtual interface — this is a simple approach for reset sync.
    sc_core::sc_signal<bool>* resetn = nullptr;
    if (uvm::uvm_config_db<sc_core::sc_signal<bool>*>::get(this, "", "resetn", resetn)) {
      while (!resetn->read())
        sc_core::wait(10, sc_core::SC_NS);
      sc_core::wait(10, sc_core::SC_NS);  // One extra cycle for stability
    }

    // ---- Test setup ----
    uint32_t src_addr = 0x1000;
    uint32_t dst_addr = 0x2000;
    uint32_t xfer_len = 8;  // 8 words = 32 bytes

    // Pre-load source memory with a recognizable pattern.
    // The DMA will read from here (via AXI) and write to destination.
    // mem[0x1000]=0xDEAD0000, mem[0x1004]=0xDEAD0001, ... mem[0x101C]=0xDEAD0007
    env->mem->load_pattern(src_addr, xfer_len, 0xDEAD0000);

    // Tell the scoreboard what to expect. The scoreboard will compare each
    // AXI write transaction against golden data from source memory.
    env->scoreboard->exp_src_addr = src_addr;
    env->scoreboard->exp_dst_addr = dst_addr;
    env->scoreboard->exp_xfer_len = xfer_len;

    // ---- Run the DMA transfer ----
    // Create the sequence via factory, configure it, and start it on the
    // APB agent's sequencer. start() blocks until the sequence completes.
    dma_xfer_sequence* seq = dma_xfer_sequence::type_id::create("seq");
    seq->src_addr = src_addr;
    seq->dst_addr = dst_addr;
    seq->xfer_len = xfer_len;
    seq->start(env->apb_agent->sequencer);  // Blocking: runs entire sequence

    // Allow extra time for the monitor to observe the last AXI transactions.
    // Without this, simulation might end before the monitor's polling loop
    // catches the final BVALID/BREADY handshake.
    sc_core::wait(50, sc_core::SC_NS);

    // Signal that this test is complete — simulation can end.
    phase.drop_objection(this, "dma_simple_xfer_test");
  }
};

#endif // DMA_SIMPLE_XFER_TEST_H
