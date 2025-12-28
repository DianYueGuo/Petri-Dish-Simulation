#pragma once

#include <memory>
#include <optional>
#include <vector>
#include <algorithm>

#include <SFML/Graphics.hpp>
#include <box2d/box2d.h>
#include <NEAT/genome.hpp>

#include "circle_registry.hpp"
#include "eatable_circle.hpp"
#include "game/selection_manager.hpp"
#include "game/spawner.hpp"

class CreatureCircle;
class GameInputHandler;
class GameSelectionController;
class GamePopulationManager;
class GameSimulationController;

class Game {
    friend class Spawner;
    friend class CreatureCircle;
    friend class GameInputHandler;
    friend class GameSelectionController;
    friend class GamePopulationManager;
    friend class GameSimulationController;

public:
    enum class CursorMode { Add, Select };
    enum class AddType { Creature, FoodPellet, ToxicPellet, DivisionPellet };
    enum class SelectionMode {
        Manual = 0,
        OldestLargest,
#ifndef NDEBUG
        OldestMedian,
#endif
        OldestSmallest
    };

    Game();
    ~Game();

    // Core lifecycle
    void process_game_logic_with_speed();
    void process_game_logic();
    void draw(sf::RenderWindow& window) const;
    void process_input_events(sf::RenderWindow& window, const std::optional<sf::Event>& event);

    // Time & pause
    void set_time_scale(float scale) { timing.time_scale = scale; }
    float get_time_scale() const { return timing.time_scale; }
    void set_paused(bool p) { paused = p; }
    bool is_paused() const { return paused; }
    void accumulate_real_time(float dt);
    void frame_rendered();
    float get_sim_time() const { return timing.sim_time_accum; }
    float get_real_time() const { return timing.real_time_accum; }
    float get_actual_sim_speed() const { return timing.actual_sim_speed_inst; }
    float get_last_fps() const { return fps.last; }
    ContactGraph& get_contact_graph() { return contact_graph; }
    const ContactGraph& get_contact_graph() const { return contact_graph; }
    CircleRegistry& get_circle_registry() { return circle_registry; }
    const CircleRegistry& get_circle_registry() const { return circle_registry; }

    // Cursor & spawning
    void set_cursor_mode(CursorMode mode) { cursor.mode = mode; }
    CursorMode get_cursor_mode() const { return cursor.mode; }
    void set_add_type(AddType type) { cursor.add_type = type; }
    AddType get_add_type() const { return cursor.add_type; }
    void set_add_eatable_area(float area) { creature.add_eatable_area = area; }
    float get_add_eatable_area() const { return creature.add_eatable_area; }

    // Brain & creature
    void set_brain_updates_per_sim_second(float hz) { brain.updates_per_second = hz; }
    float get_brain_updates_per_sim_second() const { return brain.updates_per_second; }
    void set_minimum_area(float area) { creature.minimum_area = area; }
    float get_minimum_area() const { return creature.minimum_area; }
    void set_poison_death_probability(float p) { death.poison_death_probability = p; }
    float get_poison_death_probability() const { return death.poison_death_probability; }
    void set_poison_death_probability_normal(float p) { death.poison_death_probability_normal = p; }
    float get_poison_death_probability_normal() const { return death.poison_death_probability_normal; }
    void set_boost_area(float area) { creature.boost_area = area; }
    float get_boost_area() const { return creature.boost_area; }
    void set_average_creature_area(float area) { creature.average_area = area; }
    float get_average_creature_area() const { return creature.average_area; }

