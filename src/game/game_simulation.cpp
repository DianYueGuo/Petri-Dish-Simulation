#include <algorithm>
#include <cmath>
#include <limits>

#include "game/game_components.hpp"

#include "creature_circle.hpp"
#include "game/simulation_context.hpp"

namespace {
CirclePhysics* circle_from_shape(const b2ShapeId& shapeId) {
    return static_cast<CirclePhysics*>(b2Shape_GetUserData(shapeId));
}

void handle_sensor_begin_touch(const b2SensorBeginTouchEvent& beginTouch, SimulationContext& ctx) {
    if (!b2Shape_IsValid(beginTouch.sensorShapeId) || !b2Shape_IsValid(beginTouch.visitorShapeId)) {
        return;
    }
    if (auto* sensor = circle_from_shape(beginTouch.sensorShapeId)) {
        if (auto* visitor = circle_from_shape(beginTouch.visitorShapeId)) {
            if (sensor != visitor) {
                sensor->add_touching_circle(visitor);
                visitor->add_touching_circle(sensor);
                ctx.sim_contact_graph().add_contact(sensor->get_id(), visitor->get_id());
            }
        }
    }
}

void handle_sensor_end_touch(const b2SensorEndTouchEvent& endTouch, SimulationContext& ctx) {
    if (!b2Shape_IsValid(endTouch.sensorShapeId) || !b2Shape_IsValid(endTouch.visitorShapeId)) {
        return;
    }
    if (auto* sensor = circle_from_shape(endTouch.sensorShapeId)) {
        if (auto* visitor = circle_from_shape(endTouch.visitorShapeId)) {
            if (sensor != visitor) {
                sensor->remove_touching_circle(visitor);
                visitor->remove_touching_circle(sensor);
                ctx.sim_contact_graph().remove_contact(sensor->get_id(), visitor->get_id());
            }
        }
    }
}

void process_touch_events(const b2WorldId& worldId, SimulationContext& ctx) {
    b2SensorEvents sensorEvents = b2World_GetSensorEvents(worldId);
    for (int i = 0; i < sensorEvents.beginCount; ++i)
    {
        b2SensorBeginTouchEvent* beginTouch = sensorEvents.beginEvents + i;
        handle_sensor_begin_touch(*beginTouch, ctx);
    }

    for (int i = 0; i < sensorEvents.endCount; ++i)
    {
        b2SensorEndTouchEvent* endTouch = sensorEvents.endEvents + i;
        handle_sensor_end_touch(*endTouch, ctx);
}
}
} // namespace

GameSimulationController::GameSimulationController(SimulationContext& context) : ctx(context) {}

void GameSimulationController::process_game_logic_with_speed() {
    if (ctx.sim_is_paused()) {
        ctx.sim_set_last_sim_dt(0.0f);
        update_actual_sim_speed();
        return;
    }

    float timeStep = (1.0f / 60.0f);

    ctx.sim_accumulate_desired_time(timeStep * ctx.sim_time_scale());

    sf::Clock clock; // starts the clock
    float begin_sim_time = ctx.sim_time_accum();

    while (ctx.sim_time_accum() + timeStep < ctx.sim_desired_time()) {
        process_game_logic();

        if (clock.getElapsedTime() > sf::seconds(timeStep)) {
            ctx.sim_set_desired_time(ctx.sim_desired_time() - timeStep * ctx.sim_time_scale());
            ctx.sim_set_desired_time(ctx.sim_desired_time() + ctx.sim_time_accum() - begin_sim_time);

            break;
        }
    }

    // Record how much sim time actually advanced this frame.
    ctx.sim_set_last_sim_dt(ctx.sim_time_accum() - begin_sim_time);
    update_actual_sim_speed();
}

void GameSimulationController::process_game_logic() {
    float timeStep = (1.0f / 60.0f);
    int subStepCount = 4;
    b2World_Step(ctx.sim_world_id(), timeStep, subStepCount);
    ctx.sim_add_time(timeStep);

    process_touch_events(ctx.sim_world_id(), ctx);

    ctx.sim_sprinkle_entities(timeStep);
    update_creatures(ctx.sim_world_id(), timeStep);
    run_brain_updates(ctx.sim_world_id(), timeStep);
    ctx.sim_cleanup_population(timeStep);
    ctx.sim_remove_outside_if_enabled();
    ctx.sim_update_selection_after_step();
}

