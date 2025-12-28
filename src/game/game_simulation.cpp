#include <algorithm>
#include <cmath>
#include <limits>

#include "game/game_components.hpp"

#include "creature_circle.hpp"
#include "game.hpp"

namespace {
CirclePhysics* circle_from_shape(const b2ShapeId& shapeId) {
    return static_cast<CirclePhysics*>(b2Shape_GetUserData(shapeId));
}

void handle_sensor_begin_touch(const b2SensorBeginTouchEvent& beginTouch, Game& game) {
    if (!b2Shape_IsValid(beginTouch.sensorShapeId) || !b2Shape_IsValid(beginTouch.visitorShapeId)) {
        return;
    }
    if (auto* sensor = circle_from_shape(beginTouch.sensorShapeId)) {
        if (auto* visitor = circle_from_shape(beginTouch.visitorShapeId)) {
            if (sensor != visitor) {
                sensor->add_touching_circle(visitor);
                visitor->add_touching_circle(sensor);
                game.get_contact_graph().add_contact(sensor->get_id(), visitor->get_id());
            }
        }
    }
}

void handle_sensor_end_touch(const b2SensorEndTouchEvent& endTouch, Game& game) {
    if (!b2Shape_IsValid(endTouch.sensorShapeId) || !b2Shape_IsValid(endTouch.visitorShapeId)) {
        return;
    }
    if (auto* sensor = circle_from_shape(endTouch.sensorShapeId)) {
        if (auto* visitor = circle_from_shape(endTouch.visitorShapeId)) {
            if (sensor != visitor) {
                sensor->remove_touching_circle(visitor);
                visitor->remove_touching_circle(sensor);
                game.get_contact_graph().remove_contact(sensor->get_id(), visitor->get_id());
            }
        }
    }
}

void process_touch_events(const b2WorldId& worldId, Game& game) {
    b2SensorEvents sensorEvents = b2World_GetSensorEvents(worldId);
    for (int i = 0; i < sensorEvents.beginCount; ++i)
    {
        b2SensorBeginTouchEvent* beginTouch = sensorEvents.beginEvents + i;
        handle_sensor_begin_touch(*beginTouch, game);
    }

    for (int i = 0; i < sensorEvents.endCount; ++i)
    {
        b2SensorEndTouchEvent* endTouch = sensorEvents.endEvents + i;
        handle_sensor_end_touch(*endTouch, game);
    }
}
} // namespace

GameSimulationController::GameSimulationController(Game& game) : game(game) {}

void GameSimulationController::process_game_logic_with_speed() {
    if (game.paused) {
        game.timing.last_sim_dt = 0.0f;
        update_actual_sim_speed();
        return;
    }

    float timeStep = (1.0f / 60.0f);

    game.timing.desired_sim_time_accum += timeStep * game.timing.time_scale;

    sf::Clock clock; // starts the clock
    float begin_sim_time = game.timing.sim_time_accum;

    while (game.timing.sim_time_accum + timeStep < game.timing.desired_sim_time_accum) {
        process_game_logic();

        if (clock.getElapsedTime() > sf::seconds(timeStep)) {
            game.timing.desired_sim_time_accum -= timeStep * game.timing.time_scale;
            game.timing.desired_sim_time_accum += game.timing.sim_time_accum - begin_sim_time;

            break;
        }
    }

    // Record how much sim time actually advanced this frame.
    game.timing.last_sim_dt = game.timing.sim_time_accum - begin_sim_time;
    update_actual_sim_speed();
}

void GameSimulationController::process_game_logic() {
    float timeStep = (1.0f / 60.0f);
    int subStepCount = 4;
    b2World_Step(game.worldId, timeStep, subStepCount);
    game.timing.sim_time_accum += timeStep;

    process_touch_events(game.worldId, game);

    game.brain.time_accumulator += timeStep;
    game.spawner.sprinkle_entities(timeStep);
    update_creatures(game.worldId, timeStep);
    run_brain_updates(game.worldId, timeStep);
    game.population->cleanup_pellets_by_rate(timeStep);
    finalize_world_state();
}

void GameSimulationController::update_creatures(const b2WorldId& worldId, float dt) {
    for (size_t i = 0; i < game.circles.size(); ++i) {
        if (game.circles[i] && game.circles[i]->get_kind() == CircleKind::Creature) {
            auto* creature_circle = static_cast<CreatureCircle*>(game.circles[i].get());
            creature_circle->process_eating(worldId, game, game.death.poison_death_probability, game.death.poison_death_probability_normal);
            creature_circle->update_inactivity(dt, game.death.inactivity_timeout);
        }
    }
}

