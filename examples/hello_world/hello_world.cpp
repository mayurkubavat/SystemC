#include "systemc.h"

SC_MODULE(hello_world){

   SC_CTOR(hello_world){}

   void hello(){
   
      cout << "\nMy first SystemC program! \n\n";
   }
};


int sc_main(int argc, char* argv[]){

   hello_world ihw("helloWorldInstance");

   ihw.hello();

   return 0;
}