void GameSimulationController::update_creatures(const b2WorldId&, float dt) {
    for (size_t i = 0; i < ctx.sim_circles().size(); ++i) {
        if (ctx.sim_circles()[i] && ctx.sim_circles()[i]->get_kind() == CircleKind::Creature) {
            auto* creature_circle = static_cast<CreatureCircle*>(ctx.sim_circles()[i].get());
            creature_circle->set_contact_context(ctx.sim_contact_graph(), ctx.sim_circle_registry(), ctx.sim_petri_radius());
            CreatureCircle::BehaviorContext behavior_ctx{};
            behavior_ctx.boost_area = ctx.sim_boost_area();
            behavior_ctx.circle_density = ctx.sim_circle_density();
            behavior_ctx.boost_particle_impulse_fraction = ctx.sim_boost_particle_impulse_fraction();
            behavior_ctx.boost_particle_linear_damping = ctx.sim_boost_particle_linear_damping();
            behavior_ctx.linear_impulse_magnitude = ctx.sim_linear_impulse_magnitude();
            behavior_ctx.angular_impulse_magnitude = ctx.sim_angular_impulse_magnitude();
            behavior_ctx.angular_damping = ctx.sim_angular_damping();
            behavior_ctx.live_mutation_enabled = ctx.sim_live_mutation_enabled();
            behavior_ctx.mutate_weight_thresh = ctx.sim_mutate_weight_thresh();
            behavior_ctx.mutate_weight_full_change_thresh = ctx.sim_mutate_weight_full_change_thresh();
            behavior_ctx.mutate_weight_factor = ctx.sim_mutate_weight_factor();
            behavior_ctx.tick_add_connection_thresh = ctx.sim_tick_add_connection_thresh();
            behavior_ctx.tick_add_node_thresh = ctx.sim_tick_add_node_thresh();
            behavior_ctx.max_iterations_find_connection = ctx.sim_max_iterations_find_connection_thresh();
            behavior_ctx.max_iterations_find_node = ctx.sim_max_iterations_find_node_thresh();
            behavior_ctx.reactivate_connection_thresh = ctx.sim_reactivate_connection_thresh();
            const bool is_selected = (ctx.sim_selected_creature() == creature_circle);
            behavior_ctx.selected_and_possessed = is_selected && ctx.sim_is_selected_possessed();
            behavior_ctx.left_key_down = ctx.sim_left_key_down();
            behavior_ctx.right_key_down = ctx.sim_right_key_down();
            behavior_ctx.space_key_down = ctx.sim_space_key_down();
            behavior_ctx.spawn_circle = [&](std::unique_ptr<EatableCircle> c) { ctx.sim_circles().push_back(std::move(c)); };
            creature_circle->set_behavior_context(behavior_ctx);
            CreatureCircle::DivisionContext division_ctx{};
            division_ctx.circle_density = ctx.sim_circle_density();
            division_ctx.init_mutation_rounds = ctx.sim_init_mutation_rounds();
            division_ctx.init_add_node_thresh = ctx.sim_init_add_node_thresh();
            division_ctx.init_add_connection_thresh = ctx.sim_init_add_connection_thresh();
            division_ctx.linear_impulse_magnitude = ctx.sim_linear_impulse_magnitude();
            division_ctx.angular_impulse_magnitude = ctx.sim_angular_impulse_magnitude();
            division_ctx.linear_damping = ctx.sim_linear_damping();
            division_ctx.angular_damping = ctx.sim_angular_damping();
            division_ctx.mutation_rounds = ctx.sim_mutation_rounds();
            division_ctx.mutate_weight_thresh = ctx.sim_mutate_weight_thresh();
            division_ctx.mutate_weight_full_change_thresh = ctx.sim_mutate_weight_full_change_thresh();
            division_ctx.mutate_weight_factor = ctx.sim_mutate_weight_factor();
            division_ctx.add_connection_thresh = ctx.sim_add_connection_thresh();
            division_ctx.max_iterations_find_connection = ctx.sim_max_iterations_find_connection_thresh();
            division_ctx.reactivate_connection_thresh = ctx.sim_reactivate_connection_thresh();
            division_ctx.add_node_thresh = ctx.sim_add_node_thresh();
            division_ctx.max_iterations_find_node = ctx.sim_max_iterations_find_node_thresh();
            division_ctx.sim_time = ctx.sim_time_accum();
            creature_circle->set_division_context(division_ctx);
            creature_circle->process_eating(ctx.sim_world_id(), ctx.sim_owner_game(), ctx.sim_poison_death_probability(), ctx.sim_poison_death_probability_normal());
            creature_circle->update_inactivity(dt, ctx.sim_inactivity_timeout());
        }
    }
}

