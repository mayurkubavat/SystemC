 #include <systemc>

// These using-declarations to make the code more readable.
// 'sc_core' contains the core simulation kernel (e.g., sc_module, sc_port, sc_signal).
// 'sc_dt' contains the SystemC data types (e.g., sc_logic, sc_int, sc_bv).
using namespace sc_core;
using namespace sc_dt;

using namespace std;

SC_MODULE(or_gate){

   // List of ports
   sc_in<sc_logic> a;
   sc_in<sc_logic> b;

   sc_out<sc_logic> c;


   // Function implements OR logic
   void or_p(){
      c.write(a.read() | b.read());
   }


   // Constructor
   SC_CTOR(or_gate){

      SC_METHOD(or_p);
      sensitive << a << b;
   }
};


// Function to set up VCD tracing for the OR gate testbench
sc_trace_file* setup_tracing(sc_signal<sc_logic>& a, sc_signal<sc_logic>& b, sc_signal<sc_logic>& c) {
   sc_trace_file *wave = sc_create_vcd_trace_file("waves");
   sc_trace(wave, a, "a");
   sc_trace(wave, b, "b");
   sc_trace(wave, c, "c");
   wave->set_time_unit(1, SC_NS);
   return wave;
}

// Function to apply stimulus and check the OR gate's output
void run_test(sc_signal<sc_logic>& a, sc_signal<sc_logic>& b, sc_signal<sc_logic>& c) {
   // Print a header for the output
   cout << "\n" << "Time\tA\tB\tC" << endl;
   cout << "-------------------------" << endl;

   // Loop through all 4 input combinations (00, 01, 10, 11)
   for (int i = 0; i < 4; ++i) {
       // Use bitwise operations to assign values to a and b
       a = sc_logic((i >> 1) & 1); // Bit 1 of i
       b = sc_logic(i & 1);        // Bit 0 of i
       sc_start(1, SC_NS);         // Wait for the delta cycle to propagate
       cout << sc_time_stamp() << "\t" << a << "\t" << b << "\t" << c << endl;
   }
   cout << "\n";
}

// Testbench for OR gate
int sc_main(int argc, char* argv[]){

   sc_signal<sc_logic> a, b, c;

   // Instantiate DUT and bind ports
   or_gate i_or("i_or");
   i_or.a(a);
   i_or.b(b);
   i_or.c(c);

   // Setup VCD trace file
   sc_trace_file *wave = setup_tracing(a, b, c);

   // Run the test stimulus
   run_test(a, b, c);

   // Clean up
   sc_close_vcd_trace_file(wave);

   return 0;
}
