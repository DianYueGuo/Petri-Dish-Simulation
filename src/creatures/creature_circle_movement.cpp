#include "creatures/creature_circle.hpp"
#include "game/game.hpp"

#include <algorithm>
#include <cmath>
#include <cstdlib>

namespace {
constexpr float PI = 3.14159f;

void spawn_boost_particle(const b2WorldId& worldId,
                          Game& game,
                          const CreatureCircle& parent,
                          float boost_radius,
                          float angle,
                          const b2Vec2& back_position,
                          const CreatureCircle::BehaviorContext& behavior) {
    auto boost_circle = std::make_unique<EatableCircle>(
        worldId,
        back_position.x,
        back_position.y,
        boost_radius,
        behavior.circle_density,
        /*toxic=*/false,
        /*division_pellet=*/false,
        /*angle=*/0.0f,
        /*boost_particle=*/true);
    EatableCircle* boost_circle_ptr = boost_circle.get();
    const auto creature_signal_color = parent.get_color_rgb(); // use true signal, not smoothed display
    boost_circle_ptr->set_color_rgb(creature_signal_color[0], creature_signal_color[1], creature_signal_color[2]);
    boost_circle_ptr->smooth_display_color(1.0f);
    float frac = behavior.boost_particle_impulse_fraction;
    boost_circle_ptr->set_impulse_magnitudes(behavior.linear_impulse_magnitude * frac, behavior.angular_impulse_magnitude * frac);
    boost_circle_ptr->set_linear_damping(behavior.boost_particle_linear_damping, worldId);
    boost_circle_ptr->set_angular_damping(behavior.angular_damping, worldId);
    if (behavior.spawn_circle) {
        behavior.spawn_circle(std::move(boost_circle));
    } else {
        game.add_circle(std::move(boost_circle));
    }
    boost_circle_ptr->setAngle(angle + PI, worldId);
    boost_circle_ptr->apply_forward_impulse();
}

b2Vec2 compute_lateral_boost_position(const CreatureCircle& creature, bool to_right) {
    b2Vec2 pos = creature.getPosition();
    float angle = creature.getAngle();
    b2Vec2 direction = {cos(angle), sin(angle)};
    b2Vec2 right_dir = {direction.y, -direction.x};
    constexpr float lateral_fraction = 0.5f; // exact lateral offset as a fraction of radius
    float lateral_sign = to_right ? 1.0f : -1.0f;
    float lat = std::clamp(lateral_fraction, 0.0f, 1.0f);
    float back = std::sqrt(std::max(0.0f, 1.0f - lat * lat));
    // Offset direction already unit length; scale to rim.
    b2Vec2 offset_dir = {
        -direction.x * back + right_dir.x * lat * lateral_sign,
        -direction.y * back + right_dir.y * lat * lateral_sign
    };
    float scale = creature.getRadius();
    return {
        pos.x + offset_dir.x * scale,
        pos.y + offset_dir.y * scale
    };
}
} // namespace

void CreatureCircle::move_randomly(const b2WorldId &worldId, Game &game) {
    float probability = static_cast<float>(rand()) / RAND_MAX;
    if (probability > 0.9f)
        this->boost_eccentric_forward_right(worldId, game);

    probability = static_cast<float>(rand()) / RAND_MAX;
    if (probability > 0.9f)
        this->boost_eccentric_forward_left(worldId, game);
}

void CreatureCircle::move_intelligently(const b2WorldId &worldId, Game &game, float dt) {
    (void)dt;
    run_brain_cycle_from_touching();

    if (behavior.selected_and_possessed) {
        if (behavior.left_key_down) {
            this->boost_eccentric_forward_left(worldId, game);
        }
        if (behavior.right_key_down) {
            this->boost_eccentric_forward_right(worldId, game);
        }
        if (behavior.space_key_down) {
            this->divide(worldId, game);
        }
    } else {
        float probability = static_cast<float>(rand()) / RAND_MAX;
        if (brain_outputs[0] >= probability) {
            this->boost_eccentric_forward_left(worldId, game);
        }
        probability = static_cast<float>(rand()) / RAND_MAX;
        if (brain_outputs[1] >= probability) {
            this->boost_eccentric_forward_right(worldId, game);
        }
        probability = static_cast<float>(rand()) / RAND_MAX;
        if (brain_outputs[2] >= probability) {
            this->divide(worldId, game);
        }
    }

    if (behavior.live_mutation_enabled && neat_innovations && neat_last_innov_id) {
        brain.mutate(
            neat_innovations,
            neat_last_innov_id,
            behavior.mutate_weight_thresh,
            behavior.mutate_weight_full_change_thresh,
            behavior.mutate_weight_factor,
            behavior.tick_add_connection_thresh,
            behavior.max_iterations_find_connection,
            behavior.reactivate_connection_thresh,
            behavior.tick_add_node_thresh,
            behavior.max_iterations_find_node);
    }

    // Update memory from dedicated memory outputs (clamped).
    // Memory outputs are distinct: outputs 6-9.
    for (int i = 0; i < MEMORY_SLOTS; ++i) {
        memory_state[i] = std::clamp(brain_outputs[6 + i], 0.0f, 1.0f);
    }

}

