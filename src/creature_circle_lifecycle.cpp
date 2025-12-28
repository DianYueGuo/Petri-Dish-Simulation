#include "creature_circle.hpp"
#include "game.hpp"

#include <algorithm>
#include <cmath>
#include <cstdlib>

namespace {
constexpr float PI = 3.14159f;

float calculate_overlap_area(float r1, float r2, float distance) {
    if (distance >= r1 + r2) return 0.0f;
    if (distance <= fabs(r1 - r2)) return PI * fmin(r1, r2) * fmin(r1, r2);

    float r_sq1 = r1 * r1;
    float r_sq2 = r2 * r2;
    float d_sq = distance * distance;

    float clamp1 = std::clamp((d_sq + r_sq1 - r_sq2) / (2.0f * distance * r1), -1.0f, 1.0f);
    float clamp2 = std::clamp((d_sq + r_sq2 - r_sq1) / (2.0f * distance * r2), -1.0f, 1.0f);

    float part1 = r_sq1 * acos(clamp1);
    float part2 = r_sq2 * acos(clamp2);
    float part3 = 0.5f * sqrt((r1 + r2 - distance) * (r1 - r2 + distance) * (-r1 + r2 + distance) * (r1 + r2 + distance));

    return part1 + part2 - part3;
}
} // namespace

void CreatureCircle::process_eating(const b2WorldId &worldId, CreatureContext& ctx, float poison_death_probability_toxic, float poison_death_probability_normal) {
    poisoned = false;
    Game& game = ctx.cc_owner_game();
    if (contacts.graph && contacts.registry) {
        auto& graph = *contacts.graph;
        auto& registry = *contacts.registry;
        graph.for_each_neighbor(get_id(), [&](CircleId neighbor) {
            auto* edible = registry.get_edible(neighbor);
            auto* touching_circle = registry.get_physics(neighbor);
            if (!edible || !touching_circle) {
                return;
            }
            if (!can_eat_circle(*touching_circle)) {
                return;
            }
            if (edible->edible_is_eaten()) {
                return;
            }
            if (!has_overlap_to_eat(*touching_circle)) {
                return;
            }
            float touching_area = edible->edible_area();
            auto* eatable_circle = dynamic_cast<EatableCircle*>(touching_circle);
            if (!eatable_circle) {
                return;
            }
            consume_touching_circle(worldId, game, *eatable_circle, touching_area, poison_death_probability_toxic, poison_death_probability_normal);
        });
    }

    if (poisoned) {
        this->be_eaten();
    }
}

bool CreatureCircle::can_eat_circle(const CirclePhysics& circle) const {
    return circle.getRadius() < this->getRadius();
}

bool CreatureCircle::has_overlap_to_eat(const CirclePhysics& circle) const {
    const float touching_area = circle.getArea();
    const float overlap_threshold = touching_area * 0.8f;

    const float r_self = this->getRadius();
    const float r_other = circle.getRadius();

    const b2Vec2 self_pos = this->getPosition();
    const b2Vec2 other_pos = circle.getPosition();
    const float dx = self_pos.x - other_pos.x;
    const float dy = self_pos.y - other_pos.y;
    const float dist2 = dx * dx + dy * dy;

    const float sum_r = r_self + r_other;
    const float sum_r2 = sum_r * sum_r;
    if (dist2 >= sum_r2) {
        // No overlap possible.
        return false;
    }

    const float diff_r = r_self - r_other;
    const float diff_r2 = diff_r * diff_r;
    if (dist2 <= diff_r2) {
        // Smaller circle fully contained; overlap is entire touching area.
        return touching_area >= overlap_threshold;
    }

    const float distance = std::sqrt(dist2);
    float overlap_area = calculate_overlap_area(r_self, r_other, distance);

    return overlap_area >= overlap_threshold;
}

void CreatureCircle::consume_touching_circle(const b2WorldId &worldId, Game& game, EatableCircle& eatable, float touching_area, float poison_death_probability_toxic, float poison_death_probability_normal) {
    float roll = static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX);
    if (eatable.is_toxic()) {
        if (roll < poison_death_probability_toxic) {
            poisoned = true;
        }
        eatable.be_eaten();
        eatable.set_eaten_by(this);
    } else {
        if (roll < poison_death_probability_normal) {
            poisoned = true;
        }
        eatable.be_eaten();
        eatable.set_eaten_by(this);
        if (eatable.is_division_pellet()) {
            float div_roll = static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX);
            if (div_roll <= game.get_division_pellet_divide_probability()) {
                this->divide(worldId, game);
            }
        }
    }

    this->grow_by_area(touching_area, worldId);
}

