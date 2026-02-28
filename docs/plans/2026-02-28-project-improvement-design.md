# SystemC Project Improvement Design

**Date:** 2026-02-28
**Approach:** Bottom-Up (fix foundations, then build up)

## Goals

Comprehensive improvement across three areas:
1. Repository hygiene and build system
2. Practical UVM verification with an APB bus slave DUT
3. Documentation as a learning resource

## 1. Repository Cleanup & Structure

### Directory Layout

```
SystemC/
├── CMakeLists.txt
├── README.md
├── .gitignore
├── cmake/
│   └── FindSystemC.cmake
├── examples/
│   ├── CMakeLists.txt
│   ├── 01_hello_world/
│   │   ├── CMakeLists.txt
│   │   └── hello_world.cpp
│   ├── 02_or_gate/
│   │   ├── CMakeLists.txt
│   │   └── or_gate.cpp
│   └── 03_and_gate/
│       ├── CMakeLists.txt
│       └── and_gate.cpp
├── projects/
│   └── apb_slave/
│       ├── CMakeLists.txt
│       ├── rtl/
│       │   ├── apb_slave.h
│       │   └── apb_if.h
│       ├── env/
│       │   ├── apb_env.h
│       │   ├── apb_transaction.h
│       │   ├── apb_sequencer.h
│       │   └── apb_scoreboard.h
│       ├── agents/
│       │   ├── apb_master_agent.h
│       │   ├── apb_master_driver.h
│       │   ├── apb_monitor.h
│       │   └── apb_slave_agent.h
│       ├── sequences/
│       │   ├── apb_base_sequence.h
│       │   ├── apb_write_sequence.h
│       │   └── apb_read_sequence.h
│       ├── test/
│       │   ├── apb_base_test.h
│       │   └── apb_write_read_test.h
│       └── top/
│           └── top.cpp
└── docs/
    └── plans/
```

### Cleanup Actions

- Remove vim swap files from tracking
- Remove old Makefiles and run.pl (replaced by CMake)
- Update .gitignore: build/, *.vcd, *.swp, *.swo, .DS_Store, *.o
- Remove Gemini CI workflows
- Rename example directories with numeric prefixes for learning order

## 2. CMake Build System

### Root CMakeLists.txt

- CMake 3.14+, C++17 standard
- Find SystemC via custom FindSystemC.cmake
- Optionally find UVM-SystemC for projects/
- Add examples/ and projects/ as subdirectories

### FindSystemC.cmake

- Check SYSTEMC_HOME environment variable
- Auto-detect platform library directory (lib-linux64, lib-macosarm64)
- Export SystemC::systemc as imported target
- Handle UVM_SYSTEMC_HOME similarly

### Build Workflow

```bash
mkdir build && cd build
cmake .. -DSYSTEMC_HOME=/usr/local/systemc-3.0.0
make          # builds everything
make or_gate  # builds single example
```

## 3. APB Slave DUT

### APB3 Interface Signals

- PCLK, PRESETn (clock, active-low reset)
- PSEL, PENABLE, PWRITE (control)
- PADDR (32-bit address)
- PWDATA (32-bit write data)
- PRDATA (32-bit read data)
- PREADY (slave ready)
- PSLVERR (slave error)

### DUT Behavior

- 16 x 32-bit register file (0x00 to 0x3C)
- Single-cycle access (PREADY always high)
- PSLVERR on out-of-range or unaligned addresses
- All registers reset to 0x0 on PRESETn

### APB Protocol State Machine

- IDLE -> SETUP (PSEL asserted)
- SETUP -> ACCESS (PENABLE asserted)
- ACCESS -> IDLE (transfer complete)

## 4. UVM Testbench Architecture

### Components

| Component | Role |
|---|---|
| apb_transaction | uvm_sequence_item: addr, data, direction, expected response |
| apb_master_driver | Drives APB protocol signals onto interface |
| apb_monitor | Passively observes bus, sends transactions to scoreboard |
| apb_master_agent | Contains driver, monitor, sequencer |
| apb_scoreboard | Reference model: expected register state, compare on reads |
| apb_env | Instantiates agent + scoreboard, connects analysis ports |
| apb_base_test | Base test with common setup |

### Sequences

| Sequence | Purpose |
|---|---|
| apb_write_sequence | Write to all 16 registers |
| apb_read_sequence | Read back and verify data |
| apb_write_read_test | Write-then-read data integrity test |

### Data Flow

```
Sequence -> Sequencer -> Driver -> DUT signals
                                   DUT signals -> Monitor -> Scoreboard
```

## 5. Documentation & CI

### README.md

- Intro to SystemC and UVM-SystemC
- Prerequisites: SystemC 3.0, UVM-SystemC, CMake 3.14+, C++17
- Build instructions
- Learning progression: hello_world -> or_gate -> and_gate -> apb_slave

### GitHub Actions CI

- Trigger on push/PR to master
- Install SystemC from source (cached by version)
- Build all examples via CMake
- Run example binaries to verify no crashes
- Separate job for UVM testbench (requires UVM-SystemC)

## Implementation Order

1. Repository cleanup (gitignore, remove swap files, remove old build scripts)
2. CMake migration (FindSystemC, root + per-target CMakeLists)
3. Rename/reorganize example directories
4. APB slave DUT (rtl/)
5. UVM testbench components (env/, agents/, sequences/, test/)
6. Wire testbench to DUT (top/)
7. README and inline documentation
8. GitHub Actions CI
