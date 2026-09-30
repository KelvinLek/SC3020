// bptree.h - B+ tree on FG_PCT_home (Task 2) - Member 3 skeleton.
//
// STATUS: the construction algorithm (bulk loading), statistics and validation
// work now against an IN-MEMORY node store. Member 2 owns the final node layout
// and on-disk format; once it is agreed, only the parts marked TODO(Member 2)
// need to change - the bulk loader talks to nodes only through NodeStore.
//
// Terminology (Lecture notes): n = the maximum number of keys in a node.
//   leaf node      : up to n (key, record pointer) pairs + 1 pointer to the next leaf
//   internal node  : up to n keys and n+1 child pointers
//   minimum fill   : leaf >= floor((n+1)/2) keys, internal >= floor(n/2) keys,
//                    root >= 1 key (i.e. >= 2 children), unless the root is a leaf
#pragma once
#include <cstddef>
#include <cstdint>
#include <limits>
#include <string>
#include <vector>

#include "config.h"   // BLOCK_SIZE (Member 1)
#include "record.h"   // RecordId, RECORD_ID_SIZE (Member 1)

namespace bptree {

using storage::RecordId;
using NodeId = std::uint32_t;                 // on disk this will be a BlockId in index.db
constexpr NodeId NO_NODE = std::numeric_limits<NodeId>::max();

// ---------------------------------------------------------------------------
// Parameter n
// ---------------------------------------------------------------------------
// TODO(Member 2): replace these with the real node header / pointer sizes.
constexpr std::size_t NODE_HEADER_SIZE = 8;                        // isLeaf, numKeys, ...
constexpr std::size_t KEY_SIZE = sizeof(float);                    // FG_PCT_home
constexpr std::size_t CHILD_PTR_SIZE = sizeof(storage::BlockId);   // internal -> child
constexpr std::size_t LEAF_PTR_SIZE = storage::RECORD_ID_SIZE;     // leaf -> record

// Largest n such that both a full leaf and a full internal node fit in one block:
//   leaf     : header + n*(key + recordPtr) + nextLeafPtr   <= BLOCK_SIZE
//   internal : header + n*key + (n+1)*childPtr              <= BLOCK_SIZE
std::size_t computeN(std::size_t blockSize = storage::BLOCK_SIZE);

// ---------------------------------------------------------------------------
// Node (in-memory form)
// ---------------------------------------------------------------------------
struct Node {
    bool isLeaf = true;
    std::vector<float> keys;          // sorted, non-decreasing (duplicates allowed)
    std::vector<RecordId> records;    // leaf only: records[i] belongs to keys[i]
    std::vector<NodeId> children;     // internal only: children.size() == keys.size() + 1
    NodeId next = NO_NODE;            // leaf only: right sibling in the leaf chain
};

// ---------------------------------------------------------------------------
// Where nodes live. Every read()/write() counts as one index-node access, so the
// same counters serve Task 3 ("number of index nodes the process accesses").
// ---------------------------------------------------------------------------
class NodeStore {
public:
    virtual ~NodeStore() = default;
    virtual NodeId allocate() = 0;                          // new empty node
    virtual Node read(NodeId id) = 0;                       // 1 node access
    virtual void write(NodeId id, const Node& node) = 0;    // 1 node access
    virtual std::size_t numNodes() const = 0;               // nodes allocated

    std::uint64_t reads() const { return reads_; }
    std::uint64_t writes() const { return writes_; }
    void resetCounters() { reads_ = writes_ = 0; }

protected:
    std::uint64_t reads_ = 0;
    std::uint64_t writes_ = 0;
};

// Mock store: a vector of nodes. Good enough to develop and test the algorithm.
class InMemoryNodeStore : public NodeStore {
public:
    NodeId allocate() override;
    Node read(NodeId id) override;
    void write(NodeId id, const Node& node) override;
    std::size_t numNodes() const override { return nodes_.size(); }

private:
    std::vector<Node> nodes_;
};

// TODO(Member 2): class DiskNodeStore : public NodeStore that keeps one node per
// block of its own storage::Disk ("index.db"): allocate() = disk.allocateBlock(),
// read() = disk.readBlock() + deserialise, write() = serialise + disk.writeBlock().

// ---------------------------------------------------------------------------
// The tree
// ---------------------------------------------------------------------------
struct Entry {                        // one leaf entry: FG_PCT_home + where the record is
    float key;
    RecordId rid;
};

struct TreeStats {
    std::size_t n = 0;
    std::size_t numNodes = 0;
    std::size_t numLeaves = 0;
    std::size_t numInternal = 0;
    std::size_t levels = 0;
    std::size_t numEntries = 0;
    std::vector<float> rootKeys;
};

class BPlusTree {
public:
    BPlusTree(NodeStore& store, std::size_t n);

    // Task 2: build the tree bottom-up from all entries (sorted inside).
    void bulkLoad(std::vector<Entry> entries);

    NodeId root() const { return root_; }
    std::size_t n() const { return n_; }
    NodeStore& store() { return store_; }

    TreeStats stats();                                   // Task 2 statistics
    // Checks every B+ tree invariant and that the leaf chain holds exactly `expected`
    // in sorted order. Returns "" if OK, otherwise a description of the first problem.
    std::string validate(std::vector<Entry> expected);
    std::string dump(std::size_t maxNodesPerLevel = 20);  // level-by-level keys, for demos

    std::size_t minLeafKeys() const { return (n_ + 1) / 2; }
    std::size_t minInternalChildren() const { return n_ / 2 + 1; }

private:
    NodeStore& store_;
    std::size_t n_;
    NodeId root_ = NO_NODE;
};

// Sort order used everywhere: by key, ties broken by physical position so the
// build is deterministic.
bool entryLess(const Entry& a, const Entry& b);

}  // namespace bptree
