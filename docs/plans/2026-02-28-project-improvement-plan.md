# SystemC Project Improvement — Implementation Plan

> **For Claude:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task.

**Goal:** Transform this SystemC learning repository into a polished educational resource with a working APB slave UVM testbench and modern CMake build system.

**Architecture:** Bottom-up approach — clean repo hygiene first, then CMake build system, then APB slave DUT with UVM testbench, finally documentation and CI. Each task builds on the previous.

**Tech Stack:** SystemC 3.0, UVM-SystemC, CMake 3.14+, C++17, GitHub Actions

---

## Task 1: Repository Cleanup — Update .gitignore

**Files:**
- Modify: `.gitignore`

**Step 1: Replace .gitignore with comprehensive version**

Write the following to `.gitignore`:

```gitignore
# Build artifacts
build/
*.o
*.d
sim

# SystemC local installs
/systemc-2.3.1

# Waveform dumps
*.vcd
*.wlf
*.fsdb

# Editor swap files
*.swp
*.swo
*~

# OS files
.DS_Store
Thumbs.db

# Gemini
.gemini/
gha-creds-*.json

# IDE
.vscode/
.idea/
```

**Step 2: Commit**

```bash
git add .gitignore
git commit -m "chore: update .gitignore with build, editor, and OS patterns"
```

---

## Task 2: Repository Cleanup — Remove Tracked Swap Files and Old Build Scripts

**Files:**
- Delete: `examples/basic_gates/or_gate/.Makefile.swo`
- Delete: `examples/basic_gates/or_gate/.Makefile.swp`
- Delete: `projects/sc_testbench/sim/.Makefile.swp`
- Delete: `projects/sc_testbench/sim/.run.pl.swp`
- Delete: `examples/basic_gates/or_gate/orGate.vcd`
- Delete: `examples/hello_world/Makefile`
- Delete: `examples/basic_gates/or_gate/Makefile`
- Delete: `examples/basic_gates/and_gate/Makefile`
- Delete: `projects/sc_testbench/sim/Makefile`
- Delete: `projects/sc_testbench/sim/run.pl`

**Step 1: Remove swap files and generated files from tracking**

```bash
git rm --cached examples/basic_gates/or_gate/.Makefile.swo
git rm --cached examples/basic_gates/or_gate/.Makefile.swp
git rm --cached projects/sc_testbench/sim/.Makefile.swp
git rm --cached projects/sc_testbench/sim/.run.pl.swp
git rm --cached examples/basic_gates/or_gate/orGate.vcd
```

**Step 2: Remove old Makefiles and run.pl**

```bash
git rm examples/hello_world/Makefile
git rm examples/basic_gates/or_gate/Makefile
git rm examples/basic_gates/and_gate/Makefile
git rm projects/sc_testbench/sim/Makefile
git rm projects/sc_testbench/sim/run.pl
```

**Step 3: Commit**

```bash
git commit -m "chore: remove swap files, VCD dumps, and old Makefiles

Replaced by CMake build system in subsequent commits."
```

---

## Task 3: Repository Cleanup — Remove Gemini Workflows

**Files:**
- Delete: `.github/workflows/gemini-dispatch.yml`
- Delete: `.github/workflows/gemini-invoke.yml`
- Delete: `.github/workflows/gemini-review.yml`
- Delete: `.github/workflows/gemini-scheduled-triage.yml`
- Delete: `.github/workflows/gemini-triage.yml`

**Step 1: Remove all Gemini workflow files**

```bash
git rm .github/workflows/gemini-dispatch.yml
git rm .github/workflows/gemini-invoke.yml
git rm .github/workflows/gemini-review.yml
git rm .github/workflows/gemini-scheduled-triage.yml
git rm .github/workflows/gemini-triage.yml
rmdir .github/workflows .github 2>/dev/null || true
```

**Step 2: Commit**

```bash
git commit -m "chore: remove Gemini CI workflows

Will be replaced by build/test CI in a later task."
```

---

## Task 4: Reorganize Example Directories

**Files:**
- Move: `examples/hello_world/` → `examples/01_hello_world/`
- Move: `examples/basic_gates/or_gate/` → `examples/02_or_gate/`
- Move: `examples/basic_gates/and_gate/` → `examples/03_and_gate/`
- Delete: `examples/basic_gates/` (empty parent after moves)

**Step 1: Restructure examples with numbered prefixes**

```bash
git mv examples/hello_world examples/01_hello_world
git mv examples/basic_gates/or_gate examples/02_or_gate
git mv examples/basic_gates/and_gate examples/03_and_gate
git rm -r examples/basic_gates
```

Note: If `examples/basic_gates/` has swap files on disk (untracked), remove them manually first: `rm -f examples/basic_gates/and_gate/.Makefile.sw*`

**Step 2: Verify structure**

```bash
find examples -type f -name "*.cpp"
```

Expected output:
```
examples/01_hello_world/hello_world.cpp
examples/02_or_gate/or_gate.cpp
examples/03_and_gate/and_gate.cpp
```

**Step 3: Commit**

```bash
git commit -m "refactor: reorganize examples with numbered prefixes for learning order

01_hello_world -> 02_or_gate -> 03_and_gate progression."
```

---

## Task 5: CMake — Create FindSystemC.cmake Module

**Files:**
- Create: `cmake/FindSystemC.cmake`

**Step 1: Write the FindSystemC module**

Create `cmake/FindSystemC.cmake`:

