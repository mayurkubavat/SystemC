// ----------------------------------------------------------------------------
// dma_xfer_sequence — Programs DMA registers and executes a transfer
//
// 1. Writes SRC_ADDR, DST_ADDR, XFER_LEN via APB
// 2. Writes CTRL with start bit to trigger the transfer
// 3. Waits a calculated amount of time for the transfer to complete
//
// Why timed wait instead of polling?
//   In SV UVM, the sequence would poll the STATUS register via APB reads
//   until DONE bit is set. In UVM-SystemC, reg_read() doesn't return actual
//   read data (see dma_base_sequence.h for explanation). Instead, we compute
//   a generous timeout based on transfer length. The scoreboard verifies
//   correctness independently by monitoring the AXI bus.
// ----------------------------------------------------------------------------
#ifndef DMA_XFER_SEQUENCE_H
#define DMA_XFER_SEQUENCE_H

#include <uvm>
#include <sstream>
#include <iomanip>
#include "dma_base_sequence.h"
#include "dma_constants.h"

class dma_xfer_sequence : public dma_base_sequence {
public:
  UVM_OBJECT_UTILS(dma_xfer_sequence);

  // Transfer parameters — set by the test before starting
  uint32_t src_addr;
  uint32_t dst_addr;
  uint32_t xfer_len;
  bool     irq_en;

  dma_xfer_sequence(const std::string& name = "dma_xfer_sequence")
    : dma_base_sequence(name), src_addr(0), dst_addr(0),
      xfer_len(0), irq_en(false) {}

  // body() is the sequence's main task — called when the test starts the
  // sequence on a sequencer: seq->start(env->apb_agent->sequencer).
  // In SV: `virtual task body();`
  void body() {
    std::ostringstream ss;
    ss << "DMA transfer: src=0x" << std::hex << src_addr
       << " dst=0x" << dst_addr << " len=" << std::dec << xfer_len;
    UVM_INFO("DMA_SEQ", ss.str(), uvm::UVM_LOW);

    // Program registers
    reg_write(DMA_SRC_ADDR, src_addr);
    reg_write(DMA_DST_ADDR, dst_addr);
    reg_write(DMA_XFER_LEN, xfer_len);

    // Trigger transfer
    uint32_t ctrl_val = DMA_CTRL_START;
    if (irq_en) ctrl_val |= DMA_CTRL_IRQ_EN;
    reg_write(DMA_CTRL, ctrl_val);

    // Transfer is now in progress. Completion detection (via IRQ or polling)
    // is handled by the test, not the sequence. The sequence's job is just
    // to program registers and trigger the transfer.
    UVM_INFO("DMA_SEQ", "Transfer triggered, completion handled by test", uvm::UVM_LOW);
  }
};

#endif // DMA_XFER_SEQUENCE_H
