CLIBS   := $(SYSTEMC_HOME)/lib-linux64
CINC    := $(SYSTEMC_HOME)/include

INC     := ./
SRCS    := $(wildcard *.cpp)

run: compile simulate

compile:
	g++ -I$(INC) -I$(CINC) -L. -L$(CLIBS) $(SRCS) -o sim -lsystemc -lm

simulate:
	./sim

.PHONY: clean
clean:
	rm -rf ./sim *.vcd
