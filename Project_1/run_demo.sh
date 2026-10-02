#!/usr/bin/env bash
# run_demo.sh – build and run the full SC3020 Project 1 pipeline.
# Creates output files in out/ so originals are never overwritten.
# Task 3 additionally creates a fresh task3_runs/run_NNN folder.
#
# Usage:
#   bash run_demo.sh          # use games.txt in the same directory
#   bash run_demo.sh path/to/games.txt
set -euo pipefail
cd -- "$(dirname -- "$0")"

GAMES="${1:-games.txt}"
if [ ! -f "$GAMES" ]; then
  echo "Error: data file not found: $GAMES"
  echo "Usage: bash run_demo.sh [path/to/games.txt]"
  exit 1
fi

bash build.sh

mkdir -p out

echo ""
echo "=========================================="
echo "  Task 1: Loading $GAMES into storage"
echo "=========================================="
./task1 "$GAMES" out/data.db

echo ""
echo "=========================================="
echo "  Task 2: Building B+ tree index"
echo "=========================================="
./task2 out/data.db --index out/index.db

echo ""
echo "=========================================="
echo "  Task 3: Retrieval comparison + deletion"
echo "          (FG_PCT_home > 0.5)"
echo "=========================================="
./task3 out/data.db out/index.db

echo ""
echo "Done. Output files are in out/  and  task3_runs/"