```cmake
# FindSystemC.cmake
# Locates SystemC and optionally UVM-SystemC libraries.
#
# Usage:
#   set(SYSTEMC_HOME "/path/to/systemc" CACHE PATH "SystemC installation")
#   find_package(SystemC REQUIRED)
#   target_link_libraries(my_target SystemC::systemc)
#
# Provides:
#   SystemC::systemc    - imported target for SystemC
#   SystemC::uvm        - imported target for UVM-SystemC (optional)
#   SystemC_FOUND       - TRUE if SystemC was found
#   UVM_SystemC_FOUND   - TRUE if UVM-SystemC was found

# --- Detect platform library directory suffix ---
if(CMAKE_SYSTEM_NAME STREQUAL "Darwin" AND CMAKE_SYSTEM_PROCESSOR STREQUAL "arm64")
    set(_SC_LIB_SUFFIX "lib-macosarm64")
elseif(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    set(_SC_LIB_SUFFIX "lib-macosx64")
elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux" AND CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(_SC_LIB_SUFFIX "lib-linux64")
elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    set(_SC_LIB_SUFFIX "lib-linux")
else()
    set(_SC_LIB_SUFFIX "lib")
endif()

# --- Find SystemC ---
if(NOT SYSTEMC_HOME)
    set(SYSTEMC_HOME $ENV{SYSTEMC_HOME})
endif()

find_path(SystemC_INCLUDE_DIR
    NAMES systemc.h
    PATHS ${SYSTEMC_HOME}/include
    NO_DEFAULT_PATH
)

find_library(SystemC_LIBRARY
    NAMES systemc
    PATHS ${SYSTEMC_HOME}/${_SC_LIB_SUFFIX}
          ${SYSTEMC_HOME}/lib
    NO_DEFAULT_PATH
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(SystemC
    REQUIRED_VARS SystemC_LIBRARY SystemC_INCLUDE_DIR
)

if(SystemC_FOUND AND NOT TARGET SystemC::systemc)
    add_library(SystemC::systemc UNKNOWN IMPORTED)
    set_target_properties(SystemC::systemc PROPERTIES
        IMPORTED_LOCATION "${SystemC_LIBRARY}"
        INTERFACE_INCLUDE_DIRECTORIES "${SystemC_INCLUDE_DIR}"
    )
endif()

# --- Find UVM-SystemC (optional) ---
if(NOT UVM_SYSTEMC_HOME)
    set(UVM_SYSTEMC_HOME $ENV{UVM_SYSTEMC_HOME})
endif()

if(UVM_SYSTEMC_HOME)
    find_path(UVM_SystemC_INCLUDE_DIR
        NAMES uvm.h
        PATHS ${UVM_SYSTEMC_HOME}/include
        NO_DEFAULT_PATH
    )

    find_library(UVM_SystemC_LIBRARY
        NAMES uvm-systemc
        PATHS ${UVM_SYSTEMC_HOME}/${_SC_LIB_SUFFIX}
              ${UVM_SYSTEMC_HOME}/lib
        NO_DEFAULT_PATH
    )

    if(UVM_SystemC_INCLUDE_DIR AND UVM_SystemC_LIBRARY)
        set(UVM_SystemC_FOUND TRUE)
        if(NOT TARGET SystemC::uvm)
            add_library(SystemC::uvm UNKNOWN IMPORTED)
            set_target_properties(SystemC::uvm PROPERTIES
                IMPORTED_LOCATION "${UVM_SystemC_LIBRARY}"
                INTERFACE_INCLUDE_DIRECTORIES "${UVM_SystemC_INCLUDE_DIR}"
                INTERFACE_LINK_LIBRARIES SystemC::systemc
            )
        endif()
    endif()
endif()
```

**Step 2: Commit**

```bash
git add cmake/FindSystemC.cmake
git commit -m "build: add FindSystemC.cmake with cross-platform detection

Auto-detects lib-linux64, lib-macosarm64, etc. based on platform.
Provides SystemC::systemc and optional SystemC::uvm imported targets."
```

---

## Task 6: CMake — Root and Example CMakeLists.txt

**Files:**
- Create: `CMakeLists.txt`
- Create: `examples/CMakeLists.txt`
- Create: `examples/01_hello_world/CMakeLists.txt`
- Create: `examples/02_or_gate/CMakeLists.txt`
- Create: `examples/03_and_gate/CMakeLists.txt`

**Step 1: Write root CMakeLists.txt**

Create `CMakeLists.txt`:

```cmake
cmake_minimum_required(VERSION 3.14)
project(SystemC_Learning LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Allow user to set SYSTEMC_HOME via -D or environment
set(SYSTEMC_HOME "" CACHE PATH "Path to SystemC installation")

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")
find_package(SystemC REQUIRED)

add_subdirectory(examples)

if(UVM_SystemC_FOUND)
    add_subdirectory(projects)
else()
    message(STATUS "UVM-SystemC not found — skipping projects/. Set UVM_SYSTEMC_HOME to enable.")
endif()
```

**Step 2: Write examples/CMakeLists.txt**

Create `examples/CMakeLists.txt`:

```cmake
add_subdirectory(01_hello_world)
add_subdirectory(02_or_gate)
add_subdirectory(03_and_gate)
```

**Step 3: Write per-example CMakeLists.txt files**

Create `examples/01_hello_world/CMakeLists.txt`:

```cmake
add_executable(hello_world hello_world.cpp)
target_link_libraries(hello_world SystemC::systemc)
```

Create `examples/02_or_gate/CMakeLists.txt`:

