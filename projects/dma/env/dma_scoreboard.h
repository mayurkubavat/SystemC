// ----------------------------------------------------------------------------
// dma_scoreboard — Checks DMA write transactions against expected data
//
// The scoreboard subscribes to the AXI monitor's analysis port. When the
// DMA engine writes data to the destination memory (via AXI), the scoreboard
// verifies the address and data match what was originally in source memory.
//
// Before starting a DMA transfer, the test sets:
//   exp_src_addr, exp_dst_addr, exp_xfer_len — expected transfer parameters
//   src_mem — pointer to the source memory model for golden data
//
// UVM-SV equivalent:
//   class dma_scoreboard extends uvm_scoreboard;
//     uvm_analysis_imp #(axi_lite_transaction, dma_scoreboard) analysis_export;
//     function void write(axi_lite_transaction t); ... endfunction
//   endclass
// ----------------------------------------------------------------------------
#ifndef DMA_SCOREBOARD_H
#define DMA_SCOREBOARD_H

#include <uvm>
#include "axi_lite_transaction.h"
#include "memory_model.h"
#include "dma_constants.h"

class dma_scoreboard : public uvm::uvm_scoreboard {
public:
  UVM_COMPONENT_UTILS(dma_scoreboard);

  // Analysis imp — the subscribe side of the observer pattern.
  // Template args: <transaction_type, callback_class>
  // This creates a write() method that the analysis port calls automatically.
  // When the monitor calls ap.write(txn), this scoreboard's write() is invoked.
  //
  // SV equivalent:
  //   uvm_analysis_imp #(axi_lite_transaction, dma_scoreboard) analysis_export;
  //   function void write(axi_lite_transaction t);  // auto-generated callback
  //
  // Connection (done in env's connect_phase):
  //   monitor.ap  →  scoreboard.analysis_export
  //   (publisher)     (subscriber — receives every transaction)
  uvm::uvm_analysis_imp<axi_lite_transaction, dma_scoreboard> analysis_export;

  // Expected transfer parameters (set by test before DMA starts).
  // In a more sophisticated testbench, you'd use a queue of expected
  // transactions to support multiple back-to-back transfers.
  uint32_t exp_src_addr;
  uint32_t exp_dst_addr;
  uint32_t exp_xfer_len;

  // Reference to the source memory model — used to look up "golden" data.
  // The scoreboard compares what the DMA actually wrote (observed via AXI
  // monitor) against what SHOULD have been written (source memory contents).
  memory_model* src_mem;

  // Statistics — tallied across all write() callbacks
  int pass_count;
  int fail_count;
  int write_index;  // Tracks which word in the transfer we're checking

  dma_scoreboard(uvm::uvm_component_name name)
    : uvm::uvm_scoreboard(name), analysis_export("analysis_export", this),
      exp_src_addr(0), exp_dst_addr(0), exp_xfer_len(0),
      src_mem(nullptr), pass_count(0), fail_count(0), write_index(0) {}

  // write() — called automatically each time the AXI monitor's ap.write() fires.
  // This is the analysis_imp callback. The method name MUST be "write" — it's
  // required by the uvm_analysis_imp template (same as SV).
  void write(const axi_lite_transaction& txn) {
    // Only check AXI_WRITE transactions (DMA writing to destination)
    if (txn.direction != AXI_WRITE)
      return;

    // Compute expected address and data for this write beat
    uint32_t expected_addr = exp_dst_addr + write_index * 4;
    uint32_t expected_data = src_mem->read(exp_src_addr + write_index * 4);

    // Compare observed transaction against expected values
    if (txn.addr == expected_addr && txn.data == expected_data) {
      pass_count++;
      std::ostringstream ss;
      ss << "PASS: write[" << write_index << "] addr=0x" << std::hex << txn.addr
         << " data=0x" << txn.data << " (expected)";
      UVM_INFO(get_name(), ss.str(), uvm::UVM_MEDIUM);
    } else {
      fail_count++;
      std::ostringstream ss;
      ss << "FAIL: write[" << write_index << "] addr=0x" << std::hex << txn.addr
         << " data=0x" << txn.data
         << " (expected addr=0x" << expected_addr
         << " data=0x" << expected_data << ")";
      UVM_ERROR(get_name(), ss.str());
    }

    write_index++;
  }

  // report_phase — called automatically after run_phase completes.
  // UVM phase order: build → connect → run → extract → check → report → final
  // This is the right place for final pass/fail summary.
  void report_phase(uvm::uvm_phase& phase) {
    std::ostringstream ss;
    ss << "DMA Scoreboard: " << pass_count << " passed, " << fail_count
       << " failed (expected " << exp_xfer_len << " writes)";
    if (fail_count > 0 || pass_count != static_cast<int>(exp_xfer_len)) {
      UVM_ERROR(get_name(), ss.str());
    } else {
      UVM_INFO(get_name(), ss.str(), uvm::UVM_LOW);
    }
  }
};

#endif // DMA_SCOREBOARD_H