    // Mutation config
    void set_add_node_thresh(float p) { mutation.add_node_thresh = std::clamp(p, 0.0f, 1.0f); }
    float get_add_node_thresh() const { return mutation.add_node_thresh; }
    void set_add_connection_thresh(float p) { mutation.add_connection_thresh = std::clamp(p, 0.0f, 1.0f); }
    float get_add_connection_thresh() const { return mutation.add_connection_thresh; }
    void set_tick_add_node_thresh(float p) { mutation.tick_add_node_thresh = std::clamp(p, 0.0f, 1.0f); }
    float get_tick_add_node_thresh() const { return mutation.tick_add_node_thresh; }
    void set_tick_add_connection_thresh(float p) { mutation.tick_add_connection_thresh = std::clamp(p, 0.0f, 1.0f); }
    float get_tick_add_connection_thresh() const { return mutation.tick_add_connection_thresh; }
    void set_weight_extremum_init(float v) { mutation.weight_extremum_init = std::max(0.0f, v); }
    float get_weight_extremum_init() const { return mutation.weight_extremum_init; }
    void set_live_mutation_enabled(bool enabled) { mutation.live_mutation_enabled = enabled; }
    bool get_live_mutation_enabled() const { return mutation.live_mutation_enabled; }
    void set_mutate_weight_thresh(float v) { mutation.mutate_weight_thresh = std::clamp(v, 0.0f, 1.0f); }
    float get_mutate_weight_thresh() const { return mutation.mutate_weight_thresh; }
    void set_mutate_weight_full_change_thresh(float v) { mutation.mutate_weight_full_change_thresh = std::clamp(v, 0.0f, 1.0f); }
    float get_mutate_weight_full_change_thresh() const { return mutation.mutate_weight_full_change_thresh; }
    void set_mutate_weight_factor(float v) { mutation.mutate_weight_factor = std::max(0.0f, v); }
    float get_mutate_weight_factor() const { return mutation.mutate_weight_factor; }
    void set_max_iterations_find_connection_thresh(int v) { mutation.max_iterations_find_connection_thresh = std::max(1, v); }
    int get_max_iterations_find_connection_thresh() const { return mutation.max_iterations_find_connection_thresh; }
    void set_reactivate_connection_thresh(float v) { mutation.reactivate_connection_thresh = std::clamp(v, 0.0f, 1.0f); }
    float get_reactivate_connection_thresh() const { return mutation.reactivate_connection_thresh; }
    void set_max_iterations_find_node_thresh(int v) { mutation.max_iterations_find_node_thresh = std::max(1, v); }
    int get_max_iterations_find_node_thresh() const { return mutation.max_iterations_find_node_thresh; }
    void set_mutate_allow_recurrent(bool v) { mutation.mutate_allow_recurrent = v; }
    bool get_mutate_allow_recurrent() const { return mutation.mutate_allow_recurrent; }
    void set_init_add_node_thresh(float p) { mutation.init_add_node_thresh = std::clamp(p, 0.0f, 1.0f); }
    float get_init_add_node_thresh() const { return mutation.init_add_node_thresh; }
    void set_init_add_connection_thresh(float p) { mutation.init_add_connection_thresh = std::clamp(p, 0.0f, 1.0f); }
    float get_init_add_connection_thresh() const { return mutation.init_add_connection_thresh; }
    void set_init_mutation_rounds(int rounds) { mutation.init_mutation_rounds = std::clamp(rounds, 0, 100); }
    int get_init_mutation_rounds() const { return mutation.init_mutation_rounds; }
    void set_mutation_rounds(int rounds) { mutation.mutation_rounds = std::clamp(rounds, 0, 50); }
    int get_mutation_rounds() const { return mutation.mutation_rounds; }

    // Movement
    void set_circle_density(float d);
    float get_circle_density() const { return movement.circle_density; }
    void set_linear_impulse_magnitude(float m);
    float get_linear_impulse_magnitude() const { return movement.linear_impulse_magnitude; }
    void set_angular_impulse_magnitude(float m);
    float get_angular_impulse_magnitude() const { return movement.angular_impulse_magnitude; }
    void set_linear_damping(float d);
    float get_linear_damping() const { return movement.linear_damping; }
    void set_angular_damping(float d);
    float get_angular_damping() const { return movement.angular_damping; }
    void set_boost_particle_impulse_fraction(float f) { movement.boost_particle_impulse_fraction = std::clamp(f, 0.0f, 1.0f); }
    float get_boost_particle_impulse_fraction() const { return movement.boost_particle_impulse_fraction; }
    void set_boost_particle_linear_damping(float d) { movement.boost_particle_linear_damping = std::max(0.0f, d); }
    float get_boost_particle_linear_damping() const { return movement.boost_particle_linear_damping; }

    // Dish & pellets
    void set_petri_radius(float r) { dish.radius = r; }
    float get_petri_radius() const { return dish.radius; }
    void set_minimum_creature_count(int count) { dish.minimum_creature_count = std::max(0, count); }
    int get_minimum_creature_count() const { return dish.minimum_creature_count; }
    void set_auto_remove_outside(bool enabled) { dish.auto_remove_outside = enabled; }
    bool get_auto_remove_outside() const { return dish.auto_remove_outside; }
    void set_sprinkle_rate_eatable(float r) { pellets.sprinkle_rate_eatable = r; }
    void set_sprinkle_rate_toxic(float r) { pellets.sprinkle_rate_toxic = r; }
    void set_sprinkle_rate_division(float r) { pellets.sprinkle_rate_division = r; }
    float get_sprinkle_rate_eatable() const { return pellets.sprinkle_rate_eatable; }
    float get_sprinkle_rate_toxic() const { return pellets.sprinkle_rate_toxic; }
    float get_sprinkle_rate_division() const { return pellets.sprinkle_rate_division; }
    void set_max_food_pellets(int v) { pellets.max_food_pellets = std::max(0, v); }
    int get_max_food_pellets() const { return pellets.max_food_pellets; }
    void set_max_toxic_pellets(int v) { pellets.max_toxic_pellets = std::max(0, v); }
    int get_max_toxic_pellets() const { return pellets.max_toxic_pellets; }
    void set_max_division_pellets(int v) { pellets.max_division_pellets = std::max(0, v); }
    int get_max_division_pellets() const { return pellets.max_division_pellets; }
    void set_food_pellet_density(float d) { pellets.food_density = std::max(0.0f, d); }
    float get_food_pellet_density() const { return pellets.food_density; }
    void set_toxic_pellet_density(float d) { pellets.toxic_density = std::max(0.0f, d); }
    float get_toxic_pellet_density() const { return pellets.toxic_density; }
    void set_division_pellet_density(float d) { pellets.division_density = std::max(0.0f, d); }
    float get_division_pellet_density() const { return pellets.division_density; }
    std::size_t get_food_pellet_count() const;
    std::size_t get_toxic_pellet_count() const;
    std::size_t get_division_pellet_count() const;

