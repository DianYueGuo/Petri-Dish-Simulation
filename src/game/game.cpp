#include <algorithm>
#include <cmath>
#include <limits>

#include "game/game.hpp"
#include "creatures/creature_circle.hpp"
#include "game/game_components.hpp"

Game::Game()
    : selection(circles, timing.sim_time_accum),
      spawner(*this) {
    b2WorldDef worldDef = b2DefaultWorldDef();
    worldDef.gravity = b2Vec2{0.0f, 0.0f};
    worldId = b2CreateWorld(&worldDef);
    age.dirty = true;

    input_handler = std::make_unique<GameInputHandler>(*this);
    selection_controller = std::make_unique<GameSelectionController>(*this);
    population = std::make_unique<GamePopulationManager>(*this);
    simulation = std::make_unique<GameSimulationController>(*this);
}

Game::~Game() {
    circles.clear();
    b2DestroyWorld(worldId);
}

GameSimulationController& Game::sim() {
    return *simulation;
}

const GameSimulationController& Game::sim() const {
    return *simulation;
}

GameSelectionController& Game::selection_ctrl() {
    return *selection_controller;
}

const GameSelectionController& Game::selection_ctrl() const {
    return *selection_controller;
}

GamePopulationManager& Game::population_mgr() {
    return *population;
}

const GamePopulationManager& Game::population_mgr() const {
    return *population;
}

GameInputHandler& Game::input_ctrl() {
    return *input_handler;
}

const GameInputHandler& Game::input_ctrl() const {
    return *input_handler;
}

void Game::population_adjust_pellet_count(const EatableCircle* circle, int delta) {
    if (!circle) return;
    if (circle->is_boost_particle()) return;
    if (circle->get_kind() == CircleKind::Creature) return;

    auto apply = [&](std::size_t& counter) {
        if (delta > 0) {
            counter += static_cast<std::size_t>(delta);
        } else {
            std::size_t dec = static_cast<std::size_t>(-delta);
            counter = (counter > dec) ? (counter - dec) : 0;
        }
    };

    if (circle->is_division_pellet()) {
        apply(pellets.division_count_cached);
    } else if (circle->is_toxic()) {
        apply(pellets.toxic_count_cached);
    } else {
        apply(pellets.food_count_cached);
    }
}

void Game::population_on_creature_added(const CreatureCircle& creature_circle) {
    if (!age.dirty && creature_circle.get_kind() == CircleKind::Creature) {
        const float creation_time = creature_circle.get_creation_time();
        const float division_time = creature_circle.get_last_division_time();
        if (!age.has_creature) {
            age.has_creature = true;
            age.min_creation_time = creation_time;
            age.min_division_time = division_time;
        } else {
            age.min_creation_time = std::min(age.min_creation_time, creation_time);
            age.min_division_time = std::min(age.min_division_time, division_time);
        }
        age.max_age_since_creation = std::max(0.0f, timing.sim_time_accum - age.min_creation_time);
        age.max_age_since_division = std::max(0.0f, timing.sim_time_accum - age.min_division_time);
    }
}

void Game::population_spawn_cloud(const CreatureCircle& creature, std::vector<std::unique_ptr<EatableCircle>>& out) {
    spawner.spawn_eatable_cloud(creature, out);
}

void Game::sim_cleanup_population(float timeStep) {
    if (population) {
        population->cleanup_pellets_by_rate(timeStep);
        population->cull_consumed();
        population->remove_stopped_boost_particles();
    }
}

void Game::sim_remove_outside_if_enabled() {
    if (dish.auto_remove_outside && population) {
        population->remove_outside_petri();
    }
}

void Game::sim_update_selection_after_step() {
    if (selection_controller) {
        selection_controller->update_max_ages();
        selection_controller->apply_selection_mode();
    }
}

void Game::draw(sf::RenderWindow& window) const {
    // Draw petri dish boundary
    sf::CircleShape boundary(dish.radius);
    boundary.setOrigin({dish.radius, dish.radius});
    boundary.setPosition({0.0f, 0.0f});
    boundary.setOutlineColor(sf::Color::Red);
    boundary.setOutlineThickness(0.2f);
    boundary.setFillColor(sf::Color::Transparent);
    window.draw(boundary);

    for (const auto& circle : circles) {
        circle->draw(window);
    }
}

void Game::update_max_generation_from_circle(const EatableCircle* circle) {
    selection_ctrl().update_max_generation_from_circle(circle);
}

void Game::recompute_max_generation() {
    selection_ctrl().recompute_max_generation();
}

void Game::mark_age_dirty() {
    selection_ctrl().mark_age_dirty();
}

void Game::mark_selection_dirty() {
    selection_ctrl().mark_selection_dirty();
}
