#pragma once

#include <memory>
#include <vector>

#include <box2d/box2d.h>

class Game;
class EatableCircle;

// Interface describing the minimal surface Spawner needs from the simulation.
class SpawnContext {
public:
    enum class AddType { Creature, FoodPellet, ToxicPellet, DivisionPellet };
    enum class CursorMode { Add, Select };

    virtual ~SpawnContext() = default;

    // Input state
    virtual AddType spawn_get_add_type() const = 0;
    virtual CursorMode spawn_get_cursor_mode() const = 0;

    // Counts and limits
    virtual std::size_t spawn_get_food_pellet_count() const = 0;
    virtual std::size_t spawn_get_toxic_pellet_count() const = 0;
    virtual std::size_t spawn_get_division_pellet_count() const = 0;
    virtual int spawn_get_max_food_pellets() const = 0;
    virtual int spawn_get_max_toxic_pellets() const = 0;
    virtual int spawn_get_max_division_pellets() const = 0;
    virtual std::size_t spawn_get_creature_count() const = 0;
    virtual int spawn_get_minimum_creature_count() const = 0;

    // Config values
    virtual float spawn_get_add_eatable_area() const = 0;
    virtual float spawn_get_petri_radius() const = 0;
    virtual float spawn_get_circle_density() const = 0;
    virtual float spawn_get_linear_impulse_magnitude() const = 0;
    virtual float spawn_get_angular_impulse_magnitude() const = 0;
    virtual float spawn_get_linear_damping() const = 0;
    virtual float spawn_get_angular_damping() const = 0;
    virtual float spawn_get_average_creature_area() const = 0;
    virtual float spawn_get_sprinkle_rate_eatable() const = 0;
    virtual float spawn_get_sprinkle_rate_toxic() const = 0;
    virtual float spawn_get_sprinkle_rate_division() const = 0;
    virtual int spawn_get_init_mutation_rounds() const = 0;
    virtual float spawn_get_init_add_node_thresh() const = 0;
    virtual float spawn_get_init_add_connection_thresh() const = 0;
    virtual float spawn_get_minimum_area() const = 0;
    virtual float spawn_get_creature_cloud_area_percentage() const = 0;
    virtual float spawn_get_sim_time() const = 0;

    // Innovation bookkeeping
    virtual std::vector<std::vector<int>>* spawn_get_neat_innovations() const = 0;
    virtual int* spawn_get_neat_last_innovation_id() const = 0;

    // World access
    virtual b2WorldId spawn_get_world_id() const = 0;
    virtual Game* spawn_get_owner_game() const = 0;

    // Mutations to the world
    virtual void spawn_add_circle(std::unique_ptr<EatableCircle> circle) = 0;
    virtual void spawn_update_max_generation_from_circle(const EatableCircle* circle) = 0;
};
