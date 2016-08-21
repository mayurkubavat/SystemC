
#include <systemc>
#include <uvm>

#include "test.h"

int sc_main(int, char*[])
{  
  test* test_h = new test("test_h");

  uvm::run_test();

  return 0;
}
