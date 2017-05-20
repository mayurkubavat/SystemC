#include "systemc.h"

SC_MODULE(and_gate){

   // List of ports
   sc_in<sc_logic> a;
   sc_in<sc_logic> b;

   sc_out<sc_logic> c;


   // Function implements OR logic
   void and_p(){
      c.write(a.read() & b.read());
   }


   //Constructor
   SC_CTOR(and_gate){

      SC_METHOD(and_p);
      sensitive << a << b;
   }
};


// Testbench for OR gate
int sc_main(int argc, char* argv[]){

   sc_signal<sc_logic> a;
   sc_signal<sc_logic> b;
   sc_signal<sc_logic> c;


   // Instantiate DUT
   and_gate i_or("andGate");
   i_or.a(a);
   i_or.b(b);
   i_or.c(c);


   sc_trace_file *wave = sc_create_vcd_trace_file("andGate");

   sc_trace(wave, a, "a");
   sc_trace(wave, b, "b");
   sc_trace(wave, c, "c");

   wave->set_time_unit(1, SC_NS);

   cout << "\n\n";


   cout << sc_time_stamp() << " : A = " << a << " B = " << b << " C = " << c << "\n";

   sc_start(4, SC_NS);

   a = sc_logic('0');
   b = sc_logic('0');

   cout << sc_time_stamp() << " : A = " << a << " B = " << b << " C = " << c << "\n";

   sc_start(2, SC_NS);

   a = sc_logic('1');
   b = sc_logic('0');

   cout << sc_time_stamp() << " : A = " << a << " B = " << b << " C = " << c << "\n";

   sc_start(2, SC_NS);

   a = sc_logic('0');
   b = sc_logic('1');

   cout << sc_time_stamp() << " : A = " << a << " B = " << b << " C = " << c << "\n";

   sc_start(2, SC_NS);

   a = sc_logic('1');
   b = sc_logic('1');

   cout << sc_time_stamp() << " : A = " << a << " B = " << b << " C = " << c << "\n";

   sc_start(2, SC_NS);

   cout << sc_time_stamp() << " : A = " << a << " B = " << b << " C = " << c << "\n";

   cout << "\n";

   sc_stop();
   sc_close_vcd_trace_file(wave);

   return 0;

}