    // Death & reproduction
    void set_creature_cloud_area_percentage(float percentage) { death.creature_cloud_area_percentage = percentage; }
    float get_creature_cloud_area_percentage() const { return death.creature_cloud_area_percentage; }
    void set_division_pellet_divide_probability(float p) { death.division_pellet_divide_probability = std::clamp(p, 0.0f, 1.0f); }
    float get_division_pellet_divide_probability() const { return death.division_pellet_divide_probability; }
    void set_inactivity_timeout(float t) { death.inactivity_timeout = std::max(0.0f, t); }
    float get_inactivity_timeout() const { return death.inactivity_timeout; }

    // Selection
    void set_follow_selected(bool v);
    bool get_follow_selected() const;
    void set_selection_mode(SelectionMode mode);
    SelectionMode get_selection_mode() const;
    void update_follow_view(sf::View& view) const;
    void clear_selection();
    const neat::Genome* get_selected_brain() const;
    const CreatureCircle* get_selected_creature() const;
    const CreatureCircle* get_oldest_largest_creature() const;
    const CreatureCircle* get_oldest_smallest_creature() const;
#ifndef NDEBUG
    const CreatureCircle* get_oldest_middle_creature() const;
#endif
    const CreatureCircle* get_follow_target_creature() const;
    void set_selection_to_creature(const CreatureCircle* creature);
    const CreatureCircle* find_nearest_creature(const b2Vec2& pos) const;
    int get_selected_generation() const;
    bool select_circle_at_world(const b2Vec2& pos);
    void mark_age_dirty();
    void mark_selection_dirty();

