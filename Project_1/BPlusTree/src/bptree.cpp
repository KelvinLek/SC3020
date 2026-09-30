// bptree.cpp - node store, n, statistics, validation and printing for the B+ tree.
#include "bptree.h"

#include <algorithm>
#include <functional>
#include <sstream>
#include <stdexcept>

namespace bptree {

std::size_t computeN(std::size_t blockSize) {
    std::size_t leafN = (blockSize - NODE_HEADER_SIZE - CHILD_PTR_SIZE) / (KEY_SIZE + LEAF_PTR_SIZE);
    std::size_t internalN = (blockSize - NODE_HEADER_SIZE - CHILD_PTR_SIZE) / (KEY_SIZE + CHILD_PTR_SIZE);
    return std::min(leafN, internalN);
}

bool entryLess(const Entry& a, const Entry& b) {
    if (a.key != b.key) return a.key < b.key;
    if (a.rid.block != b.rid.block) return a.rid.block < b.rid.block;
    return a.rid.slot < b.rid.slot;
}

// --- InMemoryNodeStore ------------------------------------------------------

NodeId InMemoryNodeStore::allocate() {
    nodes_.emplace_back();
    return static_cast<NodeId>(nodes_.size() - 1);
}

Node InMemoryNodeStore::read(NodeId id) {
    ++reads_;
    return nodes_.at(id);
}

void InMemoryNodeStore::write(NodeId id, const Node& node) {
    ++writes_;
    nodes_.at(id) = node;
}

// --- BPlusTree ---------------------------------------------------------------

BPlusTree::BPlusTree(NodeStore& store, std::size_t n) : store_(store), n_(n) {
    if (n < 2) throw std::invalid_argument("B+ tree needs n >= 2");
}

TreeStats BPlusTree::stats() {
    // Walks the whole tree. Nodes are counted by reachability (not numNodes()),
    // so the numbers stay right after Task 3 deletes/frees nodes.
    TreeStats s;
    s.n = n_;
    if (root_ == NO_NODE) return s;
    std::vector<NodeId> level{root_};
    while (!level.empty()) {
        ++s.levels;
        std::vector<NodeId> below;
        for (NodeId id : level) {
            Node node = store_.read(id);
            if (id == root_) s.rootKeys = node.keys;
            ++s.numNodes;
            if (node.isLeaf) {
                ++s.numLeaves;
                s.numEntries += node.keys.size();
            } else {
                ++s.numInternal;
                below.insert(below.end(), node.children.begin(), node.children.end());
            }
        }
        level = std::move(below);
    }
    return s;
}

std::string BPlusTree::validate(std::vector<Entry> expected) {
    std::ostringstream err;
    if (root_ == NO_NODE) return "tree has no root";

    std::size_t leafDepth = 0;
    NodeId leftmostLeaf = NO_NODE;

    // Checks the subtree at `id`: fill, sorted keys, and all keys within [lo, hi].
    std::function<bool(NodeId, std::size_t, float, float)> check =
        [&](NodeId id, std::size_t depth, float lo, float hi) -> bool {
        Node node = store_.read(id);
        bool isRoot = (id == root_);
        if (!std::is_sorted(node.keys.begin(), node.keys.end())) {
            err << "node " << id << ": keys not sorted";
            return false;
        }
        if (!node.keys.empty() && (node.keys.front() < lo || node.keys.back() > hi)) {
            err << "node " << id << ": key outside the range allowed by its parent";
            return false;
        }
        if (node.keys.size() > n_) {
            err << "node " << id << ": " << node.keys.size() << " keys > n=" << n_;
            return false;
        }
        if (node.isLeaf) {
            if (node.records.size() != node.keys.size()) {
                err << "leaf " << id << ": #records != #keys";
                return false;
            }
            if (!isRoot && node.keys.size() < minLeafKeys()) {
                err << "leaf " << id << ": underfull (" << node.keys.size() << " keys)";
                return false;
            }
            if (leftmostLeaf == NO_NODE) { leftmostLeaf = id; leafDepth = depth; }
            if (depth != leafDepth) {
                err << "leaf " << id << ": at depth " << depth << ", expected " << leafDepth;
                return false;
            }
            return true;
        }
        if (node.children.size() != node.keys.size() + 1) {
            err << "internal " << id << ": #children != #keys + 1";
            return false;
        }
        std::size_t minChildren = isRoot ? 2 : minInternalChildren();
        if (node.children.size() < minChildren) {
            err << "internal " << id << ": underfull (" << node.children.size() << " children)";
            return false;
        }
        // Child i holds keys in [keys[i-1], keys[i]] (inclusive: duplicates may
        // straddle a separator).
        for (std::size_t i = 0; i < node.children.size(); ++i) {
            float cLo = (i == 0) ? lo : node.keys[i - 1];
            float cHi = (i == node.keys.size()) ? hi : node.keys[i];
            if (!check(node.children[i], depth + 1, cLo, cHi)) return false;
        }
        return true;
    };

    const float inf = std::numeric_limits<float>::infinity();
    if (!check(root_, 1, -inf, inf)) return err.str();

    // Leaf chain must list exactly the expected entries, in sorted order.
    std::sort(expected.begin(), expected.end(), entryLess);
    std::size_t i = 0;
    for (NodeId id = leftmostLeaf; id != NO_NODE;) {
        Node leaf = store_.read(id);
        for (std::size_t j = 0; j < leaf.keys.size(); ++j, ++i) {
            if (i >= expected.size() || leaf.keys[j] != expected[i].key ||
                leaf.records[j].block != expected[i].rid.block ||
                leaf.records[j].slot != expected[i].rid.slot) {
                err << "leaf chain differs from the sorted input at entry " << i;
                return err.str();
            }
        }
        id = leaf.next;
    }
    if (i != expected.size()) {
        err << "leaf chain has " << i << " entries, expected " << expected.size();
        return err.str();
    }
    return "";
}

std::string BPlusTree::dump(std::size_t maxNodesPerLevel) {
    std::ostringstream out;
    if (root_ == NO_NODE) return "(no tree)\n";
    std::vector<NodeId> level{root_};
    for (std::size_t depth = 1; !level.empty(); ++depth) {
        out << "level " << depth << " (" << level.size() << " nodes):";
        std::vector<NodeId> below;
        for (std::size_t k = 0; k < level.size(); ++k) {
            Node node = store_.read(level[k]);
            if (k < maxNodesPerLevel) {
                out << " [";
                for (std::size_t j = 0; j < node.keys.size(); ++j) out << (j ? " " : "") << node.keys[j];
                out << "]";
            }
            if (!node.isLeaf) below.insert(below.end(), node.children.begin(), node.children.end());
        }
        if (level.size() > maxNodesPerLevel) out << " ...";
        out << "\n";
        level = std::move(below);
    }
    return out.str();
}

}  // namespace bptree