void GameSimulationController::run_brain_updates(const b2WorldId& worldId, float timeStep) {
    (void)timeStep;
    const float brain_period = (ctx.sim_brain_updates_per_second() > 0.0f) ? (1.0f / ctx.sim_brain_updates_per_second()) : std::numeric_limits<float>::max();
    while (ctx.sim_brain_time_accum() >= brain_period) {
        for (size_t i = 0; i < ctx.sim_circles().size(); ++i) {
            if (ctx.sim_circles()[i] && ctx.sim_circles()[i]->get_kind() == CircleKind::Creature) {
                auto* creature_circle = static_cast<CreatureCircle*>(ctx.sim_circles()[i].get());
                creature_circle->set_minimum_area(ctx.sim_minimum_area());
                creature_circle->set_display_mode(!ctx.sim_show_true_color());
                creature_circle->set_contact_context(ctx.sim_contact_graph(), ctx.sim_circle_registry(), ctx.sim_petri_radius());
                CreatureCircle::BehaviorContext behavior_ctx{};
                behavior_ctx.boost_area = ctx.sim_boost_area();
                behavior_ctx.circle_density = ctx.sim_circle_density();
                behavior_ctx.boost_particle_impulse_fraction = ctx.sim_boost_particle_impulse_fraction();
                behavior_ctx.boost_particle_linear_damping = ctx.sim_boost_particle_linear_damping();
                behavior_ctx.linear_impulse_magnitude = ctx.sim_linear_impulse_magnitude();
                behavior_ctx.angular_impulse_magnitude = ctx.sim_angular_impulse_magnitude();
                behavior_ctx.angular_damping = ctx.sim_angular_damping();
                behavior_ctx.live_mutation_enabled = ctx.sim_live_mutation_enabled();
                behavior_ctx.mutate_weight_thresh = ctx.sim_mutate_weight_thresh();
                behavior_ctx.mutate_weight_full_change_thresh = ctx.sim_mutate_weight_full_change_thresh();
                behavior_ctx.mutate_weight_factor = ctx.sim_mutate_weight_factor();
                behavior_ctx.tick_add_connection_thresh = ctx.sim_tick_add_connection_thresh();
                behavior_ctx.tick_add_node_thresh = ctx.sim_tick_add_node_thresh();
                behavior_ctx.max_iterations_find_connection = ctx.sim_max_iterations_find_connection_thresh();
                behavior_ctx.max_iterations_find_node = ctx.sim_max_iterations_find_node_thresh();
                behavior_ctx.reactivate_connection_thresh = ctx.sim_reactivate_connection_thresh();
                const bool is_selected = (ctx.sim_selected_creature() == creature_circle);
                behavior_ctx.selected_and_possessed = is_selected && ctx.sim_is_selected_possessed();
                behavior_ctx.left_key_down = ctx.sim_left_key_down();
                behavior_ctx.right_key_down = ctx.sim_right_key_down();
                behavior_ctx.space_key_down = ctx.sim_space_key_down();
                behavior_ctx.spawn_circle = [&](std::unique_ptr<EatableCircle> c) { ctx.sim_circles().push_back(std::move(c)); };
                creature_circle->set_behavior_context(behavior_ctx);
                CreatureCircle::DivisionContext division_ctx{};
                division_ctx.circle_density = ctx.sim_circle_density();
                division_ctx.init_mutation_rounds = ctx.sim_init_mutation_rounds();
                division_ctx.init_add_node_thresh = ctx.sim_init_add_node_thresh();
                division_ctx.init_add_connection_thresh = ctx.sim_init_add_connection_thresh();
                division_ctx.linear_impulse_magnitude = ctx.sim_linear_impulse_magnitude();
                division_ctx.angular_impulse_magnitude = ctx.sim_angular_impulse_magnitude();
                division_ctx.linear_damping = ctx.sim_linear_damping();
                division_ctx.angular_damping = ctx.sim_angular_damping();
                division_ctx.mutation_rounds = ctx.sim_mutation_rounds();
                division_ctx.mutate_weight_thresh = ctx.sim_mutate_weight_thresh();
                division_ctx.mutate_weight_full_change_thresh = ctx.sim_mutate_weight_full_change_thresh();
                division_ctx.mutate_weight_factor = ctx.sim_mutate_weight_factor();
                division_ctx.add_connection_thresh = ctx.sim_add_connection_thresh();
                division_ctx.max_iterations_find_connection = ctx.sim_max_iterations_find_connection_thresh();
                division_ctx.reactivate_connection_thresh = ctx.sim_reactivate_connection_thresh();
                division_ctx.add_node_thresh = ctx.sim_add_node_thresh();
                division_ctx.max_iterations_find_node = ctx.sim_max_iterations_find_node_thresh();
                division_ctx.sim_time = ctx.sim_time_accum();
                creature_circle->set_division_context(division_ctx);
                creature_circle->move_intelligently(worldId, ctx.sim_owner_game(), brain_period);
            }
        }
        ctx.sim_set_brain_time_accum(ctx.sim_brain_time_accum() - brain_period);
    }
}

