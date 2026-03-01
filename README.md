# SystemC Learning Repository

A progressive learning path through SystemC and UVM-SystemC, from basic modules to complete verification environments with Verilator RTL co-simulation. Includes an APB3 slave testbench and a DMA engine testbench with AXI4-Lite master interface.

## Prerequisites

- **SystemC 3.0+** — [Accellera download](https://systemc.org/downloads/standards/)
- **UVM-SystemC 1.0-beta6** — [GitHub](https://github.com/accellera-official/uvm-systemc) (required for `projects/`)
- **CMake 3.14+**
- **C++17 compiler** (g++ or clang++)
- **Verilator 5+** *(optional)* — for RTL co-simulation
- **GTKWave** *(optional)* — for viewing VCD waveforms

> **macOS ARM64 note:** UVM-SystemC `configure.ac` does not yet support Apple Silicon out of the box. A 4-line patch is needed — see [accellera-official/uvm-systemc#346](https://github.com/accellera-official/uvm-systemc/issues/346).

## Build

### Examples only (no UVM required)

```bash
mkdir build && cd build
cmake .. -DSYSTEMC_HOME=/path/to/systemc
make
```

### With UVM testbench (SystemC model DUT)

```bash
cmake .. -DSYSTEMC_HOME=/path/to/systemc -DUVM_SYSTEMC_HOME=/path/to/uvm-systemc
make
```

### With RTL DUT (via Verilator)

```bash
cmake .. -DSYSTEMC_HOME=/path/to/systemc -DUVM_SYSTEMC_HOME=/path/to/uvm-systemc -DUSE_RTL=ON
make
```

The same UVM testbench runs against either the SystemC behavioral model or the Verilated RTL — controlled by the `USE_RTL` flag.

## Learning Progression

### 1. Hello World (`examples/01_hello_world`)

Your first SystemC program. Introduces `SC_MODULE`, `SC_CTOR`, and `sc_main`.

```bash
./build/examples/01_hello_world/hello_world
```

### 2. OR Gate (`examples/02_or_gate`)

Combinational logic with `SC_METHOD`, port binding, sensitivity lists, and VCD waveform tracing.

```bash
./build/examples/02_or_gate/or_gate
gtkwave orGate.vcd   # view waveforms
```

### 3. AND Gate (`examples/03_and_gate`)

Introduces `sc_logic` (4-state: 0, 1, X, Z) instead of `bool`, important for modeling uninitialized hardware values.

```bash
./build/examples/03_and_gate/and_gate
```

### 4. APB Slave Verification (`projects/apb`)

A complete UVM-SystemC testbench verifying an AMBA APB3 slave with a 16x32-bit register file. Demonstrates transaction-level verification methodology: driver, monitor, scoreboard, sequences, and config_db-based virtual interface passing.

```bash
./build/projects/apb/sim
gtkwave apb_slave.vcd   # view APB bus waveforms
```

**APB testbench architecture:**

```
sc_main (top.cpp)
 +-- clk, resetn, reset_gen
 +-- apb_if (signal-level interface)
 +-- apb_slave DUT (SystemC model or Verilated RTL)
 +-- uvm::run_test("apb_rw_test")
      +-- apb_env
           +-- apb_master_agent
           |    +-- apb_sequencer
           |    +-- apb_master_driver  --> drives apb_if
           |    +-- apb_monitor        --> observes apb_if
           +-- apb_scoreboard         <-- analysis port from monitor
```

### 5. DMA Engine Verification (`projects/dma`)

A single-channel DMA engine with APB3 slave (register programming) and AXI4-Lite master (data transfers) interfaces. The UVM-SystemC testbench reuses the APB agent from `projects/apb` and adds a reactive AXI4-Lite slave agent with memory model. Extensively commented for DV engineers and students learning SystemC.

```bash
# SystemC behavioral model
./build/projects/dma/dma_sim

# Verilator RTL co-simulation (rebuild with -DUSE_RTL=ON)
./build/projects/dma/dma_sim

gtkwave dma_engine.vcd   # view AXI + APB waveforms
```

**DMA register map:**

| Offset | Register   | Description                           |
|--------|-----------|---------------------------------------|
| 0x00   | SRC_ADDR  | Source address for DMA read           |
| 0x04   | DST_ADDR  | Destination address for DMA write     |
| 0x08   | XFER_LEN  | Number of 32-bit words to transfer    |
| 0x0C   | CTRL      | bit 0: START, bit 1: IRQ_EN          |
| 0x10   | STATUS    | bit 0: BUSY, bit 1: DONE, bit 2: ERR |

**DMA testbench architecture:**

```
sc_main (top.cpp)
 +-- clk, resetn, reset_gen
 +-- apb_if (APB signal interface)
 +-- axi_lite_if (AXI4-Lite signal interface)
 +-- dma_engine DUT (SystemC model or Verilated RTL)
 +-- uvm::run_test("dma_simple_xfer_test")
      +-- dma_env
           +-- apb_master_agent (reused from projects/apb)
           |    +-- apb_sequencer
           |    +-- apb_master_driver  --> drives apb_if (register writes)
           |    +-- apb_monitor        --> observes apb_if
           +-- axi_lite_slave_agent (reactive — no sequencer)
           |    +-- axi_lite_slave_driver  --> responds to AXI requests
           |    +-- axi_lite_monitor       --> observes AXI bus
           +-- memory_model               <-- backing store for AXI slave
           +-- dma_scoreboard             <-- verifies AXI writes vs expected
```

**Key SystemC concepts demonstrated:**
- SC_METHOD vs SC_THREAD process types and when to use each
- Delta cycle semantics and `wait(SC_ZERO_TIME)` for process alignment
- Reactive (responder) agent pattern without a sequencer
- AXI4-Lite valid/ready handshake protocol
- Cross-project component reuse (APB agent shared between projects)

## Project Structure

```
examples/                Progressive SystemC examples
  01_hello_world/        SC_MODULE basics
  02_or_gate/            Combinational logic, VCD tracing
  03_and_gate/           4-state logic (sc_logic)
projects/
  apb/                   APB3 slave UVM verification environment
    rtl/                 SystemC DUT model + signal interface
    rtl/verilog/         Verilog RTL DUT (for Verilator)
    agents/              Master driver, monitor, agent
    env/                 Environment, scoreboard, transactions
    sequences/           Write/read stimulus sequences
    test/                UVM test classes
    top/                 Top-level entry point (sc_main)
  dma/                   DMA engine UVM verification environment
    rtl/                 SystemC DUT model + AXI4-Lite interface
    rtl/verilog/         Verilog RTL DUT (for Verilator)
    agents/              AXI4-Lite slave driver, monitor, agent
    env/                 Environment, scoreboard, memory model
    sequences/           DMA transfer sequences (reuses APB transactions)
    test/                UVM test classes
    top/                 Top-level entry point (sc_main)
cmake/                   CMake modules (FindSystemC)
docs/plans/              Design and implementation documents
```
