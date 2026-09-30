CXX      ?= g++
CXXFLAGS ?= -std=c++17 -O2 -Wall -Wextra -I../Storage/src
STORAGE  = ../Storage/src/disk.cpp ../Storage/src/record.cpp ../Storage/src/storage.cpp
TREE     = src/bptree.cpp src/bulk_load.cpp

task2: src/task2.cpp $(TREE) $(STORAGE) $(wildcard src/*.h ../Storage/src/*.h)
	$(CXX) $(CXXFLAGS) -o task2 src/task2.cpp $(TREE) $(STORAGE)

run: task2
	./task2 ../Storage/data.db

clean:
	rm -f task2 task2.exe
