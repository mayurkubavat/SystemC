#include "systemc.h"

SC_MODULE(or_gate){

   // List of ports
   sc_in<bool> a;
   sc_in<bool> b;

   sc_out<bool> c;


   // Function implements OR logic
   void or_p(){
      c.write(a.read() | b.read());
   }


   //Constructor
   SC_CTOR(or_gate){

      SC_METHOD(or_p);
      sensitive << a << b;
   }
};


// Testbench for OR gate
int sc_main(int argc, char* argv[]){

   sc_signal<bool> a;
   sc_signal<bool> b;
   sc_signal<bool> c;


   // Instantiate DUT
   or_gate i_or("orGate");
   i_or.a(a);
   i_or.b(b);
   i_or.c(c);


   sc_trace_file *wave = sc_create_vcd_trace_file("orGate");

   sc_trace(wave, a, "a");
   sc_trace(wave, b, "b");
   sc_trace(wave, c, "c");

   wave->set_time_unit(1, SC_NS);

   cout << "\n\n";


   cout << sc_time_stamp() << " : A = " << a << " B = " << b << " C = " << c << "\n";

   sc_start(4, SC_NS);

   a = 0;
   b = 0;

   cout << sc_time_stamp() << " : A = " << a << " B = " << b << " C = " << c << "\n";

   sc_start(2, SC_NS);

   a = 1;
   b = 0;

   cout << sc_time_stamp() << " : A = " << a << " B = " << b << " C = " << c << "\n";

   sc_start(2, SC_NS);

   a = 0;
   b = 1;

   cout << sc_time_stamp() << " : A = " << a << " B = " << b << " C = " << c << "\n";

   sc_start(2, SC_NS);

   a = 1;
   b = 1;

   cout << sc_time_stamp() << " : A = " << a << " B = " << b << " C = " << c << "\n";

   sc_start(2, SC_NS);

   cout << sc_time_stamp() << " : A = " << a << " B = " << b << " C = " << c << "\n";

   cout << "\n";

   sc_stop();
   sc_close_vcd_trace_file(wave);

   return 0;

}
