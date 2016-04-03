//Incude SystemC header file
#include <systemc.h>

//Access 'cout' method
#include <iostream>


//Define SystemC module
SC_MODULE(HelloWorld){

  //Clock
  sc_in_clk clk;

  //Constructor for the module
  SC_CTOR(HelloWorld){

    //Declare Method and sensitivity
    SC_METHOD(HelloWorldMethod);
    //Method is sensitive to Clock Negative Edge
    sensitive << clk.neg();
    //Don't provide with initial values
    dont_initialize();
  }

  //Method definition
  void HelloWorldMethod(void){
    //Print current time value with sc_time_stamp
    std::cout << sc_time_stamp()
	      << ":\tHello World!\n"
	      << std::endl;
  }

};

// Program starts from here
int sc_main(int argc, char* argv[]){

  //Define clock time period
  const sc_time t_PERIOD(8, SC_NS);
  sc_clock clk("clk", t_PERIOD);

  //Instantiate Module
  HelloWorld Hello("Hello");
  //Connect clock signal to module clock
  Hello.clk(clk);

  //Initial. Ends after 10ns
  sc_start(10, SC_NS);

  return 0;

}