```cmake
add_executable(or_gate or_gate.cpp)
target_link_libraries(or_gate SystemC::systemc)
```

Create `examples/03_and_gate/CMakeLists.txt`:

```cmake
add_executable(and_gate and_gate.cpp)
target_link_libraries(and_gate SystemC::systemc)
```

**Step 4: Build and verify examples compile**

```bash
mkdir -p build && cd build
cmake .. -DSYSTEMC_HOME=/usr/local/systemc-3.0.0
make -j$(nproc)
```

Expected: All three targets compile without errors.

**Step 5: Run each example to verify**

```bash
cd build
./examples/01_hello_world/hello_world
./examples/02_or_gate/or_gate
./examples/03_and_gate/and_gate
```

Expected:
- `hello_world` prints "My first SystemC program!"
- `or_gate` prints truth table and exits cleanly
- `and_gate` prints truth table and exits cleanly

**Step 6: Commit**

```bash
git add CMakeLists.txt examples/CMakeLists.txt examples/*/CMakeLists.txt
git commit -m "build: add CMake build system for all examples

Root CMakeLists with SystemC auto-detection.
Per-example CMakeLists for hello_world, or_gate, and_gate."
```

---

## Task 7: APB Slave DUT — Interface and Slave Module

**Files:**
- Create: `projects/apb_slave/rtl/apb_if.h`
- Create: `projects/apb_slave/rtl/apb_slave.h`

**Step 1: Write APB interface signal bundle**

Create `projects/apb_slave/rtl/apb_if.h`:

```cpp
#ifndef APB_IF_H
#define APB_IF_H

#include <systemc>

// APB3 signal interface — connects DUT to testbench
struct apb_if {
    sc_core::sc_signal<bool>          PSEL;
    sc_core::sc_signal<bool>          PENABLE;
    sc_core::sc_signal<bool>          PWRITE;
    sc_core::sc_signal<sc_dt::sc_uint<32>> PADDR;
    sc_core::sc_signal<sc_dt::sc_uint<32>> PWDATA;
    sc_core::sc_signal<sc_dt::sc_uint<32>> PRDATA;
    sc_core::sc_signal<bool>          PREADY;
    sc_core::sc_signal<bool>          PSLVERR;
};

#endif // APB_IF_H
```

**Step 2: Write APB slave DUT**

Create `projects/apb_slave/rtl/apb_slave.h`:

```cpp
#ifndef APB_SLAVE_H
#define APB_SLAVE_H

#include <systemc>

// APB3 slave with 16 x 32-bit register file.
// Single-cycle access (PREADY always high).
// Returns PSLVERR on out-of-range or unaligned addresses.
SC_MODULE(apb_slave) {

    // Clock and reset
    sc_core::sc_in<bool> PCLK;
    sc_core::sc_in<bool> PRESETn;

    // APB3 interface
    sc_core::sc_in<bool>               PSEL;
    sc_core::sc_in<bool>               PENABLE;
    sc_core::sc_in<bool>               PWRITE;
    sc_core::sc_in<sc_dt::sc_uint<32>> PADDR;
    sc_core::sc_in<sc_dt::sc_uint<32>> PWDATA;

    sc_core::sc_out<sc_dt::sc_uint<32>> PRDATA;
    sc_core::sc_out<bool>               PREADY;
    sc_core::sc_out<bool>               PSLVERR;

    static const int NUM_REGS = 16;
    static const int ADDR_MAX = (NUM_REGS - 1) * 4; // 0x3C
    sc_dt::sc_uint<32> regs[NUM_REGS];

    SC_CTOR(apb_slave) {
        SC_METHOD(transfer);
        sensitive << PCLK.pos();
        dont_initialize();
    }

    void transfer() {
        if (!PRESETn.read()) {
            // Reset: clear all registers and outputs
            for (int i = 0; i < NUM_REGS; i++)
                regs[i] = 0;
            PRDATA.write(0);
            PREADY.write(false);
            PSLVERR.write(false);
            return;
        }

        // Default: always ready, no error
        PREADY.write(true);
        PSLVERR.write(false);

        if (PSEL.read() && PENABLE.read()) {
            sc_dt::sc_uint<32> addr = PADDR.read();

            // Check alignment (must be 4-byte aligned) and range
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
```

**Step 3: Commit**

```bash
git add projects/apb_slave/rtl/apb_if.h projects/apb_slave/rtl/apb_slave.h
git commit -m "feat: add APB3 slave DUT with 16x32-bit register file

Implements AMBA APB3 protocol with:
- Single-cycle access (PREADY always high)
- PSLVERR on out-of-range or unaligned addresses
- Synchronous reset"
```

---

## Task 8: APB Testbench — Transaction and Constants

**Files:**
- Create: `projects/apb_slave/env/apb_constants.h`
- Create: `projects/apb_slave/env/apb_transaction.h`

**Step 1: Write APB constants**

Create `projects/apb_slave/env/apb_constants.h`:

```cpp
#ifndef APB_CONSTANTS_H
#define APB_CONSTANTS_H

enum apb_direction_t { APB_READ, APB_WRITE };

#endif // APB_CONSTANTS_H
```

**Step 2: Write APB transaction**

Create `projects/apb_slave/env/apb_transaction.h`:

