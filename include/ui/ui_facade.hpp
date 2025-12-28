#pragma once

#include <SFML/Graphics.hpp>

#include "game/game.hpp"

// Thin wrapper exposing only the surface used by the UI layer.
// This keeps ImGui code from depending on the full Game implementation details.
class UiFacade {
public:
    using CursorMode = Game::CursorMode;
    using AddType = Game::AddType;
    using SelectionMode = Game::SelectionMode;

    explicit UiFacade(Game& game_ref) : game(&game_ref) {}

    // Cursor & spawning
    void set_cursor_mode(CursorMode mode) { game->set_cursor_mode(mode); }
    CursorMode get_cursor_mode() const { return game->get_cursor_mode(); }
    void set_add_type(AddType type) { game->set_add_type(type); }
    AddType get_add_type() const { return game->get_add_type(); }
    void set_add_eatable_area(float area) { game->set_add_eatable_area(area); }
    float get_add_eatable_area() const { return game->get_add_eatable_area(); }

    // Time & pause
    void set_time_scale(float scale) { game->set_time_scale(scale); }
    float get_time_scale() const { return game->get_time_scale(); }
    void set_paused(bool p) { game->set_paused(p); }
    bool is_paused() const { return game->is_paused(); }
    float get_sim_time() const { return game->get_sim_time(); }
    float get_real_time() const { return game->get_real_time(); }
    float get_actual_sim_speed() const { return game->get_actual_sim_speed(); }
    float get_last_fps() const { return game->get_last_fps(); }

    // Region
    void set_petri_radius(float r) { game->set_petri_radius(r); }
    float get_petri_radius() const { return game->get_petri_radius(); }
    void set_auto_remove_outside(bool enabled) { game->set_auto_remove_outside(enabled); }
    bool get_auto_remove_outside() const { return game->get_auto_remove_outside(); }

    // Brain & creature
    void set_brain_updates_per_sim_second(float hz) { game->set_brain_updates_per_sim_second(hz); }
    float get_brain_updates_per_sim_second() const { return game->get_brain_updates_per_sim_second(); }
    void set_minimum_area(float area) { game->set_minimum_area(area); }
    float get_minimum_area() const { return game->get_minimum_area(); }
    void set_boost_area(float area) { game->set_boost_area(area); }
    float get_boost_area() const { return game->get_boost_area(); }
    void set_average_creature_area(float area) { game->set_average_creature_area(area); }
    float get_average_creature_area() const { return game->get_average_creature_area(); }
    void set_show_true_color(bool value) { game->set_show_true_color(value); }
    bool get_show_true_color() const { return game->get_show_true_color(); }
    void set_selected_creature_possessed(bool possessed) { game->set_selected_creature_possessed(possessed); }
    bool is_selected_creature_possessed() const { return game->is_selected_creature_possessed(); }

    // Mutation config
    void set_add_node_thresh(float p) { game->set_add_node_thresh(p); }
    float get_add_node_thresh() const { return game->get_add_node_thresh(); }
    void set_add_connection_thresh(float p) { game->set_add_connection_thresh(p); }
    float get_add_connection_thresh() const { return game->get_add_connection_thresh(); }
    void set_tick_add_node_thresh(float p) { game->set_tick_add_node_thresh(p); }
    float get_tick_add_node_thresh() const { return game->get_tick_add_node_thresh(); }
    void set_tick_add_connection_thresh(float p) { game->set_tick_add_connection_thresh(p); }
    float get_tick_add_connection_thresh() const { return game->get_tick_add_connection_thresh(); }
    void set_weight_extremum_init(float v) { game->set_weight_extremum_init(v); }
    float get_weight_extremum_init() const { return game->get_weight_extremum_init(); }
    void set_live_mutation_enabled(bool enabled) { game->set_live_mutation_enabled(enabled); }
    bool get_live_mutation_enabled() const { return game->get_live_mutation_enabled(); }
    void set_mutate_weight_thresh(float v) { game->set_mutate_weight_thresh(v); }
    float get_mutate_weight_thresh() const { return game->get_mutate_weight_thresh(); }
    void set_mutate_weight_full_change_thresh(float v) { game->set_mutate_weight_full_change_thresh(v); }
    float get_mutate_weight_full_change_thresh() const { return game->get_mutate_weight_full_change_thresh(); }
    void set_mutate_weight_factor(float v) { game->set_mutate_weight_factor(v); }
    float get_mutate_weight_factor() const { return game->get_mutate_weight_factor(); }
    void set_max_iterations_find_connection_thresh(int v) { game->set_max_iterations_find_connection_thresh(v); }
    int get_max_iterations_find_connection_thresh() const { return game->get_max_iterations_find_connection_thresh(); }
    void set_reactivate_connection_thresh(float v) { game->set_reactivate_connection_thresh(v); }
    float get_reactivate_connection_thresh() const { return game->get_reactivate_connection_thresh(); }
    void set_max_iterations_find_node_thresh(int v) { game->set_max_iterations_find_node_thresh(v); }
    int get_max_iterations_find_node_thresh() const { return game->get_max_iterations_find_node_thresh(); }
    void set_mutate_allow_recurrent(bool v) { game->set_mutate_allow_recurrent(v); }
    bool get_mutate_allow_recurrent() const { return game->get_mutate_allow_recurrent(); }
    void set_init_add_node_thresh(float p) { game->set_init_add_node_thresh(p); }
    float get_init_add_node_thresh() const { return game->get_init_add_node_thresh(); }
    void set_init_add_connection_thresh(float p) { game->set_init_add_connection_thresh(p); }
    float get_init_add_connection_thresh() const { return game->get_init_add_connection_thresh(); }
    void set_init_mutation_rounds(int rounds) { game->set_init_mutation_rounds(rounds); }
    int get_init_mutation_rounds() const { return game->get_init_mutation_rounds(); }
    void set_mutation_rounds(int rounds) { game->set_mutation_rounds(rounds); }
    int get_mutation_rounds() const { return game->get_mutation_rounds(); }

