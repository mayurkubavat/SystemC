// ----------------------------------------------------------------------------
// dma_constants — Shared enums and constants for the DMA testbench
//
// These constants define the DMA engine's programming model (register map,
// control/status bits) and AXI protocol codes. They are shared between the
// DUT (dma_engine.h) and the verification environment, ensuring both sides
// agree on addresses and bit positions.
//
// DMA Register Map (APB address space):
//   Offset  Name       Access  Description
//   0x00    SRC_ADDR   R/W     Source start address (byte-aligned)
//   0x04    DST_ADDR   R/W     Destination start address (byte-aligned)
//   0x08    XFER_LEN   R/W     Transfer length in 32-bit words
//   0x0C    CTRL       R/W     Control: bit0=start, bit1=irq_enable
//   0x10    STATUS     R/O     Status: bit0=busy, bit1=done, bit2=error
//
// Why share constants between DUT and TB?
//   In production, the DUT uses its own register defines and the TB has a
//   separate register model (like uvm_reg in SV). For this learning example,
//   sharing a single file keeps things simple and avoids copy-paste errors.
// ----------------------------------------------------------------------------
#ifndef DMA_CONSTANTS_H
#define DMA_CONSTANTS_H

#include <cstdint>

// ---- AXI4-Lite transaction direction ----
// Used by the monitor and scoreboard to distinguish read vs write traffic.
enum axi_direction_t { AXI_READ, AXI_WRITE };

// ---- DMA register offsets (byte addresses) ----
// These match the APB PADDR values the CPU uses to program the DMA.
// Offsets are 4 bytes apart because each register is 32 bits wide.
constexpr uint32_t DMA_SRC_ADDR  = 0x00;  // Source address register
constexpr uint32_t DMA_DST_ADDR  = 0x04;  // Destination address register
constexpr uint32_t DMA_XFER_LEN  = 0x08;  // Transfer length (in 32-bit words, not bytes)
constexpr uint32_t DMA_CTRL      = 0x0C;  // Control register
constexpr uint32_t DMA_STATUS    = 0x10;  // Status register (read-only)

// ---- DMA control register bits ----
constexpr uint32_t DMA_CTRL_START  = (1 << 0);  // bit 0: writing 1 triggers transfer
constexpr uint32_t DMA_CTRL_IRQ_EN = (1 << 1);  // bit 1: enable interrupt on completion

// ---- DMA status register bits ----
// Note: BUSY and DONE are mutually exclusive — the FSM sets one or the other.
constexpr uint32_t DMA_STATUS_BUSY  = (1 << 0);  // bit 0: transfer in progress
constexpr uint32_t DMA_STATUS_DONE  = (1 << 1);  // bit 1: transfer complete
constexpr uint32_t DMA_STATUS_ERROR = (1 << 2);  // bit 2: AXI error during transfer

// ---- AXI response codes (RRESP / BRESP) ----
// AXI4-Lite uses a 2-bit response: 00=OKAY, 10=SLVERR. We only use these two.
constexpr uint32_t AXI_RESP_OKAY   = 0;  // Normal access success
constexpr uint32_t AXI_RESP_SLVERR = 2;  // Slave error

#endif // DMA_CONSTANTS_H
