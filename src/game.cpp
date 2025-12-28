#include <algorithm>
#include <cmath>
#include <limits>

#include "game.hpp"
#include "creature_circle.hpp"

namespace {
CirclePhysics* circle_from_shape(const b2ShapeId& shapeId) {
    return static_cast<CirclePhysics*>(b2Shape_GetUserData(shapeId));
}

void handle_sensor_begin_touch(const b2SensorBeginTouchEvent& beginTouch) {
    if (!b2Shape_IsValid(beginTouch.sensorShapeId) || !b2Shape_IsValid(beginTouch.visitorShapeId)) {
        return;
    }
    if (auto* sensor = circle_from_shape(beginTouch.sensorShapeId)) {
        if (auto* visitor = circle_from_shape(beginTouch.visitorShapeId)) {
            if (sensor != visitor) {
                sensor->add_touching_circle(visitor);
                visitor->add_touching_circle(sensor);
            }
        }
    }
}

void handle_sensor_end_touch(const b2SensorEndTouchEvent& endTouch) {
    if (!b2Shape_IsValid(endTouch.sensorShapeId) || !b2Shape_IsValid(endTouch.visitorShapeId)) {
        return;
    }
    if (auto* sensor = circle_from_shape(endTouch.sensorShapeId)) {
        if (auto* visitor = circle_from_shape(endTouch.visitorShapeId)) {
            if (sensor != visitor) {
                sensor->remove_touching_circle(visitor);
                visitor->remove_touching_circle(sensor);
            }
        }
    }
}

void process_touch_events(const b2WorldId& worldId) {
    b2SensorEvents sensorEvents = b2World_GetSensorEvents(worldId);
    for (int i = 0; i < sensorEvents.beginCount; ++i)
    {
        b2SensorBeginTouchEvent* beginTouch = sensorEvents.beginEvents + i;
        handle_sensor_begin_touch(*beginTouch);
    }

    for (int i = 0; i < sensorEvents.endCount; ++i)
    {
        b2SensorEndTouchEvent* endTouch = sensorEvents.endEvents + i;
        handle_sensor_end_touch(*endTouch);
    }
}
} // namespace

Game::Game()
    : selection(circles, timing.sim_time_accum),
      spawner(*this) {
    b2WorldDef worldDef = b2DefaultWorldDef();
    worldDef.gravity = b2Vec2{0.0f, 0.0f};
    worldId = b2CreateWorld(&worldDef);
    age.dirty = true;
}

Game::~Game() {
    circles.clear();
    b2DestroyWorld(worldId);
}

void Game::process_game_logic_with_speed() {
    if (paused) {
        timing.last_sim_dt = 0.0f;
        update_actual_sim_speed();
        return;
    }

    float timeStep = (1.0f / 60.0f);

    timing.desired_sim_time_accum += timeStep * timing.time_scale;

    sf::Clock clock; // starts the clock
    float begin_sim_time = timing.sim_time_accum;

    while (timing.sim_time_accum + timeStep < timing.desired_sim_time_accum) {
        process_game_logic();

        if (clock.getElapsedTime() > sf::seconds(timeStep)) {
            timing.desired_sim_time_accum -= timeStep * timing.time_scale;
            timing.desired_sim_time_accum += timing.sim_time_accum - begin_sim_time;

            break;
        }
    }

    // Record how much sim time actually advanced this frame.
    timing.last_sim_dt = timing.sim_time_accum - begin_sim_time;
    update_actual_sim_speed();
}

