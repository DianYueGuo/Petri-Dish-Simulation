#pragma once

#include <memory>
#include <vector>

#include <box2d/box2d.h>

class EatableCircle;
class CreatureCircle;
class SelectionManager;
class GameSelectionController;
class ContactGraph;
class CircleRegistry;

// Minimal surface GamePopulationManager needs to mutate simulation state.
class PopulationContext {
public:
    virtual ~PopulationContext() = default;

    virtual SelectionManager& population_selection() = 0;
    virtual GameSelectionController& population_selection_controller() = 0;
    virtual ContactGraph& population_contact_graph() = 0;
    virtual CircleRegistry& population_circle_registry() = 0;

    virtual std::vector<std::unique_ptr<EatableCircle>>& population_circles() = 0;

    virtual float population_sim_time() const = 0;
    virtual float population_dish_radius() const = 0;
    virtual float population_add_eatable_area() const = 0;
    virtual float population_creature_cloud_area_percentage() const = 0;
    virtual float population_food_density() const = 0;
    virtual float population_toxic_density() const = 0;
    virtual float population_division_density() const = 0;

    virtual void population_set_sprinkle_rate_eatable(float v) = 0;
    virtual void population_set_sprinkle_rate_toxic(float v) = 0;
    virtual void population_set_sprinkle_rate_division(float v) = 0;
    virtual float population_get_sprinkle_rate_eatable() const = 0;
    virtual float population_get_sprinkle_rate_toxic() const = 0;
    virtual float population_get_sprinkle_rate_division() const = 0;

    virtual void population_set_cleanup_rate_food(float v) = 0;
    virtual void population_set_cleanup_rate_toxic(float v) = 0;
    virtual void population_set_cleanup_rate_division(float v) = 0;
    virtual float population_get_cleanup_rate_food() const = 0;
    virtual float population_get_cleanup_rate_toxic() const = 0;
    virtual float population_get_cleanup_rate_division() const = 0;

    virtual std::size_t population_get_food_cached() const = 0;
    virtual std::size_t population_get_toxic_cached() const = 0;
    virtual std::size_t population_get_division_cached() const = 0;
    virtual void population_adjust_pellet_count(const EatableCircle* circle, int delta) = 0;

    virtual void population_on_creature_added(const CreatureCircle& creature) = 0;

    virtual void population_spawn_cloud(const CreatureCircle& creature, std::vector<std::unique_ptr<EatableCircle>>& out) = 0;
};