void CreatureCircle::divide(const b2WorldId &worldId, Game& game) {
    const float current_area = this->getArea();
    const float divided_area = current_area / 2.0f;

    if (!has_sufficient_area_for_division(divided_area)) {
        return;
    }

    const float new_radius = std::sqrt(divided_area / PI);
    neat::Genome parent_brain_copy = brain;

    const b2Vec2 original_pos = this->getPosition();
    const float angle = this->getAngle();
    const auto [parent_position, child_position] = calculate_division_positions(original_pos, angle, new_radius);

    this->setRadius(new_radius, worldId);
    this->setPosition(parent_position, worldId);

    const int next_generation = this->get_generation() + 1;
    auto new_circle = create_division_child(
        worldId,
        game,
        new_radius,
        angle,
        next_generation,
        child_position,
        parent_brain_copy);
    CreatureCircle* new_circle_ptr = new_circle.get();

    apply_post_division_updates(game, new_circle_ptr, next_generation);
    game.add_circle(std::move(new_circle));
}

bool CreatureCircle::has_sufficient_area_for_division(float divided_area) const {
    return divided_area > minimum_area;
}

std::pair<b2Vec2, b2Vec2> CreatureCircle::calculate_division_positions(const b2Vec2& original_pos, float angle, float new_radius) const {
    b2Vec2 direction = {std::cos(angle), std::sin(angle)};
    b2Vec2 forward_offset = {direction.x * new_radius, direction.y * new_radius};

    b2Vec2 parent_position = {original_pos.x + forward_offset.x, original_pos.y + forward_offset.y};
    b2Vec2 child_position = {original_pos.x - forward_offset.x, original_pos.y - forward_offset.y};
    return {parent_position, child_position};
}

std::unique_ptr<CreatureCircle> CreatureCircle::create_division_child(const b2WorldId& worldId,
                                                                      Game& game,
                                                                      float new_radius,
                                                                      float angle,
                                                                      int next_generation,
                                                                      const b2Vec2& child_position,
                                                                      const neat::Genome& parent_brain_copy) {
    auto new_circle = std::make_unique<CreatureCircle>(
        worldId,
        child_position.x,
        child_position.y,
        new_radius,
        division.circle_density,
        angle + PI,
        next_generation,
        division.init_mutation_rounds,
        division.init_add_node_thresh,
        division.init_add_connection_thresh,
        &brain,
        game.get_neat_innovations(),
        game.get_neat_last_innovation_id(),
        &game);

    if (new_circle) {
        configure_child_after_division(*new_circle, worldId, game, angle, parent_brain_copy);
    }

    return new_circle;
}

void CreatureCircle::apply_post_division_updates(Game& game, CreatureCircle* child, int next_generation) {
    this->set_generation(next_generation);
    if (child) {
        child->set_generation(next_generation);
    }

    owner_game = &game;
    set_last_division_time(division.sim_time);
    if (owner_game) {
        owner_game->mark_age_dirty();
    }

    game.update_max_generation_from_circle(this);
    game.update_max_generation_from_circle(child);

    this->apply_forward_impulse();

    mutate_lineage(game, child);

    update_color_from_brain();
}

void CreatureCircle::configure_child_after_division(CreatureCircle& child, const b2WorldId& worldId, const Game&, float angle, const neat::Genome& parent_brain_copy) const {
    child.brain = parent_brain_copy;
    child.set_impulse_magnitudes(division.linear_impulse_magnitude, division.angular_impulse_magnitude);
    child.set_linear_damping(division.linear_damping, worldId);
    child.set_angular_damping(division.angular_damping, worldId);
    child.setAngle(angle + PI, worldId);
    child.apply_forward_impulse();
    child.update_color_from_brain();
    // Keep the original creation age so lineage age persists across divisions.
    child.set_creation_time(get_creation_time());
    child.set_last_division_time(division.sim_time);
}

void CreatureCircle::mutate_lineage(const Game&, CreatureCircle* child) {
    const int mutation_rounds = std::max(0, division.mutation_rounds);
    float weight_thresh = division.mutate_weight_thresh;
    float weight_full = division.mutate_weight_full_change_thresh;
    float weight_factor = division.mutate_weight_factor;
    float reactivate = division.reactivate_connection_thresh;
    int add_conn_iters = division.max_iterations_find_connection;
    int add_node_iters = division.max_iterations_find_node;
    for (int i = 0; i < mutation_rounds; ++i) {
        if (neat_innovations && neat_last_innov_id) {
            brain.mutate(
                neat_innovations,
                neat_last_innov_id,
                weight_thresh,
                weight_full,
                weight_factor,
                division.add_connection_thresh,
                add_conn_iters,
                reactivate,
                division.add_node_thresh,
                add_node_iters);
        }
        if (child && child->neat_innovations && child->neat_last_innov_id) {
            child->brain.mutate(
                child->neat_innovations,
                child->neat_last_innov_id,
                weight_thresh,
                weight_full,
                weight_factor,
                division.add_connection_thresh,
                add_conn_iters,
                reactivate,
                division.add_node_thresh,
                add_node_iters);
        }
    }
}
