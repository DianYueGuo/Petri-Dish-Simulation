#pragma once

#include <unordered_map>

#include "circles/circle_capabilities.hpp"
#include "circles/contact_graph.hpp"
#include "circles/circle_physics.hpp"

class CircleRegistry {
public:
    void register_circle(CirclePhysics& circle) {
        entries[circle.get_id()].physics = &circle;
    }

    void register_capabilities(CirclePhysics& circle) {
        CircleId id = circle.get_id();
        auto& entry = entries[id];
        entry.physics = &circle;
        if (auto* senseable = dynamic_cast<ISenseable*>(&circle)) {
            entry.senseable = senseable;
        }
        if (auto* edible = dynamic_cast<IEdible*>(&circle)) {
            entry.edible = edible;
        }
    }

    void unregister_circle(const CirclePhysics& circle) {
        entries.erase(circle.get_id());
    }

    const ISenseable* get_senseable(CircleId id) const {
        auto it = entries.find(id);
        if (it == entries.end()) return nullptr;
        return it->second.senseable;
    }

    IEdible* get_edible(CircleId id) const {
        auto it = entries.find(id);
        if (it == entries.end()) return nullptr;
        return it->second.edible;
    }

    CirclePhysics* get_physics(CircleId id) const {
        auto it = entries.find(id);
        if (it == entries.end()) return nullptr;
        return it->second.physics;
    }

private:
    struct Entry {
        CirclePhysics* physics = nullptr;
        ISenseable* senseable = nullptr;
        IEdible* edible = nullptr;
    };
    std::unordered_map<CircleId, Entry> entries;
};
