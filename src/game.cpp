#include <algorithm>
#include <cmath>
#include <limits>

#include "game.hpp"
#include "creature_circle.hpp"
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

void Game::process_game_logic_with_speed() {
    simulation->process_game_logic_with_speed();
}

void Game::process_game_logic() {
    simulation->process_game_logic();
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

void Game::accumulate_real_time(float dt) {
    simulation->accumulate_real_time(dt);
}

void Game::frame_rendered() {
    simulation->frame_rendered();
}

void Game::set_circle_density(float d) {
    simulation->set_circle_density(d);
}

void Game::set_linear_impulse_magnitude(float m) {
    simulation->set_linear_impulse_magnitude(m);
}

void Game::set_angular_impulse_magnitude(float m) {
    simulation->set_angular_impulse_magnitude(m);
}

void Game::set_linear_damping(float d) {
    simulation->set_linear_damping(d);
}

void Game::set_angular_damping(float d) {
    simulation->set_angular_damping(d);
}