void CreatureCircle::update_inactivity(float dt, float timeout) {
    if (dt <= 0.0f) return;
    inactivity_timer += dt;
    b2Vec2 velocity = getLinearVelocity();
    constexpr float vel_epsilon = 1e-3f;
    const bool is_moving = (std::fabs(velocity.x) > vel_epsilon) || (std::fabs(velocity.y) > vel_epsilon);
    if (is_moving) {
        inactivity_timer = 0.0f;
        return;
    }
    if (timeout <= 0.0f) {
        // Inactivity timeout disabled.
        inactivity_timer = 0.0f;
        return;
    }
    if (inactivity_timer >= timeout && !is_eaten()) {
        poisoned = true;
        this->be_eaten();
        inactivity_timer = 0.0f;
    }
}

void CreatureCircle::boost_forward(const b2WorldId &worldId, Game& game) {
    float current_area = this->getArea();
    float boost_cost = std::max(behavior.boost_area, 0.0f);
    float new_area = current_area - boost_cost;

    if (boost_cost <= 0.0f) {
        // No cost, just apply impulse without leaving a circle.
        this->apply_forward_impulse();
        inactivity_timer = 0.0f;
        return;
    }

    if (new_area > minimum_area) {
        this->setArea(new_area, worldId);
        this->apply_forward_impulse();

        float boost_radius = sqrt(boost_cost / PI);
        b2Vec2 pos = this->getPosition();
        float angle = this->getAngle();
        b2Vec2 direction = {cos(angle), sin(angle)};
        b2Vec2 back_position = {
            pos.x - direction.x * (this->getRadius() + boost_radius),
            pos.y - direction.y * (this->getRadius() + boost_radius)
        };

        spawn_boost_particle(worldId, game, *this, boost_radius, angle, back_position, behavior);
    }
}

void CreatureCircle::boost_eccentric_forward_right(const b2WorldId &worldId, Game& game) {
    float current_area = this->getArea();
    float boost_cost = std::max(behavior.boost_area, 0.0f);
    float new_area = current_area - boost_cost;

    float boost_radius = (boost_cost > 0.0f) ? sqrt(boost_cost / PI) : 0.0f;
    if (boost_cost <= 0.0f) {
        b2Vec2 boost_position = compute_lateral_boost_position(*this, /*to_right=*/true);
        this->apply_forward_impulse_at_point(boost_position);
        inactivity_timer = 0.0f;
        return;
    }

    if (new_area > minimum_area) {
        this->setArea(new_area, worldId);
        float angle = this->getAngle();
        b2Vec2 boost_position = compute_lateral_boost_position(*this, /*to_right=*/true);
        this->apply_forward_impulse_at_point(boost_position);

        spawn_boost_particle(worldId, game, *this, boost_radius, angle, boost_position, behavior);
    }
}

void CreatureCircle::boost_eccentric_forward_left(const b2WorldId &worldId, Game& game) {
    float current_area = this->getArea();
    float boost_cost = std::max(behavior.boost_area, 0.0f);
    float new_area = current_area - boost_cost;

    float boost_radius = (boost_cost > 0.0f) ? sqrt(boost_cost / PI) : 0.0f;
    if (boost_cost <= 0.0f) {
        b2Vec2 boost_position = compute_lateral_boost_position(*this, /*to_right=*/false);
        this->apply_forward_impulse_at_point(boost_position);
        inactivity_timer = 0.0f;
        return;
    }

    if (new_area > minimum_area) {
        this->setArea(new_area, worldId);
        float angle = this->getAngle();
        b2Vec2 boost_position = compute_lateral_boost_position(*this, /*to_right=*/false);
        this->apply_forward_impulse_at_point(boost_position);

        spawn_boost_particle(worldId, game, *this, boost_radius, angle, boost_position, behavior);
    }
}