    // Population & stats
    void add_circle(std::unique_ptr<EatableCircle> circle);
    std::size_t get_creature_count() const;
    void update_max_generation_from_circle(const EatableCircle* circle);
    void recompute_max_generation();
    void remove_random_percentage(float percentage);
    void remove_percentage_pellets(float percentage, bool toxic, bool division_pellet);
    void remove_outside_petri();
    void set_show_true_color(bool value) { show_true_color = value; }
    bool get_show_true_color() const { return show_true_color; }
    void set_selected_creature_possessed(bool possessed) { possesing.possess_selected_creature = possessed; }
    bool is_selected_creature_possessed() const { return possesing.possess_selected_creature; }
    bool get_left_key_down() const { return possesing.left_key_down; }
    bool get_right_key_down() const { return possesing.right_key_down; }
    bool get_up_key_down() const { return possesing.up_key_down; }
    bool get_space_key_down() const { return possesing.space_key_down; }
    std::size_t get_circle_count() const { return circles.size(); }
    float get_longest_life_since_creation() const { return age.max_age_since_creation; }
    float get_longest_life_since_division() const { return age.max_age_since_division; }
    int get_max_generation() const { return generation.max_generation; }
    const neat::Genome* get_max_generation_brain() const { return generation.brain ? &(*generation.brain) : nullptr; }
    std::vector<std::vector<int>>* get_neat_innovations() { return &innovation.innovations; }
    int* get_neat_last_innovation_id() { return &innovation.last_innovation_id; }

private:
    struct SimulationTiming {
        float time_scale = 1.0f;
        float sim_time_accum = 0.0f;
        float real_time_accum = 0.0f;
        float desired_sim_time_accum = 0.0f;
        float last_real_dt = 0.0f;
        float last_sim_dt = 0.0f;
        float actual_sim_speed_inst = 0.0f;
    };
    struct FpsStats {
        float accum_time = 0.0f;
        int frames = 0;
        float last = 0.0f;
    };
    struct BrainSettings {
        float updates_per_second = 10.0f;
        float time_accumulator = 0.0f;
    };
    struct CreatureSettings {
        float minimum_area = 1.0f;
        float add_eatable_area = 0.3f;
        float boost_area = 0.002f;
        float average_area = 5.0f;
    };
    struct CursorState {
        CursorMode mode = CursorMode::Add;
        AddType add_type = AddType::Creature;
    };
    struct DishSettings {
        float radius = 50.0f;
        int minimum_creature_count = 0;
        bool auto_remove_outside = true;
    };
    struct PelletSettings {
        float sprinkle_rate_eatable = 50.0f;
        float sprinkle_rate_toxic = 1.0f;
        float sprinkle_rate_division = 1.0f;
        int max_food_pellets = 5000;
        int max_toxic_pellets = 5000;
        int max_division_pellets = 5000;
        float food_density = 0.1f;
        float toxic_density = 0.008f;
        float division_density = 0.005f;
        float cleanup_rate_food = 0.0f;
        float cleanup_rate_toxic = 0.0f;
        float cleanup_rate_division = 0.0f;
        std::size_t food_count_cached = 0;
        std::size_t toxic_count_cached = 0;
        std::size_t division_count_cached = 0;
    };
    struct MutationSettings {
        float add_node_thresh = 0.005f;
        float add_connection_thresh = 0.1f;
        float tick_add_node_thresh = 0.0f;
        float tick_add_connection_thresh = 0.0f;
        float weight_extremum_init = 0.0f;
        bool live_mutation_enabled = false;
        float mutate_weight_thresh = 0.05f;
        float mutate_weight_full_change_thresh = 0.0f;
        float mutate_weight_factor = 0.2f;
        int max_iterations_find_connection_thresh = 20;
        float reactivate_connection_thresh = 0.25f;
        int max_iterations_find_node_thresh = 20;
        bool mutate_allow_recurrent = false;
        float init_add_node_thresh = 0.0f;
        float init_add_connection_thresh = 0.0f;
        int init_mutation_rounds = 0;
        int mutation_rounds = 1;
    };
    struct MovementSettings {
        float circle_density = 1.0f;
        float linear_impulse_magnitude = 0.5f;
        float angular_impulse_magnitude = 0.5f;
        float linear_damping = 1.0f;
        float angular_damping = 1.0f;
        float boost_particle_impulse_fraction = 0.003f;
        float boost_particle_linear_damping = 5.0f;
    };
    struct DeathSettings {
        float poison_death_probability = 1.0f;
        float poison_death_probability_normal = 0.0f;
        float creature_cloud_area_percentage = 70.0f;
        float division_pellet_divide_probability = 1.0f;
        float inactivity_timeout = 0.1f;
    };
    struct ViewDragState {
        bool dragging = false;
        bool right_dragging = false;
        sf::Vector2i last_drag_pixels{};
    };
    struct GenerationStats {
        int max_generation = 0;
        std::optional<neat::Genome> brain;
    };
    struct InnovationState {
        std::vector<std::vector<int>> innovations;
        int last_innovation_id = 0;
    };
    struct AgeStats {
        float max_age_since_creation = 0.0f;
        float max_age_since_division = 0.0f;
        float min_creation_time = 0.0f;
        float min_division_time = 0.0f;
        bool has_creature = false;
        bool dirty = true;
    };
    struct PossesingSelectedCreature {
        bool possess_selected_creature = false;
        bool left_key_down = false;
        bool right_key_down = false;
        bool up_key_down = false;
        bool space_key_down = false;
    };

    struct RemovalResult {
        bool should_remove = false;
        const CreatureCircle* killer = nullptr;
    };
    struct CullState {
        std::vector<char> remove_mask;
        bool removed_any = false;
        bool removed_creature = false;
        bool selected_was_removed = false;
        const CreatureCircle* selected_killer = nullptr;
    };
    struct SpawnRates {
        float sprinkle = 0.0f;
        float cleanup = 0.0f;
    };

    b2WorldId worldId;
    std::vector<std::unique_ptr<EatableCircle>> circles;
    SimulationTiming timing;
    FpsStats fps;
    BrainSettings brain;
    CreatureSettings creature;
    CursorState cursor;
    SelectionMode selection_mode = SelectionMode::Manual;
    bool selection_dirty = true;
    DishSettings dish;
    PelletSettings pellets;
    MutationSettings mutation;
    MovementSettings movement;
    DeathSettings death;
    GenerationStats generation;
    InnovationState innovation;
    AgeStats age;
    ViewDragState view_drag;
    SelectionManager selection;
    Spawner spawner;
    ContactGraph contact_graph;
    CircleRegistry circle_registry;
    PossesingSelectedCreature possesing;
    bool show_true_color = false;
    bool paused = false;
    std::unique_ptr<GameInputHandler> input_handler;
    std::unique_ptr<GameSelectionController> selection_controller;
    std::unique_ptr<GamePopulationManager> population;
    std::unique_ptr<GameSimulationController> simulation;
};
