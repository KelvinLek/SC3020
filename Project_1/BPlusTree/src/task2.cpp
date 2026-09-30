// task2.cpp - Task 2 driver: build the B+ tree on FG_PCT_home and report statistics.
//
// Usage: task2 [data.db] [--n N] [--limit K] [--print]
//   data.db    database built by Task 1 (default ../Storage/data.db)
//   --n N      override n (default: computeN(BLOCK_SIZE)); try --n 3 to see splits
//   --limit K  only index the first K records (small trees for testing/demo)
//   --print    print the keys of every level
#include <iomanip>
#include <iostream>
#include <string>

#include "bptree.h"
#include "storage.h"

using namespace storage;
using namespace bptree;

int main(int argc, char** argv) {
    std::string dbPath = "../Storage/data.db";
    std::size_t n = computeN();
    std::size_t limit = 0;
    bool print = false;
    for (int i = 1; i < argc; ++i) {
        std::string a = argv[i];
        if (a == "--n" && i + 1 < argc) n = std::stoul(argv[++i]);
        else if (a == "--limit" && i + 1 < argc) limit = std::stoul(argv[++i]);
        else if (a == "--print") print = true;
        else dbPath = a;
    }

    try {
        // 1. Read every data block once and collect (FG_PCT_home, RecordId).
        StorageManager db(dbPath, /*createNew=*/false);
        std::vector<Entry> entries;
        std::size_t skippedNull = 0;
        for (BlockId b = db.firstDataBlock(); b < db.endDataBlock(); ++b) {
            DataBlock blk = db.readDataBlock(b);
            for (std::uint16_t s = 0; s < blk.slotsUsed(); ++s) {
                if (!blk.isLive(s)) continue;
                Record r = blk.record(s);
                if (r.isNull(NULL_FG_PCT)) { ++skippedNull; continue; }
                if (limit && entries.size() >= limit) break;
                entries.push_back({r.fgPctHome, blk.recordId(s)});
            }
        }

        // 2. Build.
        InMemoryNodeStore store;   // TODO(Member 2): DiskNodeStore on "index.db"
        BPlusTree tree(store, n);
        tree.bulkLoad(entries);
        std::uint64_t nodeWrites = store.writes();

        // 3. Report.
        store.resetCounters();
        TreeStats s = tree.stats();
        std::cout << "=== Task 2: B+ tree on FG_PCT_home ===\n"
                  << "Indexed " << entries.size() << " records (" << skippedNull
                  << " with empty FG_PCT_home skipped), " << db.dataBlockReads() << " data blocks read\n"
                  << "Built by bulk loading, " << nodeWrites << " node writes\n\n"
                  << "Parameter n                     " << s.n << "\n"
                  << "Number of nodes                 " << s.numNodes
                  << " (" << s.numInternal << " internal + " << s.numLeaves << " leaves)\n"
                  << "Number of levels                " << s.levels << "\n"
                  << "Root node keys (" << s.rootKeys.size() << ")            ";
        std::cout << std::fixed << std::setprecision(3);
        for (std::size_t i = 0; i < s.rootKeys.size(); ++i) std::cout << (i ? " " : "") << s.rootKeys[i];
        std::cout << "\n";

        if (print) std::cout << "\n" << tree.dump();

        std::string problem = tree.validate(entries);
        std::cout << "\nValidation: " << (problem.empty() ? "OK" : "FAILED - " + problem) << "\n";
        return problem.empty() ? 0 : 1;
    } catch (const std::exception& e) {
        std::cerr << "error: " << e.what() << "\n";
        return 1;
    }
}