void Game::process_game_logic() {
    float timeStep = (1.0f / 60.0f);
    int subStepCount = 4;
    b2World_Step(worldId, timeStep, subStepCount);
    timing.sim_time_accum += timeStep;

    process_touch_events(worldId);

    brain.time_accumulator += timeStep;
    spawner.sprinkle_entities(timeStep);
    update_creatures(worldId, timeStep);
    run_brain_updates(worldId, timeStep);
    cleanup_pellets_by_rate(timeStep);
    finalize_world_state();
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

void Game::update_creatures(const b2WorldId& worldId, float dt) {
    for (size_t i = 0; i < circles.size(); ++i) {
        if (circles[i] && circles[i]->get_kind() == CircleKind::Creature) {
            auto* creature_circle = static_cast<CreatureCircle*>(circles[i].get());
            creature_circle->process_eating(worldId, *this, death.poison_death_probability, death.poison_death_probability_normal);
            creature_circle->update_inactivity(dt, death.inactivity_timeout);
        }
    }
}

void Game::run_brain_updates(const b2WorldId& worldId, float timeStep) {
    (void)timeStep;
    const float brain_period = (brain.updates_per_second > 0.0f) ? (1.0f / brain.updates_per_second) : std::numeric_limits<float>::max();
    while (brain.time_accumulator >= brain_period) {
        for (size_t i = 0; i < circles.size(); ++i) {
            if (circles[i] && circles[i]->get_kind() == CircleKind::Creature) {
                auto* creature_circle = static_cast<CreatureCircle*>(circles[i].get());
                creature_circle->set_minimum_area(creature.minimum_area);
                creature_circle->set_display_mode(!show_true_color);
                creature_circle->move_intelligently(worldId, *this, brain_period);
            }
        }
        brain.time_accumulator -= brain_period;
    }
}

void Game::finalize_world_state() {
    cull_consumed();
    remove_stopped_boost_particles();
    if (dish.auto_remove_outside) {
        remove_outside_petri();
    }
    update_max_ages();
    apply_selection_mode();
}

void Game::accumulate_real_time(float dt) {
    if (dt <= 0.0f) return;
    timing.last_real_dt = dt;
    timing.real_time_accum += dt;
    fps.accum_time += dt;
    ++fps.frames;
    if (fps.accum_time >= 0.5f) {
        fps.last = static_cast<float>(fps.frames) / fps.accum_time;
        fps.accum_time = 0.0f;
        fps.frames = 0;
    }
}

void Game::frame_rendered() {
    // reserved for any per-frame hooks; fps handled in accumulate_real_time
}

void Game::update_actual_sim_speed() {
    constexpr float eps = std::numeric_limits<float>::epsilon();
    if (timing.last_real_dt > eps) {
        timing.actual_sim_speed_inst = timing.last_sim_dt / timing.last_real_dt;
    } else {
        timing.actual_sim_speed_inst = 0.0f;
    }
}

void Game::set_circle_density(float d) {
    float clamped = std::max(d, 0.0f);
    if (std::abs(clamped - movement.circle_density) < 1e-6f) {
        return;
    }
    movement.circle_density = clamped;
    for (auto& circle : circles) {
        circle->set_density(movement.circle_density, worldId);
    }
}

void Game::set_linear_impulse_magnitude(float m) {
    float clamped = std::max(m, 0.0f);
    if (std::abs(clamped - movement.linear_impulse_magnitude) < 1e-6f) {
        return;
    }
    movement.linear_impulse_magnitude = clamped;
    apply_impulse_magnitudes_to_circles();
}

void Game::set_angular_impulse_magnitude(float m) {
    float clamped = std::max(m, 0.0f);
    if (std::abs(clamped - movement.angular_impulse_magnitude) < 1e-6f) {
        return;
    }
    movement.angular_impulse_magnitude = clamped;
    apply_impulse_magnitudes_to_circles();
}

void Game::apply_impulse_magnitudes_to_circles() {
    for (auto& circle : circles) {
        circle->set_impulse_magnitudes(movement.linear_impulse_magnitude, movement.angular_impulse_magnitude);
    }
}

void Game::set_linear_damping(float d) {
    float clamped = std::max(d, 0.0f);
    if (std::abs(clamped - movement.linear_damping) < 1e-6f) {
        return;
    }
    movement.linear_damping = clamped;
    apply_damping_to_circles();
}

void Game::set_angular_damping(float d) {
    float clamped = std::max(d, 0.0f);
    if (std::abs(clamped - movement.angular_damping) < 1e-6f) {
        return;
    }
    movement.angular_damping = clamped;
    apply_damping_to_circles();
}

void Game::apply_damping_to_circles() {
    for (auto& circle : circles) {
        circle->set_linear_damping(movement.linear_damping, worldId);
        circle->set_angular_damping(movement.angular_damping, worldId);
    }
}
