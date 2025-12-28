#include <algorithm>
#include <cmath>
#include <limits>

#include "game/game_components.hpp"

#include "creatures/creature_circle.hpp"
#include "game/game.hpp"

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
    if (game.is_paused()) {
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
    game.brain.time_accumulator += timeStep;

    process_touch_events(game.worldId, game);

    game.spawner.sprinkle_entities(timeStep);
    update_creatures(game.worldId, timeStep);
    run_brain_updates(game.worldId, timeStep);
    game.sim_cleanup_population(timeStep);
    game.sim_remove_outside_if_enabled();
    game.sim_update_selection_after_step();
}

void GameSimulationController::update_creatures(const b2WorldId&, float dt) {
    for (size_t i = 0; i < game.circles.size(); ++i) {
        if (game.circles[i] && game.circles[i]->get_kind() == CircleKind::Creature) {
            auto* creature_circle = static_cast<CreatureCircle*>(game.circles[i].get());
            creature_circle->set_contact_context(game.contact_graph, game.circle_registry, game.dish.radius);
            CreatureCircle::BehaviorContext behavior_ctx{};
            behavior_ctx.boost_area = game.creature.boost_area;
            behavior_ctx.circle_density = game.movement.circle_density;
            behavior_ctx.boost_particle_impulse_fraction = game.movement.boost_particle_impulse_fraction;
            behavior_ctx.boost_particle_linear_damping = game.movement.boost_particle_linear_damping;
            behavior_ctx.linear_impulse_magnitude = game.movement.linear_impulse_magnitude;
            behavior_ctx.angular_impulse_magnitude = game.movement.angular_impulse_magnitude;
            behavior_ctx.angular_damping = game.movement.angular_damping;
            behavior_ctx.live_mutation_enabled = game.mutation.live_mutation_enabled;
            behavior_ctx.mutate_weight_thresh = game.mutation.mutate_weight_thresh;
            behavior_ctx.mutate_weight_full_change_thresh = game.mutation.mutate_weight_full_change_thresh;
            behavior_ctx.mutate_weight_factor = game.mutation.mutate_weight_factor;
            behavior_ctx.tick_add_connection_thresh = game.mutation.tick_add_connection_thresh;
            behavior_ctx.tick_add_node_thresh = game.mutation.tick_add_node_thresh;
            behavior_ctx.max_iterations_find_connection = game.mutation.max_iterations_find_connection_thresh;
            behavior_ctx.max_iterations_find_node = game.mutation.max_iterations_find_node_thresh;
            behavior_ctx.reactivate_connection_thresh = game.mutation.reactivate_connection_thresh;
            behavior_ctx.selected_and_possessed = game.possesing.possess_selected_creature && (game.selection_controller->get_selected_creature() == creature_circle);
            behavior_ctx.left_key_down = game.possesing.left_key_down;
            behavior_ctx.right_key_down = game.possesing.right_key_down;
            behavior_ctx.space_key_down = game.possesing.space_key_down;
            behavior_ctx.spawn_circle = [&](std::unique_ptr<EatableCircle> c) { game.population_mgr().add_circle(std::move(c)); };
            creature_circle->set_behavior_context(behavior_ctx);
            CreatureCircle::DivisionContext division_ctx{};
            division_ctx.circle_density = game.movement.circle_density;
            division_ctx.init_mutation_rounds = game.mutation.init_mutation_rounds;
            division_ctx.init_add_node_thresh = game.mutation.init_add_node_thresh;
            division_ctx.init_add_connection_thresh = game.mutation.init_add_connection_thresh;
            division_ctx.linear_impulse_magnitude = game.movement.linear_impulse_magnitude;
            division_ctx.angular_impulse_magnitude = game.movement.angular_impulse_magnitude;
            division_ctx.linear_damping = game.movement.linear_damping;
            division_ctx.angular_damping = game.movement.angular_damping;
            division_ctx.mutation_rounds = game.mutation.mutation_rounds;
            division_ctx.mutate_weight_thresh = game.mutation.mutate_weight_thresh;
            division_ctx.mutate_weight_full_change_thresh = game.mutation.mutate_weight_full_change_thresh;
            division_ctx.mutate_weight_factor = game.mutation.mutate_weight_factor;
            division_ctx.add_connection_thresh = game.mutation.add_connection_thresh;
            division_ctx.max_iterations_find_connection = game.mutation.max_iterations_find_connection_thresh;
            division_ctx.reactivate_connection_thresh = game.mutation.reactivate_connection_thresh;
            division_ctx.add_node_thresh = game.mutation.add_node_thresh;
            division_ctx.max_iterations_find_node = game.mutation.max_iterations_find_node_thresh;
            division_ctx.sim_time = game.timing.sim_time_accum;
            creature_circle->set_division_context(division_ctx);
            creature_circle->process_eating(game.worldId, game, game.death.poison_death_probability, game.death.poison_death_probability_normal);
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
                creature_circle->set_contact_context(game.contact_graph, game.circle_registry, game.dish.radius);
                CreatureCircle::BehaviorContext behavior_ctx{};
                behavior_ctx.boost_area = game.creature.boost_area;
                behavior_ctx.circle_density = game.movement.circle_density;
                behavior_ctx.boost_particle_impulse_fraction = game.movement.boost_particle_impulse_fraction;
                behavior_ctx.boost_particle_linear_damping = game.movement.boost_particle_linear_damping;
                behavior_ctx.linear_impulse_magnitude = game.movement.linear_impulse_magnitude;
                behavior_ctx.angular_impulse_magnitude = game.movement.angular_impulse_magnitude;
                behavior_ctx.angular_damping = game.movement.angular_damping;
                behavior_ctx.live_mutation_enabled = game.mutation.live_mutation_enabled;
                behavior_ctx.mutate_weight_thresh = game.mutation.mutate_weight_thresh;
                behavior_ctx.mutate_weight_full_change_thresh = game.mutation.mutate_weight_full_change_thresh;
                behavior_ctx.mutate_weight_factor = game.mutation.mutate_weight_factor;
                behavior_ctx.tick_add_connection_thresh = game.mutation.tick_add_connection_thresh;
                behavior_ctx.tick_add_node_thresh = game.mutation.tick_add_node_thresh;
                behavior_ctx.max_iterations_find_connection = game.mutation.max_iterations_find_connection_thresh;
                behavior_ctx.max_iterations_find_node = game.mutation.max_iterations_find_node_thresh;
                behavior_ctx.reactivate_connection_thresh = game.mutation.reactivate_connection_thresh;
                behavior_ctx.selected_and_possessed = game.possesing.possess_selected_creature && (game.selection_controller->get_selected_creature() == creature_circle);
                behavior_ctx.left_key_down = game.possesing.left_key_down;
                behavior_ctx.right_key_down = game.possesing.right_key_down;
                behavior_ctx.space_key_down = game.possesing.space_key_down;
                behavior_ctx.spawn_circle = [&](std::unique_ptr<EatableCircle> c) { game.population_mgr().add_circle(std::move(c)); };
                creature_circle->set_behavior_context(behavior_ctx);
                CreatureCircle::DivisionContext division_ctx{};
                division_ctx.circle_density = game.movement.circle_density;
                division_ctx.init_mutation_rounds = game.mutation.init_mutation_rounds;
                division_ctx.init_add_node_thresh = game.mutation.init_add_node_thresh;
                division_ctx.init_add_connection_thresh = game.mutation.init_add_connection_thresh;
                division_ctx.linear_impulse_magnitude = game.movement.linear_impulse_magnitude;
                division_ctx.angular_impulse_magnitude = game.movement.angular_impulse_magnitude;
                division_ctx.linear_damping = game.movement.linear_damping;
                division_ctx.angular_damping = game.movement.angular_damping;
                division_ctx.mutation_rounds = game.mutation.mutation_rounds;
                division_ctx.mutate_weight_thresh = game.mutation.mutate_weight_thresh;
                division_ctx.mutate_weight_full_change_thresh = game.mutation.mutate_weight_full_change_thresh;
                division_ctx.mutate_weight_factor = game.mutation.mutate_weight_factor;
                division_ctx.add_connection_thresh = game.mutation.add_connection_thresh;
                division_ctx.max_iterations_find_connection = game.mutation.max_iterations_find_connection_thresh;
                division_ctx.reactivate_connection_thresh = game.mutation.reactivate_connection_thresh;
                division_ctx.add_node_thresh = game.mutation.add_node_thresh;
                division_ctx.max_iterations_find_node = game.mutation.max_iterations_find_node_thresh;
                division_ctx.sim_time = game.timing.sim_time_accum;
                creature_circle->set_division_context(division_ctx);
                creature_circle->move_intelligently(worldId, game, brain_period);
            }
        }
        game.brain.time_accumulator -= brain_period;
    }
}