```cpp
#ifndef APB_TRANSACTION_H
#define APB_TRANSACTION_H

#include <systemc>
#include <uvm>
#include <iomanip>
#include <sstream>

#include "apb_constants.h"

class apb_transaction : public uvm::uvm_sequence_item {
public:
    sc_dt::sc_uint<32> addr;
    sc_dt::sc_uint<32> data;
    apb_direction_t direction;
    bool error; // Set by monitor after observing PSLVERR

    apb_transaction(const std::string& name = "apb_transaction")
        : uvm::uvm_sequence_item(name), addr(0), data(0),
          direction(APB_READ), error(false) {}

    UVM_OBJECT_UTILS(apb_transaction);

    virtual void do_copy(const uvm::uvm_object& rhs) {
        const apb_transaction* rhs_ = dynamic_cast<const apb_transaction*>(&rhs);
        if (!rhs_)
            UVM_FATAL("do_copy", "cast failed");
        uvm_sequence_item::do_copy(rhs);
        addr = rhs_->addr;
        data = rhs_->data;
        direction = rhs_->direction;
        error = rhs_->error;
    }

    virtual bool do_compare(const uvm::uvm_object& rhs,
                            const uvm::uvm_comparer* comparer) {
        const apb_transaction* rhs_ = dynamic_cast<const apb_transaction*>(&rhs);
        if (!rhs_)
            UVM_FATAL("do_compare", "cast failed");
        return (addr == rhs_->addr && data == rhs_->data &&
                direction == rhs_->direction);
    }

    void do_print(const uvm::uvm_printer& printer) const {
        printer.print_string("dir", (direction == APB_WRITE) ? "WRITE" : "READ");
        printer.print_field_int("addr", addr);
        printer.print_field_int("data", data);
        printer.print_field_int("error", error);
    }

    std::string convert2string() {
        std::ostringstream ss;
        ss << (direction == APB_WRITE ? "WRITE" : "READ ")
           << " addr=0x" << std::hex << std::setw(2) << std::setfill('0') << addr
           << " data=0x" << std::setw(8) << std::setfill('0') << data;
        if (error) ss << " [ERR]";
        return ss.str();
    }
};

#endif // APB_TRANSACTION_H
```

**Step 3: Commit**

```bash
git add projects/apb_slave/env/apb_constants.h projects/apb_slave/env/apb_transaction.h
git commit -m "feat: add APB transaction and constants for UVM testbench"
```

---

## Task 9: APB Testbench — Sequencer and Sequences

**Files:**
- Create: `projects/apb_slave/env/apb_sequencer.h`
- Create: `projects/apb_slave/sequences/apb_base_sequence.h`
- Create: `projects/apb_slave/sequences/apb_write_sequence.h`
- Create: `projects/apb_slave/sequences/apb_read_sequence.h`

**Step 1: Write sequencer**

Create `projects/apb_slave/env/apb_sequencer.h`:

```cpp
#ifndef APB_SEQUENCER_H
#define APB_SEQUENCER_H

#include <uvm>
#include "apb_transaction.h"

class apb_sequencer : public uvm::uvm_sequencer<apb_transaction> {
public:
    UVM_COMPONENT_UTILS(apb_sequencer);

    apb_sequencer(uvm::uvm_component_name name)
        : uvm::uvm_sequencer<apb_transaction>(name) {}
};

#endif // APB_SEQUENCER_H
```

**Step 2: Write base sequence**

Create `projects/apb_slave/sequences/apb_base_sequence.h`:

```cpp
#ifndef APB_BASE_SEQUENCE_H
#define APB_BASE_SEQUENCE_H

#include <uvm>
#include "apb_transaction.h"

class apb_base_sequence : public uvm::uvm_sequence<apb_transaction> {
public:
    UVM_OBJECT_UTILS(apb_base_sequence);

    apb_base_sequence(const std::string& name = "apb_base_sequence")
        : uvm::uvm_sequence<apb_transaction>(name) {}

    virtual ~apb_base_sequence() {}
};

#endif // APB_BASE_SEQUENCE_H
```

**Step 3: Write write sequence**

Create `projects/apb_slave/sequences/apb_write_sequence.h`:

```cpp
#ifndef APB_WRITE_SEQUENCE_H
#define APB_WRITE_SEQUENCE_H

#include <uvm>
#include "apb_base_sequence.h"

// Writes incrementing data to all 16 registers (addr 0x00..0x3C)
class apb_write_sequence : public apb_base_sequence {
public:
    UVM_OBJECT_UTILS(apb_write_sequence);

    apb_write_sequence(const std::string& name = "apb_write_sequence")
        : apb_base_sequence(name) {}

    void body() {
        for (int i = 0; i < 16; i++) {
            apb_transaction* txn = apb_transaction::type_id::create("txn");
            txn->direction = APB_WRITE;
            txn->addr = i * 4;
            txn->data = 0xA000 + i;

            start_item(txn);
            finish_item(txn);
        }
    }
};

#endif // APB_WRITE_SEQUENCE_H
```

**Step 4: Write read sequence**

Create `projects/apb_slave/sequences/apb_read_sequence.h`:

```cpp
#ifndef APB_READ_SEQUENCE_H
#define APB_READ_SEQUENCE_H

#include <uvm>
#include "apb_base_sequence.h"

// Reads all 16 registers (addr 0x00..0x3C)
class apb_read_sequence : public apb_base_sequence {
public:
    UVM_OBJECT_UTILS(apb_read_sequence);

    apb_read_sequence(const std::string& name = "apb_read_sequence")
        : apb_base_sequence(name) {}

    void body() {
        for (int i = 0; i < 16; i++) {
            apb_transaction* txn = apb_transaction::type_id::create("txn");
            txn->direction = APB_READ;
            txn->addr = i * 4;

            start_item(txn);
            finish_item(txn);
        }
    }
};

#endif // APB_READ_SEQUENCE_H
```

