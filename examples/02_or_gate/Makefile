CXX = g++
CXXFLAGS = -Wall -g -std=c++17

SYSTEMC_HOME = /usr/local/systemc-3.0.0
SYSTEMC_INCLUDE = -I$(SYSTEMC_HOME)/include
SYSTEMC_LIBDIR = -L$(SYSTEMC_HOME)/lib-macosarm64
SYSTEMC_LIBS = -lsystemc

SIM_EXEC = sim

all: $(SIM_EXEC)

$(SIM_EXEC):  # No need to list .cpp files here

# Define a rule to compile any .cpp file into an object file (.o)
%.o: %.cpp
	$(CXX) $(CXXFLAGS) $(SYSTEMC_INCLUDE) -c $< -o $@

# Link all object files into the final executable
$(SIM_EXEC): $(patsubst %.cpp,%.o,$(wildcard *.cpp)) 
	$(CXX) $(CXXFLAGS) $(SYSTEMC_LIBDIR) $^ -o $@ $(SYSTEMC_LIBS) -lm

clean:
	rm -f $(SIM_EXEC) *.o
