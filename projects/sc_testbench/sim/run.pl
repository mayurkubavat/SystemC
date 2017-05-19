#!/usr/bin/env perl

use warnings;
use strict;

sub main();


my $CLIBS = "\$SYSTEMC_HOME/lib-linux64";
my $UVMCLIBS = "\$UVM_SYSTEMC_HOME/lib-linux64";

my $CINC = "\$SYSTEMC_HOME/include";
my $UVMCINC = "\$UVM_SYSTEMC_HOME/include";
my $INC = "-I../test -I../top -I../env -I../master_agent -I../slave_agent";

main();



sub main(){

    # Run simplified syntax 
    #system "g++ $INC -I$CINC -I$UVMCINC -L$CLIBS -L$UVMCLIBS ../top/top.cpp -o sim -lsystemc -luvm-systemc -Wl,-rpath,$CLIBS -Wl,-rpath,$UVMCLIBS";


    system "g++ -I. $INC -I$CINC -I$UVMCINC -L. -L$CLIBS -L$UVMCLIBS ../top/top.cpp -o sim -lsystemc -luvm-systemc -lm";

    system "execstack -s ./sim; ./sim";
}
