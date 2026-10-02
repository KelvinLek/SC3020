# =============================================================
# SC3020 Project 1 – Unified Makefile (Member 5: integration)
#
# Usage (run from Project_1/):
#   make              # build all three tasks
#   make task1        # build only Task 1 (storage)
#   make task2        # build only Task 2 (B+ tree)
#   make task3        # build only Task 3 (deletion + comparison)
#   make run          # run the full pipeline end-to-end
#   make clean        # remove all build artefacts
#
# All binaries are placed in Project_1/ (current directory).
# The output database files go into out/.
# =============================================================

CXX      ?= g++
CXXFLAGS ?= -std=c++17 -O2 -Wall -Wextra -IStorage/src

# Source groups
STORAGE = Storage/src/disk.cpp \
          Storage/src/record.cpp \
          Storage/src/storage.cpp

NODES   = BPlusTree/src/bplustree_header_io.cpp \
          BPlusTree/src/bplustree_leaf.cpp \
          BPlusTree/src/bplustree_internal.cpp \
          BPlusTree/src/bplustree_debug.cpp

TREE    = $(NODES) \
          BPlusTree/src/bulk_load.cpp \
          BPlusTree/src/tree_stats.cpp

HDRS    = $(wildcard Storage/src/*.h BPlusTree/src/*.h)

.PHONY: all task1 task2 task3 run clean

all: task1 task2 task3

# Task 1 – load games.txt into the block-based storage file (data.db)
task1: Storage/src/task1.cpp $(STORAGE) $(HDRS)
	$(CXX) $(CXXFLAGS) -o task1 Storage/src/task1.cpp $(STORAGE)

# Task 2 – bulk-load the B+ tree index (index.db) from data.db
task2: BPlusTree/src/task2.cpp $(TREE) $(STORAGE) $(HDRS)
	$(CXX) $(CXXFLAGS) -o task2 BPlusTree/src/task2.cpp $(TREE) $(STORAGE)

# Task 3 – indexed retrieval, linear scan, and deletion of FG_PCT_home > 0.5
task3: BPlusTree/src/task3.cpp BPlusTree/src/bplustree_delete.cpp $(TREE) $(STORAGE) $(HDRS)
	$(CXX) $(CXXFLAGS) -o task3 \
	    BPlusTree/src/task3.cpp \
	    BPlusTree/src/bplustree_delete.cpp \
	    $(TREE) $(STORAGE)

# Run the full pipeline: Task 1 → Task 2 → Task 3
# Output files go in out/ so the originals are never overwritten.
run: all
	@mkdir -p out
	@echo ""
	@echo "=========================================="
	@echo "  Task 1: Loading games.txt into storage"
	@echo "=========================================="
	./task1 games.txt out/data.db
	@echo ""
	@echo "=========================================="
	@echo "  Task 2: Building B+ tree index"
	@echo "=========================================="
	./task2 out/data.db --index out/index.db
	@echo ""
	@echo "=========================================="
	@echo "  Task 3: Retrieval comparison + deletion"
	@echo "=========================================="
	./task3 out/data.db out/index.db

clean:
	rm -f task1 task2 task3 task1.exe task2.exe task3.exe
	rm -rf out/ task3_runs/ demo_inputs/
