// bulk_load.cpp - builds the B+ tree bottom-up (Task 2, Member 3).
//
// Bulk loading, in three steps:
//   1. Sort all (key, RecordId) entries by key.
//   2. Pack them left to right into leaves of up to n entries, and chain the
//      leaves together with `next` pointers.
//   3. Build each level above from the level below: group up to n+1 nodes under
//      one parent, whose keys are the smallest key of each child except the first.
//      Repeat until a level has a single node: that node is the root.
//
// Every node is packed full, except that the last node of a level may be short.
// If it would be below the minimum fill, entries are moved to it from its left
// neighbour (see groupSizes).
//
// Duplicate keys (e.g. 848 games have FG_PCT_home = 0.500) are stored as separate
// entries, so a run of equal keys can continue from one leaf into the next. A
// parent key then equals the last key of the left child as well, so
// "left subtree <= separator <= right subtree" holds (not strictly less).
// TODO(team): confirm this with Member 2/4; the alternative is one key per value
// with a list of RecordIds.
#include <algorithm>
#include <utility>

#include "bptree.h"

namespace bptree {

namespace {

// Splits `total` items into consecutive groups of at most `maxPer` items. Groups
// are full except the last one; if there is more than one group, the last is
// topped up from the one before it so that it has at least `minPer`.
// This is always possible because maxPer >= 2*minPer - 1 for both node kinds.
std::vector<std::size_t> groupSizes(std::size_t total, std::size_t maxPer, std::size_t minPer) {
    std::vector<std::size_t> sizes;
    for (std::size_t left = total; left > 0; left -= sizes.back())
        sizes.push_back(std::min(left, maxPer));
    if (sizes.size() > 1 && sizes.back() < minPer) {
        std::size_t moved = minPer - sizes.back();
        sizes[sizes.size() - 2] -= moved;
        sizes.back() += moved;
    }
    return sizes;
}

}  // namespace

void BPlusTree::bulkLoad(std::vector<Entry> entries) {
    // Step 1: sort.
    std::sort(entries.begin(), entries.end(), entryLess);

    if (entries.empty()) {                     // empty tree: the root is an empty leaf
        root_ = store_.allocate();
        store_.write(root_, Node{});
        return;
    }

    // One node of the level being built: its id and the smallest key in its subtree
    // (the key its parent needs as a separator).
    struct Built { NodeId id; float minKey; };
    std::vector<Built> level;

    // Step 2: leaves. Allocate all ids first so each leaf can point to the next one.
    std::vector<std::size_t> leafSizes = groupSizes(entries.size(), n_, minLeafKeys());
    std::vector<NodeId> leafIds;
    for (std::size_t i = 0; i < leafSizes.size(); ++i) leafIds.push_back(store_.allocate());

    std::size_t pos = 0;
    for (std::size_t i = 0; i < leafSizes.size(); ++i) {
        Node leaf;
        leaf.isLeaf = true;
        for (std::size_t j = 0; j < leafSizes[i]; ++j, ++pos) {
            leaf.keys.push_back(entries[pos].key);
            leaf.records.push_back(entries[pos].rid);
        }
        leaf.next = (i + 1 < leafIds.size()) ? leafIds[i + 1] : NO_NODE;
        store_.write(leafIds[i], leaf);
        level.push_back({leafIds[i], leaf.keys.front()});
    }

    // Step 3: internal levels, until one node is left.
    while (level.size() > 1) {
        std::vector<Built> parents;
        std::vector<std::size_t> sizes = groupSizes(level.size(), n_ + 1, minInternalChildren());
        std::size_t c = 0;
        for (std::size_t size : sizes) {
            Node parent;
            parent.isLeaf = false;
            float minKey = level[c].minKey;
            for (std::size_t j = 0; j < size; ++j, ++c) {
                parent.children.push_back(level[c].id);
                if (j > 0) parent.keys.push_back(level[c].minKey);
            }
            NodeId id = store_.allocate();
            store_.write(id, parent);
            parents.push_back({id, minKey});
        }
        level = std::move(parents);
    }
    root_ = level.front().id;
}

}  // namespace bptree
