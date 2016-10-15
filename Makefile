all: task1 task2 sample

TASK1_SRC = task1.cpp
TASK2_SRC = task2.cpp

TASK1 = asg2-task1
TASK2 = asg2-task2

task1: $(TASK1_SRC)
	clang++ -o $(TASK1) -std=c++11 $(TASK1_SRC) `llvm-config --cxxflags` `llvm-config --ldflags` `llvm-config --libs` -lpthread -lncurses -ldl

task2: $(TASK2_SRC)
	clang++ -o $(TASK2) -std=c++11 $(TASK2_SRC) `llvm-config --cxxflags` `llvm-config --ldflags` `llvm-config --libs` -lpthread -lncurses -ldl

expt: mainy.cpp
	clang++ -std=c++11 mainy.cpp `llvm-config --cxxflags` `llvm-config --ldflags` `llvm-config --libs` -lpthread -lncurses -ldl

sample:
	clang -c -S -emit-llvm samples/*.c

clean:
	rm -f *.ll
	rm -f *.out
	rm -f $(TASK1) $(TASK2) a.out

.PHONY: all sample clean expt
