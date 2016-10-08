all: task1 task2

TASK1 = task1
TASK2 = task2

task1: dead.cpp
	clang++ -o $(TASK1) -std=c++11 dead.cpp `llvm-config --cxxflags` `llvm-config --ldflags` `llvm-config --libs` -lpthread -lncurses -ldl

task2: escape.cpp
	clang++ -o $(TASK2) -std=c++11 escape.cpp `llvm-config --cxxflags` `llvm-config --ldflags` `llvm-config --libs` -lpthread -lncurses -ldl

expt: mainy.cpp
	clang++ -std=c++11 mainy.cpp `llvm-config --cxxflags` `llvm-config --ldflags` `llvm-config --libs` -lpthread -lncurses -ldl

sample:
	clang -c -S -emit-llvm samples/*.c

clean:
	rm -f *.ll
	rm -f *.out
	rm -f $(TASK1) $(TASK2) a.out

.PHONY: all sample clean expt
