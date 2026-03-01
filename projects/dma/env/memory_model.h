// ----------------------------------------------------------------------------
// memory_model — Simple backing store for AXI4-Lite slave agent
//
// Provides a word-addressable memory using std::map<uint32_t, uint32_t>.
// Uninitialized locations return 0 on read (matching typical hardware behavior).
//
// This is a pure C++ utility class with no SystemC or UVM dependencies,
// making it easy to unit-test independently.
//
// Usage:
//   memory_model mem;
//   mem.load_pattern(0x1000, 8, 0xA0);  // pre-load 8 words starting at 0x1000
//   mem.write(0x2000, 0xDEADBEEF);      // single write
//   uint32_t val = mem.read(0x1000);     // returns 0xA0
// ----------------------------------------------------------------------------
#ifndef MEMORY_MODEL_H
#define MEMORY_MODEL_H

#include <cstdint>
#include <map>

class memory_model {
public:
  // Store a 32-bit word at the given address
  void write(uint32_t addr, uint32_t data) {
    mem_[addr] = data;
  }

  // Read a 32-bit word from the given address (returns 0 if not present)
  uint32_t read(uint32_t addr) const {
    auto it = mem_.find(addr);
    return (it != mem_.end()) ? it->second : 0;
  }

  // Pre-load a sequential pattern into memory:
  //   mem[base_addr + i*4] = pattern_base + i   for i in 0..count-1
  //
  // Useful for initializing source memory before a DMA transfer test.
  void load_pattern(uint32_t base_addr, uint32_t count, uint32_t pattern_base) {
    for (uint32_t i = 0; i < count; ++i) {
      mem_[base_addr + i * 4] = pattern_base + i;
    }
  }

  // Clear all memory contents
  void clear() {
    mem_.clear();
  }

private:
  std::map<uint32_t, uint32_t> mem_;
};

#endif // MEMORY_MODEL_H
