#!/usr/bin/perl

use warnings;
use strict;

sub main();


my $LINUX64 = "\$SYSTEMC_HOME/lib-linux64";

my $INC = "\$SYSTEMC_HOME/include";

main();



sub main(){

    eval{
        $ARGV[0];
    }or do{
        print("\n\tRun the script with SystemC <filename> as argument\n\n");
        exit 1;
    };

    system "g++ -I. -I$INC -L. -L$LINUX64 -o sim $ARGV[0] -lsystemc -Wl,-rpath,$LINUX64";

    system "./sim";

}

