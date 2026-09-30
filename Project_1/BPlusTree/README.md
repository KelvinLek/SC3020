# B+ tree construction (Task 2) - Member 3

Builds a B+ tree on `FG_PCT_home` over the records stored by Member 1 (`../Storage/data.db`) and reports the Task 2 statistics.

**Status: skeleton.** Bulk loading, statistics and validation work against an **in-memory** node store. Member 2's final node format and on-disk store are still TODO (search `TODO(Member 2)`).

## Build and run

```bash
make task2
./task2                              # full tree, n from BLOCK_SIZE
./task2 --n 3 --limit 20 --print     # tiny tree, prints every level (good for learning/demo)
```

## Files

| File | What it does |
|---|---|
| `src/bptree.h` | `Node`, `NodeStore` interface + `InMemoryNodeStore` mock, `computeN`, `BPlusTree` |
| `src/bulk_load.cpp` | `BPlusTree::bulkLoad`: sort -> pack leaves -> build parent levels |
| `src/bptree.cpp` | `computeN`, mock store, `stats()`, `validate()`, `dump()` |
| `src/task2.cpp` | Task 2 driver: scan data blocks, build, print stats, validate |

## Current output (placeholder node format)

| Statistic | Value |
|---|---|
| n | 408 (leaf: (4096 - 8 header - 4 next) / (4 key + 6 RecordId)) |
| Nodes | 67 (1 root + 66 leaves) |
| Levels | 2 |
| Root keys | 65 keys, 0.342 ... 0.602 |

These will change once Member 2 fixes the header size and whether leaves and internal nodes share one n.

## To do / team decisions

- [ ] **Member 2:** real node header, pointer sizes -> constants at the top of `bptree.h`.
- [ ] **Member 2:** `DiskNodeStore` (one node per block of `index.db`); swap it in for `InMemoryNodeStore` in `task2.cpp`.
- [ ] **Duplicates:** we store one entry per record, so equal keys can span leaves (the root even has `0.500` twice). Member 4's search for `> 0.5` must descend to the first key **> 0.5**, not assume all 0.500s are in one leaf. The alternative is one key per distinct value + a list of RecordIds.
- [ ] **Fill factor:** leaves are packed 100% full. Fine for Task 3 (deletes only cause underflow), but can add a fill-factor parameter if the group prefers.
- [ ] **Clustered or not:** depends on whether Task 1 uses `--sorted` (see Storage README).
