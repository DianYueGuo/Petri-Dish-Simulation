#pragma once

#include <functional>
#include <memory>

#include <box2d/box2d.h>

class EatableCircle;
class Game;

// Minimal interface exposing simulation/game data needed by CreatureCircle behavior and division.
class CreatureContext {
public:
    virtual ~CreatureContext() = default;

    virtual float cc_boost_area() const = 0;
    virtual float cc_circle_density() const = 0;
    virtual float cc_boost_particle_impulse_fraction() const = 0;
    virtual float cc_boost_particle_linear_damping() const = 0;
    virtual float cc_linear_impulse_magnitude() const = 0;
    virtual float cc_angular_impulse_magnitude() const = 0;
    virtual float cc_linear_damping() const = 0;
    virtual float cc_angular_damping() const = 0;
    virtual bool cc_live_mutation_enabled() const = 0;
    virtual float cc_mutate_weight_thresh() const = 0;
    virtual float cc_mutate_weight_full_change_thresh() const = 0;
    virtual float cc_mutate_weight_factor() const = 0;
    virtual float cc_tick_add_connection_thresh() const = 0;
    virtual float cc_tick_add_node_thresh() const = 0;
    virtual int cc_max_iterations_find_connection() const = 0;
    virtual int cc_max_iterations_find_node() const = 0;
    virtual float cc_reactivate_connection_thresh() const = 0;
    virtual float cc_minimum_area() const = 0;
    virtual bool cc_show_true_color() const = 0;
    virtual float cc_poison_death_probability() const = 0;
    virtual float cc_poison_death_probability_normal() const = 0;
    virtual float cc_inactivity_timeout() const = 0;
    virtual int cc_init_mutation_rounds() const = 0;
    virtual float cc_init_add_node_thresh() const = 0;
    virtual float cc_init_add_connection_thresh() const = 0;
    virtual int cc_mutation_rounds() const = 0;
    virtual float cc_add_connection_thresh() const = 0;
    virtual float cc_add_node_thresh() const = 0;
    virtual float cc_sim_time() const = 0;

    virtual class Game& cc_owner_game() = 0;

    virtual bool cc_selected_and_possessed(const void* creature_ptr) const = 0;
    virtual bool cc_left_key_down() const = 0;
    virtual bool cc_right_key_down() const = 0;
    virtual bool cc_space_key_down() const = 0;

    virtual void cc_spawn_circle(std::unique_ptr<EatableCircle> circle) = 0;
};