void GameSimulationController::run_brain_updates(const b2WorldId& worldId, float timeStep) {
    (void)timeStep;
    const float brain_period = (game.brain.updates_per_second > 0.0f) ? (1.0f / game.brain.updates_per_second) : std::numeric_limits<float>::max();
    while (game.brain.time_accumulator >= brain_period) {
        for (size_t i = 0; i < game.circles.size(); ++i) {
            if (game.circles[i] && game.circles[i]->get_kind() == CircleKind::Creature) {
                auto* creature_circle = static_cast<CreatureCircle*>(game.circles[i].get());
                creature_circle->set_minimum_area(game.creature.minimum_area);
                creature_circle->set_display_mode(!game.show_true_color);
                creature_circle->move_intelligently(worldId, game, brain_period);
            }
        }
        game.brain.time_accumulator -= brain_period;
    }
}

void GameSimulationController::finalize_world_state() {
    game.population->cull_consumed();
    game.population->remove_stopped_boost_particles();
    if (game.dish.auto_remove_outside) {
        game.population->remove_outside_petri();
    }
    game.selection_controller->update_max_ages();
    game.selection_controller->apply_selection_mode();
}

void GameSimulationController::accumulate_real_time(float dt) {
    if (dt <= 0.0f) return;
    game.timing.last_real_dt = dt;
    game.timing.real_time_accum += dt;
    game.fps.accum_time += dt;
    ++game.fps.frames;
    if (game.fps.accum_time >= 0.5f) {
        game.fps.last = static_cast<float>(game.fps.frames) / game.fps.accum_time;
        game.fps.accum_time = 0.0f;
        game.fps.frames = 0;
    }
}

void GameSimulationController::frame_rendered() {
    // reserved for any per-frame hooks; fps handled in accumulate_real_time
}

void GameSimulationController::update_actual_sim_speed() {
    constexpr float eps = std::numeric_limits<float>::epsilon();
    if (game.timing.last_real_dt > eps) {
        game.timing.actual_sim_speed_inst = game.timing.last_sim_dt / game.timing.last_real_dt;
    } else {
        game.timing.actual_sim_speed_inst = 0.0f;
    }
}

void GameSimulationController::set_circle_density(float d) {
    float clamped = std::max(d, 0.0f);
    if (std::abs(clamped - game.movement.circle_density) < 1e-6f) {
        return;
    }
    game.movement.circle_density = clamped;
    for (auto& circle : game.circles) {
        circle->set_density(game.movement.circle_density, game.worldId);
    }
}

void GameSimulationController::set_linear_impulse_magnitude(float m) {
    float clamped = std::max(m, 0.0f);
    if (std::abs(clamped - game.movement.linear_impulse_magnitude) < 1e-6f) {
        return;
    }
    game.movement.linear_impulse_magnitude = clamped;
    apply_impulse_magnitudes_to_circles();
}

void GameSimulationController::set_angular_impulse_magnitude(float m) {
    float clamped = std::max(m, 0.0f);
    if (std::abs(clamped - game.movement.angular_impulse_magnitude) < 1e-6f) {
        return;
    }
    game.movement.angular_impulse_magnitude = clamped;
    apply_impulse_magnitudes_to_circles();
}

void GameSimulationController::apply_impulse_magnitudes_to_circles() {
    for (auto& circle : game.circles) {
        circle->set_impulse_magnitudes(game.movement.linear_impulse_magnitude, game.movement.angular_impulse_magnitude);
    }
}

void GameSimulationController::set_linear_damping(float d) {
    float clamped = std::max(d, 0.0f);
    if (std::abs(clamped - game.movement.linear_damping) < 1e-6f) {
        return;
    }
    game.movement.linear_damping = clamped;
    apply_damping_to_circles();
}

void GameSimulationController::set_angular_damping(float d) {
    float clamped = std::max(d, 0.0f);
    if (std::abs(clamped - game.movement.angular_damping) < 1e-6f) {
        return;
    }
    game.movement.angular_damping = clamped;
    apply_damping_to_circles();
}

void GameSimulationController::apply_damping_to_circles() {
    for (auto& circle : game.circles) {
        circle->set_linear_damping(game.movement.linear_damping, game.worldId);
        circle->set_angular_damping(game.movement.angular_damping, game.worldId);
    }
}
