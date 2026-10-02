#!/usr/bin/env bash
# build.sh – compile all three tasks (macOS / Linux / MSYS2 UCRT64)
# Run from Project_1/: bash build.sh
set -euo pipefail
cd -- "$(dirname -- "$0")"

if ! command -v g++ > /dev/null 2>&1 && ! command -v c++ > /dev/null 2>&1; then
  echo 'Error: no C++ compiler found (g++ or c++).'
  echo 'macOS: xcode-select --install'
  echo 'Ubuntu: sudo apt install build-essential'
  echo 'MSYS2 UCRT64: pacman -S --needed mingw-w64-ucrt-x86_64-gcc'
  exit 1
fi

CXX="${CXX:-$(command -v g++ 2>/dev/null || echo c++)}"
FLAGS=(-std=c++17 -O2 -Wall -Wextra -IStorage/src)
STORAGE=(Storage/src/disk.cpp Storage/src/record.cpp Storage/src/storage.cpp)
NODES=(BPlusTree/src/bplustree_header_io.cpp
       BPlusTree/src/bplustree_leaf.cpp
       BPlusTree/src/bplustree_internal.cpp
       BPlusTree/src/bplustree_debug.cpp)
TREE=("${NODES[@]}" BPlusTree/src/bulk_load.cpp BPlusTree/src/tree_stats.cpp)

echo 'Compiling Task 1 (storage)...'
"$CXX" "${FLAGS[@]}" -o task1 Storage/src/task1.cpp "${STORAGE[@]}"

echo 'Compiling Task 2 (B+ tree bulk load)...'
"$CXX" "${FLAGS[@]}" -o task2 BPlusTree/src/task2.cpp "${TREE[@]}" "${STORAGE[@]}"

echo 'Compiling Task 3 (retrieval + linear scan + deletion)...'
"$CXX" "${FLAGS[@]}" -o task3 \
    BPlusTree/src/task3.cpp \
    BPlusTree/src/bplustree_delete.cpp \
    "${TREE[@]}" "${STORAGE[@]}"

echo ''
echo 'Build successful.  Binaries: task1  task2  task3'
