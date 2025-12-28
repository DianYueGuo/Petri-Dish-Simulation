#pragma once

#include <memory>
#include <vector>

#include <SFML/Graphics/View.hpp>
#include <box2d/box2d.h>

class EatableCircle;
class CreatureCircle;
class Spawner;
class ContactGraph;
class CircleRegistry;
class Game;

// Minimal surface consumed by GameSimulationController.
class SimulationContext {
public:
    virtual ~SimulationContext() = default;

    virtual b2WorldId sim_world_id() const = 0;
    virtual float sim_time_scale() const = 0;
    virtual void sim_accumulate_desired_time(float dt_scaled) = 0;
    virtual float sim_desired_time() const = 0;
    virtual void sim_set_desired_time(float t) = 0;
    virtual float sim_time_accum() const = 0;
    virtual void sim_add_time(float dt) = 0;
    virtual float sim_last_real_dt() const = 0;
    virtual void sim_set_last_real_dt(float dt) = 0;
    virtual float sim_last_sim_dt() const = 0;
    virtual void sim_set_last_sim_dt(float dt) = 0;
    virtual void sim_set_actual_speed(float v) = 0;

    virtual bool sim_is_paused() const = 0;

    virtual ContactGraph& sim_contact_graph() = 0;
    virtual CircleRegistry& sim_circle_registry() = 0;
    virtual std::vector<std::unique_ptr<EatableCircle>>& sim_circles() = 0;

    virtual float sim_brain_updates_per_second() const = 0;
    virtual float sim_brain_time_accum() const = 0;
    virtual void sim_set_brain_time_accum(float t) = 0;
    virtual void sim_sprinkle_entities(float timeStep) = 0;
    virtual float sim_boost_area() const = 0;
    virtual float sim_circle_density() const = 0;
    virtual float sim_boost_particle_impulse_fraction() const = 0;
    virtual float sim_boost_particle_linear_damping() const = 0;
    virtual float sim_linear_impulse_magnitude() const = 0;
    virtual float sim_angular_impulse_magnitude() const = 0;
    virtual float sim_linear_damping() const = 0;
    virtual float sim_angular_damping() const = 0;
    virtual void sim_set_linear_impulse_magnitude(float v) = 0;
    virtual void sim_set_angular_impulse_magnitude(float v) = 0;
    virtual void sim_set_linear_damping(float v) = 0;
    virtual void sim_set_angular_damping(float v) = 0;
    virtual bool sim_live_mutation_enabled() const = 0;
    virtual float sim_mutate_weight_thresh() const = 0;
    virtual float sim_mutate_weight_full_change_thresh() const = 0;
    virtual float sim_mutate_weight_factor() const = 0;
    virtual float sim_tick_add_connection_thresh() const = 0;
    virtual float sim_tick_add_node_thresh() const = 0;
    virtual int sim_max_iterations_find_connection_thresh() const = 0;
    virtual int sim_max_iterations_find_node_thresh() const = 0;
    virtual float sim_reactivate_connection_thresh() const = 0;
    virtual float sim_minimum_area() const = 0;
    virtual bool sim_show_true_color() const = 0;
    virtual float sim_petri_radius() const = 0;
    virtual float sim_poison_death_probability() const = 0;
    virtual float sim_poison_death_probability_normal() const = 0;
    virtual float sim_inactivity_timeout() const = 0;
    virtual int sim_init_mutation_rounds() const = 0;
    virtual float sim_init_add_node_thresh() const = 0;
    virtual float sim_init_add_connection_thresh() const = 0;
    virtual int sim_mutation_rounds() const = 0;
    virtual float sim_add_connection_thresh() const = 0;
    virtual float sim_add_node_thresh() const = 0;

    virtual const CreatureCircle* sim_selected_creature() const = 0;
    virtual bool sim_is_selected_possessed() const = 0;
    virtual bool sim_left_key_down() const = 0;
    virtual bool sim_right_key_down() const = 0;
    virtual bool sim_up_key_down() const = 0;
    virtual bool sim_space_key_down() const = 0;

    virtual void sim_frame_rendered() = 0;
    virtual void sim_cleanup_population(float timeStep) = 0;
    virtual void sim_remove_outside_if_enabled() = 0;
    virtual void sim_update_selection_after_step() = 0;
    virtual Game& sim_owner_game() = 0;
};