**Step 5: Commit**

```bash
git add projects/apb_slave/env/apb_sequencer.h \
       projects/apb_slave/sequences/apb_base_sequence.h \
       projects/apb_slave/sequences/apb_write_sequence.h \
       projects/apb_slave/sequences/apb_read_sequence.h
git commit -m "feat: add APB sequencer and write/read sequences"
```

---

## Task 10: APB Testbench — Master Driver

**Files:**
- Create: `projects/apb_slave/agents/apb_master_driver.h`

**Step 1: Write the master driver**

This is the core protocol component — it translates transactions into APB pin wiggles.

Create `projects/apb_slave/agents/apb_master_driver.h`:

```cpp
#ifndef APB_MASTER_DRIVER_H
#define APB_MASTER_DRIVER_H

#include <systemc>
#include <uvm>
#include "apb_transaction.h"
#include "apb_if.h"

class apb_master_driver : public uvm::uvm_driver<apb_transaction> {
public:
    UVM_COMPONENT_UTILS(apb_master_driver);

    // Pointer to shared APB interface signals
    apb_if* vif;

    // Clock period for timing
    sc_core::sc_time clk_period;

    apb_master_driver(uvm::uvm_component_name name)
        : uvm::uvm_driver<apb_transaction>(name), vif(nullptr),
          clk_period(10, sc_core::SC_NS) {}

    void build_phase(uvm::uvm_phase& phase) {
        uvm::uvm_driver<apb_transaction>::build_phase(phase);
    }

    void run_phase(uvm::uvm_phase& phase) {
        apb_transaction* txn = nullptr;

        while (true) {
            // Get next transaction from sequencer
            seq_item_port->get_next_item(txn);
            drive_transfer(txn);
            seq_item_port->item_done();
        }
    }

private:
    void drive_transfer(apb_transaction* txn) {
        // SETUP phase: assert PSEL, set addr/write/data
        vif->PSEL.write(true);
        vif->PENABLE.write(false);
        vif->PADDR.write(txn->addr);
        vif->PWRITE.write(txn->direction == APB_WRITE);
        if (txn->direction == APB_WRITE)
            vif->PWDATA.write(txn->data);

        sc_core::wait(clk_period);

        // ACCESS phase: assert PENABLE, wait for PREADY
        vif->PENABLE.write(true);
        sc_core::wait(clk_period);

        // Capture read data
        if (txn->direction == APB_READ)
            txn->data = vif->PRDATA.read();
        txn->error = vif->PSLVERR.read();

        // Return to IDLE
        vif->PSEL.write(false);
        vif->PENABLE.write(false);
    }
};

#endif // APB_MASTER_DRIVER_H
```

**Step 2: Commit**

```bash
git add projects/apb_slave/agents/apb_master_driver.h
git commit -m "feat: add APB master driver — translates transactions to APB signals"
```

---

## Task 11: APB Testbench — Monitor and Scoreboard

**Files:**
- Create: `projects/apb_slave/agents/apb_monitor.h`
- Create: `projects/apb_slave/env/apb_scoreboard.h`

**Step 1: Write the APB monitor**

Create `projects/apb_slave/agents/apb_monitor.h`:

```cpp
#ifndef APB_MONITOR_H
#define APB_MONITOR_H

#include <systemc>
#include <uvm>
#include "apb_transaction.h"
#include "apb_if.h"

class apb_monitor : public uvm::uvm_monitor {
public:
    UVM_COMPONENT_UTILS(apb_monitor);

    uvm::uvm_analysis_port<apb_transaction> ap;
    apb_if* vif;
    sc_core::sc_time clk_period;

    apb_monitor(uvm::uvm_component_name name)
        : uvm::uvm_monitor(name), ap("ap"), vif(nullptr),
          clk_period(10, sc_core::SC_NS) {}

    void run_phase(uvm::uvm_phase& phase) {
        while (true) {
            // Wait for PSEL && PENABLE (ACCESS phase)
            while (!(vif->PSEL.read() && vif->PENABLE.read()))
                sc_core::wait(clk_period);

            apb_transaction* txn = apb_transaction::type_id::create("mon_txn");
            txn->addr = vif->PADDR.read();
            txn->direction = vif->PWRITE.read() ? APB_WRITE : APB_READ;
            txn->error = vif->PSLVERR.read();

            if (txn->direction == APB_WRITE)
                txn->data = vif->PWDATA.read();
            else
                txn->data = vif->PRDATA.read();

            UVM_INFO(get_name(), "Observed: " + txn->convert2string(), uvm::UVM_MEDIUM);
            ap.write(*txn);

            sc_core::wait(clk_period);
        }
    }
};

#endif // APB_MONITOR_H
```

**Step 2: Write the scoreboard**

Create `projects/apb_slave/env/apb_scoreboard.h`:

