###############################################################
## Basic makefile template.
## Uses google libraries to build test and benchmark suites.
###############################################################

######################## USER CONF ############################

# Standard compiler flags.
cppflags = -O3 -Wall -Wextra -pedantic -fPIC --std=c++17
# Any external libraries to link (gtest/gbench included below).
libs = -lClpSolver -lClp -lCoinUtils -lbz2 -lz -llapack -lblas -lm -lpthread -ligraph

######################## AUTO CONF ############################

objects = $(addprefix obj/, $(addsuffix .o, $(modules)))
opt_objects = $(addprefix obj/, $(addsuffix .opt.o, $(modules)))

TEST_FILE = tests/test1.mps
TEST_INFEAS_FILE = tests/test2.mps

all: bin/evaluate

clean:
	rm -f obj/*
	rm -f bin/*

test: bin/test
	@bin/test

bench: bin/bench
	@bin/bench | grep --invert-match Coin

eval_test: bin/evaluate
	@bin/evaluate $(TEST_FILE)

eval_test_infeas: bin/evaluate
	@bin/evaluate $(TEST_INFEAS_FILE)

# Memory access error and leak checking
memtest: bin/evaluate
	valgrind --leak-check=full bin/evaluate $(TEST_FILE) $(TEST_INFEAS_FILE)

# Feature evaluation on MPS files.
bin/evaluate: obj/graph.o obj/evaluate.o
	mkdir -p bin
	g++ -o $@ $^ $(libs)

# Feature evaluation on MPS files.
bin/test: obj/graph.o obj/test.o
	mkdir -p bin
	g++ -o $@ $^ $(libs)

# Feature evaluation on MPS files.
bin/bench: obj/graph.o obj/bench.o
	mkdir -p bin
	g++ -o $@ $^ $(libs) -lbenchmark -lpthread

# Object files.
obj/%.o: src/%.cpp
	mkdir -p obj
	g++ $(cppflags) -o $@ $< -c

# Header deps.
obj/evaluate.o: src/mipfeatures.hpp
obj/test.o: src/mipfeatures.hpp
obj/bench.o: src/mipfeatures.hpp
