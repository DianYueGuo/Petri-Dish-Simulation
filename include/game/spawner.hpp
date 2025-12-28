#pragma once

#include <memory>
#include <optional>
#include <vector>

#include <SFML/System/Vector2.hpp>
#include <box2d/box2d.h>

class EatableCircle;
class CreatureCircle;
class SpawnContext;

// Responsible for creating circles and spawning logic.
class Spawner {
public:
    explicit Spawner(SpawnContext& context_ref);

    void spawn_selected_type_at(const sf::Vector2f& worldPos);
    void begin_add_drag_if_applicable(const sf::Vector2f& worldPos);
    void continue_add_drag(const sf::Vector2f& worldPos);
    void reset_add_drag_state();

    void sprinkle_entities(float dt);
    void ensure_minimum_creatures();
    b2Vec2 random_point_in_petri() const;
    std::unique_ptr<CreatureCircle> create_creature_at(const b2Vec2& pos);
    std::unique_ptr<EatableCircle> create_eatable_at(const b2Vec2& pos, bool toxic, bool division_pellet = false) const;
    void spawn_eatable_cloud(const CreatureCircle& creature, std::vector<std::unique_ptr<EatableCircle>>& out);

private:
    bool pellet_cap_reached(int add_type_value) const;
    void sprinkle_with_rate(float rate, int type, float dt);

    SpawnContext& context;
    bool add_dragging = false;
    std::optional<sf::Vector2f> last_add_world_pos;
    std::optional<sf::Vector2f> last_drag_world_pos;
    float add_drag_distance = 0.0f;
};
