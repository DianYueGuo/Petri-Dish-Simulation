#pragma once

#include <memory>
#include <optional>
#include <vector>

#include <SFML/Graphics.hpp>
#include <box2d/box2d.h>
#include <NEAT/genome.hpp>
#include "game/selection_manager.hpp"
#include "game.hpp"

class EatableCircle;
class CreatureCircle;

class GameInputHandler {
public:
    explicit GameInputHandler(Game& game);
    void process_input_events(sf::RenderWindow& window, const std::optional<sf::Event>& event);

private:
    sf::Vector2f pixel_to_world(sf::RenderWindow& window, const sf::Vector2i& pixel) const;
    void start_view_drag(const sf::Event::MouseButtonPressed& e, bool is_right_button);
    void pan_view(sf::RenderWindow& window, const sf::Event::MouseMoved& e);
    void handle_mouse_press(sf::RenderWindow& window, const sf::Event::MouseButtonPressed& e);
    void handle_mouse_release(const sf::Event::MouseButtonReleased& e);
    void handle_mouse_move(sf::RenderWindow& window, const sf::Event::MouseMoved& e);
    void handle_key_press(sf::RenderWindow& window, const sf::Event::KeyPressed& e);
    void handle_key_release(const sf::Event::KeyReleased& e);

    Game& game;
};

class GameSelectionController {
public:
    explicit GameSelectionController(Game& game);

    void clear_selection();
    bool select_circle_at_world(const b2Vec2& pos);
    const neat::Genome* get_selected_brain() const;
    const CreatureCircle* get_selected_creature() const;
    const CreatureCircle* get_oldest_largest_creature() const;
    const CreatureCircle* get_oldest_smallest_creature() const;
#ifndef NDEBUG
    const CreatureCircle* get_oldest_middle_creature() const;
#endif
    const CreatureCircle* get_follow_target_creature() const;
    int get_selected_generation() const;
    void set_follow_selected(bool v);
    bool get_follow_selected() const;
    void set_selection_mode(Game::SelectionMode mode);
    Game::SelectionMode get_selection_mode() const;
    void update_follow_view(sf::View& view) const;
    void set_selection_to_creature(const CreatureCircle* creature);
    const CreatureCircle* find_nearest_creature(const b2Vec2& pos) const;

    void update_max_generation_from_circle(const EatableCircle* circle);
    void recompute_max_generation();
    void update_max_ages();
    void mark_age_dirty();
    void mark_selection_dirty();
    void refresh_generation_and_age();
    void apply_selection_mode();

private:
    Game::SelectionMode sanitize_selection_mode(Game::SelectionMode mode);

    Game& game;
};

class GamePopulationManager {
public:
    explicit GamePopulationManager(PopulationContext& context);

    void add_circle(std::unique_ptr<EatableCircle> circle);
    std::size_t get_creature_count() const;
    void cull_consumed();
    void erase_indices_descending(std::vector<std::size_t>& indices);
    void remove_outside_petri();
    void remove_random_percentage(float percentage);
    void remove_percentage_pellets(float percentage, bool toxic, bool division_pellet);
    std::size_t get_food_pellet_count() const;
    std::size_t get_toxic_pellet_count() const;
    std::size_t get_division_pellet_count() const;
    void cleanup_pellets_by_rate(float timeStep);
    void remove_stopped_boost_particles();
    void adjust_cleanup_rates();

private:
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

    RemovalResult evaluate_circle_removal(EatableCircle& circle, std::vector<std::unique_ptr<EatableCircle>>& spawned_cloud);
    CullState collect_removal_state(const SelectionManager::Snapshot& selection_snapshot, std::vector<std::unique_ptr<EatableCircle>>& spawned_cloud);
    void compact_circles(const std::vector<char>& remove_mask);
    bool is_circle_outside_dish(const EatableCircle& circle, float dish_radius) const;
    bool handle_outside_removal(const std::unique_ptr<EatableCircle>& circle, const SelectionManager::Snapshot& snapshot, float dish_radius, bool& selected_removed, bool& removed_creature);
    std::size_t compute_target_removal_count(std::size_t available, float percentage) const;
    std::vector<std::size_t> collect_pellet_indices(bool toxic, bool division_pellet) const;
    std::size_t count_pellets(bool toxic, bool division_pellet) const;
    std::size_t get_cached_pellet_count(bool toxic, bool division_pellet) const;
    void adjust_pellet_count(const EatableCircle* circle, int delta);
    float desired_pellet_count(float density_target) const;
    float compute_cleanup_rate(std::size_t count, float desired) const;
    SpawnRates calculate_spawn_rates(bool toxic, bool division_pellet, float density_target) const;

    PopulationContext& ctx;
};

class GameSimulationController {
public:
    explicit GameSimulationController(Game& game);

    void process_game_logic_with_speed();
    void process_game_logic();
    void accumulate_real_time(float dt);
    void frame_rendered();
    void update_actual_sim_speed();
    void set_circle_density(float d);
    void set_linear_impulse_magnitude(float m);
    void set_angular_impulse_magnitude(float m);
    void set_linear_damping(float d);
    void set_angular_damping(float d);
    void apply_impulse_magnitudes_to_circles();
    void apply_damping_to_circles();

private:
    void update_creatures(const b2WorldId& worldId, float dt);
    void run_brain_updates(const b2WorldId& worldId, float timeStep);
    void finalize_world_state();

    Game& game;
};