void GameSimulationController::finalize_world_state() {
    game.sim_cleanup_population(0.0f); // cleanup already accumulates; zero timestep to align signature removal
    game.sim_remove_outside_if_enabled();
    game.sim_update_selection_after_step();
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
        return;
    }
    game.timing.actual_sim_speed_inst = 0.0f;
}

void GameSimulationController::set_circle_density(float d) {
    float clamped = std::max(d, 0.0f);
    if (std::abs(clamped - game.movement.circle_density) < 1e-6f) {
        return;
    }
    for (auto& circle : game.circles) {
        circle->set_density(clamped, game.worldId);
    }
}

void GameSimulationController::set_linear_impulse_magnitude(float m) {
    float clamped = std::max(m, 0.0f);
    game.movement.linear_impulse_magnitude = clamped;
    apply_impulse_magnitudes_to_circles();
}

void GameSimulationController::set_angular_impulse_magnitude(float m) {
    float clamped = std::max(m, 0.0f);
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
    game.movement.linear_damping = clamped;
    apply_damping_to_circles();
}

void GameSimulationController::set_angular_damping(float d) {
    float clamped = std::max(d, 0.0f);
    game.movement.angular_damping = clamped;
    apply_damping_to_circles();
}

void GameSimulationController::apply_damping_to_circles() {
    for (auto& circle : game.circles) {
        circle->set_linear_damping(game.movement.linear_damping, game.worldId);
        circle->set_angular_damping(game.movement.angular_damping, game.worldId);
    }
}
