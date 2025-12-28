#pragma once

#include <unordered_map>
#include <unordered_set>
#include <functional>

struct CircleId {
    uint32_t value = 0;
    bool operator==(const CircleId& other) const { return value == other.value; }
    bool operator!=(const CircleId& other) const { return value != other.value; }
};

namespace std {
template<>
struct hash<CircleId> {
    size_t operator()(const CircleId& id) const noexcept {
        return std::hash<uint32_t>()(id.value);
    }
};
} // namespace std

class ContactGraph {
public:
    void add_contact(CircleId a, CircleId b) {
        if (a == b) return;
        adjacency[a].insert(b);
        adjacency[b].insert(a);
    }

    void remove_contact(CircleId a, CircleId b) {
        if (a == b) return;
        auto itA = adjacency.find(a);
        if (itA != adjacency.end()) {
            itA->second.erase(b);
            if (itA->second.empty()) adjacency.erase(itA);
        }
        auto itB = adjacency.find(b);
        if (itB != adjacency.end()) {
            itB->second.erase(a);
            if (itB->second.empty()) adjacency.erase(itB);
        }
    }

    void remove_circle(CircleId id) {
        auto it = adjacency.find(id);
        if (it == adjacency.end()) return;
        for (CircleId neighbor : it->second) {
            auto nb = adjacency.find(neighbor);
            if (nb != adjacency.end()) {
                nb->second.erase(id);
                if (nb->second.empty()) adjacency.erase(nb);
            }
        }
        adjacency.erase(it);
    }

    template <typename Fn>
    void for_each_neighbor(CircleId id, const Fn& fn) const {
        auto it = adjacency.find(id);
        if (it == adjacency.end()) return;
        for (CircleId neighbor : it->second) {
            fn(neighbor);
        }
    }

private:
    std::unordered_map<CircleId, std::unordered_set<CircleId>> adjacency;
};