```cpp
#ifndef APB_SCOREBOARD_H
#define APB_SCOREBOARD_H

#include <uvm>
#include "apb_transaction.h"

// Reference model: maintains expected register state.
// On writes: updates internal model.
// On reads: compares observed data against expected.
class apb_scoreboard : public uvm::uvm_scoreboard {
public:
    UVM_COMPONENT_UTILS(apb_scoreboard);

    uvm::uvm_analysis_imp<apb_transaction, apb_scoreboard> analysis_export;

    static const int NUM_REGS = 16;
    sc_dt::sc_uint<32> expected_regs[NUM_REGS];
    int pass_count;
    int fail_count;

    apb_scoreboard(uvm::uvm_component_name name)
        : uvm::uvm_scoreboard(name), analysis_export("analysis_export", this),
          pass_count(0), fail_count(0) {
        for (int i = 0; i < NUM_REGS; i++)
            expected_regs[i] = 0;
    }

    void write(const apb_transaction& txn) {
        unsigned int addr = txn.addr;

        // Skip error transactions
        if (txn.error) {
            UVM_INFO(get_name(), "Error response observed (expected for bad addr)", uvm::UVM_MEDIUM);
            return;
        }

        if ((addr & 0x3) != 0 || addr > (NUM_REGS - 1) * 4) {
            UVM_WARNING(get_name(), "Out-of-range address without error — unexpected");
            return;
        }

        unsigned int idx = addr >> 2;

        if (txn.direction == APB_WRITE) {
            expected_regs[idx] = txn.data;
            UVM_INFO(get_name(), "Model write: " + txn.convert2string(), uvm::UVM_HIGH);
        } else {
            if (txn.data == expected_regs[idx]) {
                pass_count++;
                UVM_INFO(get_name(), "PASS: " + txn.convert2string(), uvm::UVM_MEDIUM);
            } else {
                fail_count++;
                std::ostringstream ss;
                ss << "FAIL: " << txn.convert2string()
                   << " expected=0x" << std::hex << expected_regs[idx];
                UVM_ERROR(get_name(), ss.str());
            }
        }
    }

    void report_phase(uvm::uvm_phase& phase) {
        std::ostringstream ss;
        ss << "Scoreboard: " << pass_count << " passed, " << fail_count << " failed";
        if (fail_count > 0)
            UVM_ERROR(get_name(), ss.str());
        else
            UVM_INFO(get_name(), ss.str(), uvm::UVM_LOW);
    }
};

#endif // APB_SCOREBOARD_H
```

**Step 3: Commit**

```bash
git add projects/apb_slave/agents/apb_monitor.h projects/apb_slave/env/apb_scoreboard.h
git commit -m "feat: add APB monitor and scoreboard with reference model

Monitor passively observes APB bus and sends transactions to scoreboard.
Scoreboard maintains expected register state and checks reads."
```

---

## Task 12: APB Testbench — Master Agent and Environment

**Files:**
- Create: `projects/apb_slave/agents/apb_master_agent.h`
- Create: `projects/apb_slave/env/apb_env.h`

**Step 1: Write master agent**

Create `projects/apb_slave/agents/apb_master_agent.h`:

```cpp
#ifndef APB_MASTER_AGENT_H
#define APB_MASTER_AGENT_H

#include <uvm>
#include "apb_master_driver.h"
#include "apb_monitor.h"
#include "apb_sequencer.h"
#include "apb_if.h"

class apb_master_agent : public uvm::uvm_agent {
public:
    UVM_COMPONENT_UTILS(apb_master_agent);

    apb_master_driver* driver;
    apb_monitor* monitor;
    apb_sequencer* sequencer;
    apb_if* vif;

    apb_master_agent(uvm::uvm_component_name name)
        : uvm::uvm_agent(name), driver(nullptr), monitor(nullptr),
          sequencer(nullptr), vif(nullptr) {}

    void build_phase(uvm::uvm_phase& phase) {
        uvm::uvm_agent::build_phase(phase);
        driver = apb_master_driver::type_id::create("driver", this);
        monitor = apb_monitor::type_id::create("monitor", this);
        sequencer = apb_sequencer::type_id::create("sequencer", this);
    }

    void connect_phase(uvm::uvm_phase& phase) {
        driver->seq_item_port.connect(sequencer->seq_item_export);
        driver->vif = vif;
        monitor->vif = vif;
    }
};

#endif // APB_MASTER_AGENT_H
```

**Step 2: Write environment**

Create `projects/apb_slave/env/apb_env.h`:

```cpp
#ifndef APB_ENV_H
#define APB_ENV_H

#include <uvm>
#include "apb_master_agent.h"
#include "apb_scoreboard.h"
#include "apb_if.h"

class apb_env : public uvm::uvm_env {
public:
    UVM_COMPONENT_UTILS(apb_env);

    apb_master_agent* agent;
    apb_scoreboard* scoreboard;
    apb_if* vif;

    apb_env(uvm::uvm_component_name name)
        : uvm::uvm_env(name), agent(nullptr), scoreboard(nullptr),
          vif(nullptr) {}

    void build_phase(uvm::uvm_phase& phase) {
        uvm::uvm_env::build_phase(phase);
        agent = apb_master_agent::type_id::create("agent", this);
        scoreboard = apb_scoreboard::type_id::create("scoreboard", this);
    }

    void connect_phase(uvm::uvm_phase& phase) {
        agent->vif = vif;
        agent->monitor->ap.connect(scoreboard->analysis_export);
    }
};

#endif // APB_ENV_H
```

**Step 3: Commit**

```bash
git add projects/apb_slave/agents/apb_master_agent.h projects/apb_slave/env/apb_env.h
git commit -m "feat: add APB master agent and environment

Agent wires driver/monitor/sequencer together.
Environment connects monitor analysis port to scoreboard."
```

---

## Task 13: APB Testbench — Tests and Top-Level

**Files:**
- Create: `projects/apb_slave/test/apb_base_test.h`
- Create: `projects/apb_slave/test/apb_write_read_test.h`
- Create: `projects/apb_slave/top/top.cpp`

**Step 1: Write base test**

