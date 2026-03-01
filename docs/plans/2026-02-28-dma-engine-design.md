# DMA Engine Design — SystemC + Verilog Co-Simulation

## Overview

A single-channel DMA engine that transfers data between two memory-mapped addresses
without CPU intervention. The CPU programs the DMA via APB registers, triggers a
transfer, and optionally receives an interrupt on completion.

This project extends the existing APB verification environment with an AXI4-Lite
data path, demonstrating multi-protocol IP design and SystemC/Verilog co-simulation.

## Functional Specification

### Transfer Behavior
- Memory-to-memory, word-aligned, incrementing addresses only
- Source and destination addresses auto-increment by 4 bytes per word
- Maximum transfer length: 65535 words (16-bit length field)

### Register Map (APB, base address configurable)

| Offset | Name           | R/W | Description                          |
|--------|----------------|-----|--------------------------------------|
| 0x00   | `DMA_SRC_ADDR` | RW  | Source address (32-bit, word-aligned) |
| 0x04   | `DMA_DST_ADDR` | RW  | Destination address (32-bit)         |
| 0x08   | `DMA_XFER_LEN` | RW  | Transfer length in words (16-bit)    |
| 0x0C   | `DMA_CTRL`     | RW  | [0] start, [1] irq_en               |
| 0x10   | `DMA_STATUS`   | RO  | [0] busy, [1] done, [2] error       |

### State Machine

```
IDLE --[start=1]--> READ_REQ --[ARREADY]--> READ_RESP --[RVALID]-->
WRITE_REQ --[AWREADY & WREADY]--> WRITE_RESP --[BVALID]-->
  if (words_remaining > 0) --> READ_REQ
  else --> IDLE (set done, assert IRQ if irq_en)
```

### Interfaces
- **APB Slave:** Register access for CPU programming
- **AXI4-Lite Master:** Data read/write transfers
- **IRQ output:** Active-high, asserted when transfer completes (if irq_en set)

### Error Conditions
- Writing to DMA_SRC_ADDR/DST_ADDR/XFER_LEN/CTRL while busy: ignored
- AXI slave error response (RRESP/BRESP != OKAY): sets error bit, aborts transfer

## Architecture

### Dual-Model DUT

Both a SystemC behavioral model and Verilog RTL implementation, verified by
the same UVM-SystemC testbench. CMake flag `-DUSE_RTL=ON` switches between them.

```
                    +----------------------------------+
                    |         UVM Testbench            |
                    |                                  |
                    |  APB Master Agent    AXI Slave   |
                    |  (programs regs)     Agent       |
                    |       |             (memory sim) |
                    |       |                 |        |
                    +-------+-----------------+--------+
                            |                 |
                     APB Slave IF      AXI4-Lite Master IF
                            |                 |
                    +-------+-----------------+--------+
                    |        DMA Engine DUT             |
                    |   (SystemC model OR Verilated)    |
                    +----------------------------------+
```

### New Components

| Component                  | Purpose                                              |
|----------------------------|------------------------------------------------------|
| `axi_lite_if.h`           | AXI4-Lite signal-level interface bundle               |
| `axi_lite_slave_driver.h` | Responds to DMA's AXI read/write requests             |
| `axi_lite_monitor.h`      | Observes AXI transactions, broadcasts via analysis    |
| `axi_lite_slave_agent.h`  | Wires AXI driver/monitor together                     |
| `memory_model.h`          | Backing array for AXI slave (pre-loadable test data)  |
| `dma_scoreboard.h`        | Reference model: checks AXI writes against expected   |
| `dma_env.h`               | Top environment: APB agent + AXI agent + scoreboard   |
| `dma_xfer_sequence.h`     | Programs DMA regs via APB, triggers and polls transfer |
| `dma_engine.h`            | SystemC behavioral DMA model                          |
| `dma_engine.v`            | Verilog RTL DMA (for Verilator co-simulation)         |

