#!/usr/bin/env perl

use warnings;
use strict;

sub main();


my $CLIBS = "\$SYSTEMC_HOME/lib-linux64";
my $UVMCLIBS = "\$UVM_SYSTEMC_HOME/lib-linux64";

my $CINC = "\$SYSTEMC_HOME/include";
my $UVMCINC = "\$UVM_SYSTEMC_HOME/include";

my $INC = "./";

main();



sub main(){


    system "g++ -I.$INC -I$CINC -I$UVMCINC -L. -L$CLIBS -L$UVMCLIBS or_gate.cpp -o sim -lsystemc -luvm-systemc -lm";

    system "execstack -s ./sim; ./sim";
}
