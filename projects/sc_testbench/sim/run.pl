#!/usr/bin/perl

use warnings;
use strict;

sub main();


my $CLIBS = "/home/mayur/DV/systemc-2.3.1/lib-linux64";
my $UVMCLIBS = "/home/mayur/DV/uvm-systemc-1.0/lib-linux64";

my $CINC = "/home/mayur/DV/systemc-2.3.1/include";
my $UVMCINC = "/home/mayur/DV/uvm-systemc-1.0/include";
my $INC = "-I../test -I../top -I../env -I../master_agent -I../slave_agent";

main();



sub main(){

    system "g++ $INC -I$CINC -I$UVMCINC -L$CLIBS -lsystemc -L$UVMCLIBS -luvm-systemc ../top/top.cpp -o sim -Wl,-rpath,$CLIBS -Wl,-rpath,$UVMCLIBS";

    system "./sim";
}