void GameSimulationController::finalize_world_state() {
    ctx.sim_cleanup_population(0.0f); // cleanup already accumulates; zero timestep to align signature removal
    ctx.sim_remove_outside_if_enabled();
    ctx.sim_update_selection_after_step();
}

void GameSimulationController::accumulate_real_time(float dt) {
    if (dt <= 0.0f) return;
    ctx.sim_set_last_real_dt(dt);
}

void GameSimulationController::frame_rendered() {
    // reserved for any per-frame hooks; fps handled in accumulate_real_time
}

void GameSimulationController::update_actual_sim_speed() {
    constexpr float eps = std::numeric_limits<float>::epsilon();
    if (ctx.sim_last_real_dt() > eps) {
        ctx.sim_set_actual_speed(ctx.sim_last_sim_dt() / ctx.sim_last_real_dt());
    } else {
        ctx.sim_set_actual_speed(0.0f);
    }
}

void GameSimulationController::set_circle_density(float d) {
    float clamped = std::max(d, 0.0f);
    if (std::abs(clamped - ctx.sim_circle_density()) < 1e-6f) {
        return;
    }
    for (auto& circle : ctx.sim_circles()) {
        circle->set_density(clamped, ctx.sim_world_id());
    }
}

void GameSimulationController::set_linear_impulse_magnitude(float m) {
    float clamped = std::max(m, 0.0f);
    ctx.sim_set_linear_impulse_magnitude(clamped);
    apply_impulse_magnitudes_to_circles();
}

void GameSimulationController::set_angular_impulse_magnitude(float m) {
    float clamped = std::max(m, 0.0f);
    ctx.sim_set_angular_impulse_magnitude(clamped);
    apply_impulse_magnitudes_to_circles();
}

void GameSimulationController::apply_impulse_magnitudes_to_circles() {
    for (auto& circle : ctx.sim_circles()) {
        circle->set_impulse_magnitudes(ctx.sim_linear_impulse_magnitude(), ctx.sim_angular_impulse_magnitude());
    }
}

void GameSimulationController::set_linear_damping(float d) {
    float clamped = std::max(d, 0.0f);
    ctx.sim_set_linear_damping(clamped);
    apply_damping_to_circles();
}

void GameSimulationController::set_angular_damping(float d) {
    float clamped = std::max(d, 0.0f);
    ctx.sim_set_angular_damping(clamped);
    apply_damping_to_circles();
}

void GameSimulationController::apply_damping_to_circles() {
    for (auto& circle : ctx.sim_circles()) {
        circle->set_linear_damping(ctx.sim_linear_damping(), ctx.sim_world_id());
        circle->set_angular_damping(ctx.sim_angular_damping(), ctx.sim_world_id());
    }
}