### Reused from APB Project
- `apb_if.h` — APB signal interface
- `apb_master_driver.h` — drives APB register writes/reads
- `apb_monitor.h` — observes APB transactions
- `apb_transaction.h` — APB transaction item
- `apb_sequencer.h` — sequences for register programming

## Directory Structure

```
projects/dma/
├── CMakeLists.txt              (USE_RTL option, links APB + AXI libs)
├── rtl/
│   ├── dma_engine.h            (SystemC behavioral model)
│   ├── dma_regs.h              (APB register file sub-module)
│   ├── axi_lite_if.h           (AXI4-Lite signal interface)
│   └── verilog/
│       └── dma_engine.v        (Verilog RTL for Verilator)
├── env/
│   ├── dma_env.h               (Top-level UVM environment)
│   ├── dma_scoreboard.h        (Reference model + checker)
│   └── memory_model.h          (Backing store for AXI slave)
├── agents/
│   ├── axi_lite_slave_driver.h (Responds to DMA's AXI requests)
│   ├── axi_lite_monitor.h      (Observes AXI transactions)
│   └── axi_lite_slave_agent.h  (Wires AXI components together)
├── sequences/
│   ├── dma_base_sequence.h
│   └── dma_xfer_sequence.h     (Programs regs + triggers transfer)
├── test/
│   ├── dma_base_test.h
│   └── dma_simple_xfer_test.h  (Basic memory-to-memory test)
└── top/
    └── top.cpp                 (Clock, reset, DUT binding, UVM launch)
```

## AXI4-Lite Interface Signals

| Channel        | Signal   | Width | Direction (DMA Master) |
|----------------|----------|-------|------------------------|
| Write Address  | AWADDR   | 32    | Output                 |
|                | AWVALID  | 1     | Output                 |
|                | AWREADY  | 1     | Input                  |
| Write Data     | WDATA    | 32    | Output                 |
|                | WSTRB    | 4     | Output                 |
|                | WVALID   | 1     | Output                 |
|                | WREADY   | 1     | Input                  |
| Write Response | BRESP    | 2     | Input                  |
|                | BVALID   | 1     | Input                  |
|                | BREADY   | 1     | Output                 |
| Read Address   | ARADDR   | 32    | Output                 |
|                | ARVALID  | 1     | Output                 |
|                | ARREADY  | 1     | Input                  |
| Read Data      | RDATA    | 32    | Input                  |
|                | RRESP    | 2     | Input                  |
|                | RVALID   | 1     | Input                  |
|                | RREADY   | 1     | Output                 |

## Test Plan

### Test 1: Basic Transfer (`dma_simple_xfer_test`)
1. Pre-load source memory with known pattern (0xDEAD0000 + i)
2. Program DMA: src=0x1000, dst=0x2000, len=8
3. Set start bit via APB write to DMA_CTRL
4. Poll DMA_STATUS until done=1
5. Verify destination memory matches source
6. Scoreboard checks every AXI write against expected data

### Test 2: Single Word Transfer (edge case)
- Transfer length = 1 word

### Test 3: Back-to-Back Transfers
- Two consecutive transfers to test re-programmability

### Verification Strategy
- **Scoreboard:** Shadow memory tracks expected state, checks on AXI writes
- **Protocol checks:** AXI4-Lite handshake correctness
- **Status register:** Verify busy/done transitions via APB reads
- **VCD tracing:** Full waveform dump for debug
- **Co-simulation:** Both SystemC and Verilated models must pass identical tests

## Co-Simulation Strategy

Same proven pattern as APB project:

```
cmake .. -DUSE_RTL=OFF   # SystemC behavioral model
cmake .. -DUSE_RTL=ON    # Verilated Verilog RTL
```

Testbench code is identical — only DUT instantiation in `top.cpp` differs via
`#ifdef USE_RTL`. Both must pass the same test suite.

## Future Extensions
- Multi-channel with priority arbitration
- Fixed-address mode for peripheral FIFO access
- Scatter-gather descriptor chains
- AXI4 full burst support
