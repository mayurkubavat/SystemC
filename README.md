# SystemC Learning Repository

A progressive learning path through SystemC and UVM-SystemC, from basic modules to a complete APB bus verification environment with Verilator RTL co-simulation.

## Prerequisites

- **SystemC 3.0+** — [Accellera download](https://systemc.org/downloads/standards/)
- **UVM-SystemC 1.0-beta6** — [GitHub](https://github.com/accellera-official/uvm-systemc) (required for `projects/apb`)
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

## Project Structure

```
examples/              Progressive SystemC examples
  01_hello_world/      SC_MODULE basics
  02_or_gate/          Combinational logic, VCD tracing
  03_and_gate/         4-state logic (sc_logic)
projects/
  apb/                 Full UVM verification environment
    rtl/               SystemC DUT model + signal interface
    rtl/verilog/       Verilog RTL DUT (for Verilator)
    agents/            Master driver, monitor, agent
    env/               Environment, scoreboard, transactions
    sequences/         Write/read stimulus sequences
    test/              UVM test classes
    top/               Top-level entry point (sc_main)
  sc_testbench/        Original UVM-SystemC testbench framework
cmake/                 CMake modules (FindSystemC)
docs/plans/            Design and implementation documents
```
