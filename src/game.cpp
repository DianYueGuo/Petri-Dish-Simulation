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