    // Movement
    void set_circle_density(float d) { game->set_circle_density(d); }
    float get_circle_density() const { return game->get_circle_density(); }
    void set_linear_impulse_magnitude(float m) { game->set_linear_impulse_magnitude(m); }
    float get_linear_impulse_magnitude() const { return game->get_linear_impulse_magnitude(); }
    void set_angular_impulse_magnitude(float m) { game->set_angular_impulse_magnitude(m); }
    float get_angular_impulse_magnitude() const { return game->get_angular_impulse_magnitude(); }
    void set_linear_damping(float d) { game->set_linear_damping(d); }
    float get_linear_damping() const { return game->get_linear_damping(); }
    void set_angular_damping(float d) { game->set_angular_damping(d); }
    float get_angular_damping() const { return game->get_angular_damping(); }
    void set_boost_particle_impulse_fraction(float f) { game->set_boost_particle_impulse_fraction(f); }
    float get_boost_particle_impulse_fraction() const { return game->get_boost_particle_impulse_fraction(); }
    void set_boost_particle_linear_damping(float d) { game->set_boost_particle_linear_damping(d); }
    float get_boost_particle_linear_damping() const { return game->get_boost_particle_linear_damping(); }

    // Dish & pellets
    void set_minimum_creature_count(int count) { game->set_minimum_creature_count(count); }
    int get_minimum_creature_count() const { return game->get_minimum_creature_count(); }
    void set_sprinkle_rate_eatable(float r) { game->set_sprinkle_rate_eatable(r); }
    void set_sprinkle_rate_toxic(float r) { game->set_sprinkle_rate_toxic(r); }
    void set_sprinkle_rate_division(float r) { game->set_sprinkle_rate_division(r); }
    float get_sprinkle_rate_eatable() const { return game->get_sprinkle_rate_eatable(); }
    float get_sprinkle_rate_toxic() const { return game->get_sprinkle_rate_toxic(); }
    float get_sprinkle_rate_division() const { return game->get_sprinkle_rate_division(); }
    void set_max_food_pellets(int v) { game->set_max_food_pellets(v); }
    int get_max_food_pellets() const { return game->get_max_food_pellets(); }
    void set_max_toxic_pellets(int v) { game->set_max_toxic_pellets(v); }
    int get_max_toxic_pellets() const { return game->get_max_toxic_pellets(); }
    void set_max_division_pellets(int v) { game->set_max_division_pellets(v); }
    int get_max_division_pellets() const { return game->get_max_division_pellets(); }
    void set_food_pellet_density(float d) { game->set_food_pellet_density(d); }
    float get_food_pellet_density() const { return game->get_food_pellet_density(); }
    void set_toxic_pellet_density(float d) { game->set_toxic_pellet_density(d); }
    float get_toxic_pellet_density() const { return game->get_toxic_pellet_density(); }
    void set_division_pellet_density(float d) { game->set_division_pellet_density(d); }
    float get_division_pellet_density() const { return game->get_division_pellet_density(); }
    std::size_t get_food_pellet_count() const { return game->get_food_pellet_count(); }
    std::size_t get_toxic_pellet_count() const { return game->get_toxic_pellet_count(); }
    std::size_t get_division_pellet_count() const { return game->get_division_pellet_count(); }

    // Death & reproduction
    void set_creature_cloud_area_percentage(float percentage) { game->set_creature_cloud_area_percentage(percentage); }
    float get_creature_cloud_area_percentage() const { return game->get_creature_cloud_area_percentage(); }
    void set_division_pellet_divide_probability(float p) { game->set_division_pellet_divide_probability(p); }
    float get_division_pellet_divide_probability() const { return game->get_division_pellet_divide_probability(); }
    void set_inactivity_timeout(float t) { game->set_inactivity_timeout(t); }
    float get_inactivity_timeout() const { return game->get_inactivity_timeout(); }
    void set_poison_death_probability(float p) { game->set_poison_death_probability(p); }
    float get_poison_death_probability() const { return game->get_poison_death_probability(); }
    void set_poison_death_probability_normal(float p) { game->set_poison_death_probability_normal(p); }
    float get_poison_death_probability_normal() const { return game->get_poison_death_probability_normal(); }

    // Selection
    void set_follow_selected(bool v) { game->set_follow_selected(v); }
    bool get_follow_selected() const { return game->get_follow_selected(); }
    void set_selection_mode(SelectionMode mode) { game->set_selection_mode(mode); }
    SelectionMode get_selection_mode() const { return game->get_selection_mode(); }
    const neat::Genome* get_selected_brain() const { return game->get_selected_brain(); }
    const CreatureCircle* get_selected_creature() const { return game->get_selected_creature(); }
    int get_selected_generation() const { return game->get_selected_generation(); }

    // Population & stats
    void remove_random_percentage(float percentage) { game->remove_random_percentage(percentage); }
    std::size_t get_circle_count() const { return game->get_circle_count(); }
    std::size_t get_creature_count() const { return game->get_creature_count(); }
    float get_longest_life_since_creation() const { return game->get_longest_life_since_creation(); }
    float get_longest_life_since_division() const { return game->get_longest_life_since_division(); }
    int get_max_generation() const { return game->get_max_generation(); }

private:
    Game* game;
};