Create `projects/apb_slave/test/apb_base_test.h`:

```cpp
#ifndef APB_BASE_TEST_H
#define APB_BASE_TEST_H

#include <systemc>
#include <uvm>
#include "apb_env.h"
#include "apb_slave.h"
#include "apb_if.h"

class apb_base_test : public uvm::uvm_test {
public:
    UVM_COMPONENT_UTILS(apb_base_test);

    apb_env* env;
    apb_if* vif;

    apb_base_test(uvm::uvm_component_name name)
        : uvm::uvm_test(name), env(nullptr), vif(nullptr) {}

    void build_phase(uvm::uvm_phase& phase) {
        uvm::uvm_test::build_phase(phase);
        env = apb_env::type_id::create("env", this);

        // Get virtual interface from config db
        if (!uvm::uvm_config_db<apb_if*>::get(this, "", "vif", vif))
            UVM_FATAL(get_name(), "Failed to get APB interface from config_db");
    }

    void connect_phase(uvm::uvm_phase& phase) {
        env->vif = vif;
    }
};

#endif // APB_BASE_TEST_H
```

**Step 2: Write write-read test**

Create `projects/apb_slave/test/apb_write_read_test.h`:

```cpp
#ifndef APB_WRITE_READ_TEST_H
#define APB_WRITE_READ_TEST_H

#include <uvm>
#include "apb_base_test.h"
#include "apb_write_sequence.h"
#include "apb_read_sequence.h"

class apb_write_read_test : public apb_base_test {
public:
    UVM_COMPONENT_UTILS(apb_write_read_test);

    apb_write_read_test(uvm::uvm_component_name name)
        : apb_base_test(name) {}

    void run_phase(uvm::uvm_phase& phase) {
        phase.raise_objection(this, "write_read_test");

        // Write incrementing data to all registers
        apb_write_sequence* wr_seq = apb_write_sequence::type_id::create("wr_seq");
        wr_seq->start(env->agent->sequencer);

        // Read back and verify via scoreboard
        apb_read_sequence* rd_seq = apb_read_sequence::type_id::create("rd_seq");
        rd_seq->start(env->agent->sequencer);

        phase.drop_objection(this, "write_read_test");
    }
};

#endif // APB_WRITE_READ_TEST_H
```

**Step 3: Write top-level entry point**

This is the file that ties everything together — DUT instantiation, clock generation, interface binding, and UVM test launch.

Create `projects/apb_slave/top/top.cpp`:

```cpp
#include <systemc>
#include <uvm>

#include "apb_slave.h"
#include "apb_if.h"
#include "apb_write_read_test.h"

int sc_main(int argc, char* argv[]) {
    // Clock and reset
    sc_core::sc_clock clk("clk", 10, sc_core::SC_NS);
    sc_core::sc_signal<bool> resetn;

    // APB interface
    apb_if apb;

    // Instantiate DUT
    apb_slave dut("dut");
    dut.PCLK(clk);
    dut.PRESETn(resetn);
    dut.PSEL(apb.PSEL);
    dut.PENABLE(apb.PENABLE);
    dut.PWRITE(apb.PWRITE);
    dut.PADDR(apb.PADDR);
    dut.PWDATA(apb.PWDATA);
    dut.PRDATA(apb.PRDATA);
    dut.PREADY(apb.PREADY);
    dut.PSLVERR(apb.PSLVERR);

    // Pass interface to UVM testbench via config_db
    uvm::uvm_config_db<apb_if*>::set(nullptr, "*", "vif", &apb);

    // Reset sequence
    resetn.write(false);
    sc_core::sc_start(20, sc_core::SC_NS);
    resetn.write(true);

    // Run UVM test
    uvm::run_test("apb_write_read_test");

    return 0;
}
```

**Step 4: Commit**

```bash
git add projects/apb_slave/test/apb_base_test.h \
       projects/apb_slave/test/apb_write_read_test.h \
       projects/apb_slave/top/top.cpp
git commit -m "feat: add APB tests and top-level with DUT instantiation

- apb_base_test: gets interface from config_db
- apb_write_read_test: writes all regs then reads back
- top.cpp: clock gen, reset, DUT binding, UVM launch"
```

---

## Task 14: CMake — APB Project Build

**Files:**
- Create: `projects/CMakeLists.txt`
- Create: `projects/apb_slave/CMakeLists.txt`

**Step 1: Write projects CMakeLists.txt**

Create `projects/CMakeLists.txt`:

```cmake
add_subdirectory(apb_slave)
```

**Step 2: Write APB project CMakeLists.txt**

Create `projects/apb_slave/CMakeLists.txt`:

```cmake
add_executable(apb_slave_sim top/top.cpp)

target_include_directories(apb_slave_sim PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/rtl
    ${CMAKE_CURRENT_SOURCE_DIR}/env
    ${CMAKE_CURRENT_SOURCE_DIR}/agents
    ${CMAKE_CURRENT_SOURCE_DIR}/sequences
    ${CMAKE_CURRENT_SOURCE_DIR}/test
)

target_link_libraries(apb_slave_sim
    SystemC::systemc
    SystemC::uvm
)
```

**Step 3: Build the APB testbench**

```bash
cd build
cmake .. -DSYSTEMC_HOME=/usr/local/systemc-3.0.0 -DUVM_SYSTEMC_HOME=/usr/local/uvm-systemc
make apb_slave_sim
```

Expected: Compiles without errors.

**Step 4: Run the testbench**

```bash
./projects/apb_slave/apb_slave_sim
```

Expected: UVM phases execute, 16 writes followed by 16 reads, scoreboard reports 16 passed 0 failed.

**Step 5: Commit**

```bash
git add projects/CMakeLists.txt projects/apb_slave/CMakeLists.txt
git commit -m "build: add CMake for APB slave testbench project"
```

---

## Task 15: Documentation — README.md

**Files:**
- Create: `README.md`

**Step 1: Write README**

Create `README.md`:

```markdown
# SystemC Learning Repository

A progressive learning path through SystemC and UVM-SystemC, from basic modules to a complete APB bus verification environment.

## Prerequisites

- **SystemC 3.0** — [Accellera download](https://systemc.org/downloads/standards/)
- **UVM-SystemC** — required only for `projects/apb_slave`
- **CMake 3.14+**
- **C++17 compiler** (g++ or clang++)

## Build

```bash
mkdir build && cd build
cmake .. -DSYSTEMC_HOME=/path/to/systemc
make
```

To also build the UVM testbench:

```bash
cmake .. -DSYSTEMC_HOME=/path/to/systemc -DUVM_SYSTEMC_HOME=/path/to/uvm-systemc
make
```

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
# Open orGate.vcd with GTKWave to view waveforms
```

### 3. AND Gate (`examples/03_and_gate`)

Introduces `sc_logic` (4-state: 0, 1, X, Z) instead of `bool`, important for modeling uninitialized hardware values.

```bash
./build/examples/03_and_gate/and_gate
```

### 4. APB Slave Verification (`projects/apb_slave`)

A complete UVM-SystemC testbench verifying an AMBA APB3 slave with a 16x32-bit register file.

**Components:** DUT, master driver, monitor, scoreboard, sequences, test.

```bash
./build/projects/apb_slave/apb_slave_sim
```

## Project Structure

```
examples/          Progressive SystemC examples
  01_hello_world/  SC_MODULE basics
  02_or_gate/      Combinational logic, VCD tracing
  03_and_gate/     4-state logic (sc_logic)
projects/
  apb_slave/       Full UVM verification environment
    rtl/           APB slave DUT
    agents/        Master driver, monitor
    env/           Environment, scoreboard, transactions
    sequences/     Write/read stimulus sequences
    test/          UVM test classes
    top/           Top-level entry point
cmake/             CMake modules (FindSystemC)
docs/plans/        Design and implementation documents
```
```

**Step 2: Commit**

```bash
git add README.md
git commit -m "docs: add README with build instructions and learning guide"
```

---

## Task 16: GitHub Actions CI

**Files:**
- Create: `.github/workflows/build.yml`

**Step 1: Write CI workflow**

Create `.github/workflows/build.yml`:

```yaml
name: Build

on:
  push:
    branches: [master]
  pull_request:
    branches: [master]

jobs:
  build-examples:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v4

      - name: Cache SystemC
        id: cache-systemc
        uses: actions/cache@v4
        with:
          path: /opt/systemc
          key: systemc-3.0.0-${{ runner.os }}

      - name: Install SystemC
        if: steps.cache-systemc.outputs.cache-hit != 'true'
        run: |
          wget -q https://github.com/accellera-official/systemc/archive/refs/tags/3.0.0.tar.gz
          tar xzf 3.0.0.tar.gz
          cd systemc-3.0.0
          mkdir build && cd build
          cmake .. -DCMAKE_INSTALL_PREFIX=/opt/systemc -DCMAKE_CXX_STANDARD=17
          make -j$(nproc) && make install

      - name: Configure
        run: cmake -B build -DSYSTEMC_HOME=/opt/systemc

      - name: Build
        run: cmake --build build -j$(nproc)

      - name: Run hello_world
        run: ./build/examples/01_hello_world/hello_world

      - name: Run or_gate
        run: ./build/examples/02_or_gate/or_gate

      - name: Run and_gate
        run: ./build/examples/03_and_gate/and_gate
```

**Step 2: Commit**

```bash
mkdir -p .github/workflows
git add .github/workflows/build.yml
git commit -m "ci: add GitHub Actions workflow to build and run examples"
```

---

## Task 17: Final Cleanup — Remove Old sc_testbench

**Files:**
- Delete: `projects/sc_testbench/` (entire directory)

**Step 1: Verify APB testbench compiles and runs before removing old code**

```bash
cd build && make apb_slave_sim && ./projects/apb_slave/apb_slave_sim
```

Expected: Scoreboard passes.

**Step 2: Remove old testbench**

```bash
git rm -r projects/sc_testbench
git commit -m "chore: remove old sc_testbench, replaced by projects/apb_slave"
```

---

## Summary

| Task | Description | Dependencies |
|------|-------------|-------------|
| 1 | Update .gitignore | — |
| 2 | Remove swap files and old Makefiles | 1 |
| 3 | Remove Gemini workflows | — |
| 4 | Reorganize example directories | 2 |
| 5 | FindSystemC.cmake | — |
| 6 | Root + example CMakeLists | 4, 5 |
| 7 | APB slave DUT | — |
| 8 | APB transaction + constants | — |
| 9 | APB sequencer + sequences | 8 |
| 10 | APB master driver | 8 |
| 11 | APB monitor + scoreboard | 8 |
| 12 | APB master agent + environment | 9, 10, 11 |
| 13 | APB tests + top.cpp | 7, 12 |
| 14 | APB CMakeLists | 6, 13 |
| 15 | README | 14 |
| 16 | GitHub Actions CI | 6 |
| 17 | Remove old sc_testbench | 14 |
