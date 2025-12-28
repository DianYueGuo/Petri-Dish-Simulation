#include "game/spawner.hpp"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <limits>
#include <random>

#include "creatures/creature_circle.hpp"
#include "game/spawn_context.hpp"

namespace {
constexpr float PI = 3.14159f;

inline float random_unit() {
    return static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX);
}

inline float radius_from_area(float area) {
    return std::sqrt(std::max(area, 0.0f) / PI);
}

inline std::unique_ptr<EatableCircle> create_eatable_for_add_type(const Spawner& spawner,
                                                                  const b2Vec2& pos,
                                                                  SpawnContext::AddType type) {
    bool toxic = type == SpawnContext::AddType::ToxicPellet;
    bool division_pellet = type == SpawnContext::AddType::DivisionPellet;
    return spawner.create_eatable_at(pos, toxic, division_pellet);
}
} // namespace

Spawner::Spawner(SpawnContext& context_ref) : context(context_ref) {}

bool Spawner::pellet_cap_reached(int add_type_value) const {
    auto type = static_cast<SpawnContext::AddType>(add_type_value);
    switch (type) {
        case SpawnContext::AddType::FoodPellet:
            return context.spawn_get_food_pellet_count() >= static_cast<std::size_t>(context.spawn_get_max_food_pellets());
        case SpawnContext::AddType::ToxicPellet:
            return context.spawn_get_toxic_pellet_count() >= static_cast<std::size_t>(context.spawn_get_max_toxic_pellets());
        case SpawnContext::AddType::DivisionPellet:
            return context.spawn_get_division_pellet_count() >= static_cast<std::size_t>(context.spawn_get_max_division_pellets());
        default:
            return false;
    }
}

void Spawner::spawn_selected_type_at(const sf::Vector2f& worldPos) {
    switch (context.spawn_get_add_type()) {
        case SpawnContext::AddType::Creature:
            if (auto circle = create_creature_at({worldPos.x, worldPos.y})) {
                context.spawn_update_max_generation_from_circle(circle.get());
                context.spawn_add_circle(std::move(circle));
            }
            break;
        case SpawnContext::AddType::FoodPellet:
        case SpawnContext::AddType::ToxicPellet:
        case SpawnContext::AddType::DivisionPellet:
            if (pellet_cap_reached(static_cast<int>(context.spawn_get_add_type()))) {
                break;
            }
            context.spawn_add_circle(create_eatable_for_add_type(*this, {worldPos.x, worldPos.y}, context.spawn_get_add_type()));
            break;
        default:
            break;
    }
}

void Spawner::begin_add_drag_if_applicable(const sf::Vector2f& worldPos) {
    if (context.spawn_get_add_type() == SpawnContext::AddType::Creature) {
        reset_add_drag_state();
        return;
    }
    add_dragging = true;
    last_add_world_pos = worldPos;
    last_drag_world_pos = worldPos;
    add_drag_distance = 0.0f;
}

void Spawner::continue_add_drag(const sf::Vector2f& worldPos) {
    if (!add_dragging || context.spawn_get_cursor_mode() != SpawnContext::CursorMode::Add) {
        return;
    }

    if (!last_drag_world_pos) {
        last_drag_world_pos = worldPos;
    }

    float dx_move = worldPos.x - last_drag_world_pos->x;
    float dy_move = worldPos.y - last_drag_world_pos->y;
    add_drag_distance += std::sqrt(dx_move * dx_move + dy_move * dy_move);
    last_drag_world_pos = worldPos;

    const float min_spacing = radius_from_area(context.spawn_get_add_eatable_area()) * 2.0f;

    if (add_drag_distance >= min_spacing) {
        switch (context.spawn_get_add_type()) {
            case SpawnContext::AddType::Creature:
                break;
            case SpawnContext::AddType::FoodPellet:
            case SpawnContext::AddType::ToxicPellet:
            case SpawnContext::AddType::DivisionPellet:
                if (!pellet_cap_reached(static_cast<int>(context.spawn_get_add_type()))) {
                    context.spawn_add_circle(create_eatable_for_add_type(*this, {worldPos.x, worldPos.y}, context.spawn_get_add_type()));
                }
                last_add_world_pos = worldPos;
                break;
        }
        add_drag_distance = 0.0f;
    }
}

void Spawner::reset_add_drag_state() {
    add_dragging = false;
    last_add_world_pos.reset();
    last_drag_world_pos.reset();
    add_drag_distance = 0.0f;
}

void Spawner::sprinkle_entities(float dt) {
    ensure_minimum_creatures();
    sprinkle_with_rate(context.spawn_get_sprinkle_rate_eatable(), static_cast<int>(SpawnContext::AddType::FoodPellet), dt);
    sprinkle_with_rate(context.spawn_get_sprinkle_rate_toxic(), static_cast<int>(SpawnContext::AddType::ToxicPellet), dt);
    sprinkle_with_rate(context.spawn_get_sprinkle_rate_division(), static_cast<int>(SpawnContext::AddType::DivisionPellet), dt);
}

void Spawner::ensure_minimum_creatures() {
    int creature_count = static_cast<int>(context.spawn_get_creature_count());
    if (creature_count >= context.spawn_get_minimum_creature_count()) {
        return;
    }

    int to_spawn = context.spawn_get_minimum_creature_count() - creature_count;
    for (int i = 0; i < to_spawn; ++i) {
        if (auto creature = create_creature_at(random_point_in_petri())) {
            context.spawn_update_max_generation_from_circle(creature.get());
            context.spawn_add_circle(std::move(creature));
        }
    }
}

b2Vec2 Spawner::random_point_in_petri() const {
    float angle = random_unit() * 2.0f * PI;
    float radius = context.spawn_get_petri_radius() * std::sqrt(random_unit());
    return b2Vec2{radius * std::cos(angle), radius * std::sin(angle)};
}

std::unique_ptr<CreatureCircle> Spawner::create_creature_at(const b2Vec2& pos) {
    float base_area = std::max(context.spawn_get_average_creature_area(), 0.0001f);
    float varied_area = base_area;
    float radius = radius_from_area(varied_area);
    float angle = random_unit() * 2.0f * PI;
    const neat::Genome* base_brain = nullptr;
    auto circle = std::make_unique<CreatureCircle>(
        context.spawn_get_world_id(),
        pos.x,
        pos.y,
        radius,
        context.spawn_get_circle_density(),
        angle,
        0,
        context.spawn_get_init_mutation_rounds(),
        context.spawn_get_init_add_node_thresh(),
        context.spawn_get_init_add_connection_thresh(),
        base_brain,
        context.spawn_get_neat_innovations(),
        context.spawn_get_neat_last_innovation_id());
    circle->set_creation_time(context.spawn_get_sim_time());
    circle->set_last_division_time(context.spawn_get_sim_time());
    circle->set_impulse_magnitudes(context.spawn_get_linear_impulse_magnitude(), context.spawn_get_angular_impulse_magnitude());
    circle->set_linear_damping(context.spawn_get_linear_damping(), context.spawn_get_world_id());
    circle->set_angular_damping(context.spawn_get_angular_damping(), context.spawn_get_world_id());
    return circle;
}

std::unique_ptr<EatableCircle> Spawner::create_eatable_at(const b2Vec2& pos, bool toxic, bool division_pellet) const {
    float radius = radius_from_area(context.spawn_get_add_eatable_area());
    auto circle = std::make_unique<EatableCircle>(context.spawn_get_world_id(), pos.x, pos.y, radius, context.spawn_get_circle_density(), toxic, division_pellet, 0.0f);
    circle->set_impulse_magnitudes(context.spawn_get_linear_impulse_magnitude(), context.spawn_get_angular_impulse_magnitude());
    circle->set_linear_damping(context.spawn_get_linear_damping(), context.spawn_get_world_id());
    circle->set_angular_damping(context.spawn_get_angular_damping(), context.spawn_get_world_id());
    return circle;
}

void Spawner::spawn_eatable_cloud(const CreatureCircle& creature, std::vector<std::unique_ptr<EatableCircle>>& out) {
    float creature_radius = creature.getRadius();
    float total_area = creature.getArea();
    if (context.spawn_get_minimum_area() <= 0.0f || total_area <= 0.0f) {
        return;
    }

    float chunk_area = std::min(context.spawn_get_minimum_area(), total_area);
    float remaining_area = total_area * (std::clamp(context.spawn_get_creature_cloud_area_percentage(), 0.0f, 100.0f) / 100.0f);

    while (remaining_area > 0.0f) {
        float use_area = std::min(chunk_area, remaining_area);
        float piece_radius = radius_from_area(use_area);
        float max_offset = std::max(0.0f, creature_radius - piece_radius);

        float angle = static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX) * 2.0f * PI;
        float dist = max_offset * std::sqrt(random_unit());
        b2Vec2 pos = creature.getPosition();
        b2Vec2 piece_pos = {pos.x + std::cos(angle) * dist, pos.y + std::sin(angle) * dist};

        out.push_back(create_eatable_at(piece_pos, false));

        remaining_area -= use_area;
    }
}

void Spawner::sprinkle_with_rate(float rate, int type, float dt) {
    if (rate <= 0.0f || dt <= 0.0f || context.spawn_get_petri_radius() <= 0.0f) {
        return;
    }

    auto add_type = static_cast<SpawnContext::AddType>(type);

    float expected = rate * dt;
    int guaranteed = static_cast<int>(expected);
    float remainder = expected - static_cast<float>(guaranteed);

    auto spawn_once = [&]() -> bool {
        if (pellet_cap_reached(static_cast<int>(add_type))) {
            return false;
        }
        b2Vec2 pos = random_point_in_petri();
        switch (add_type) {
            case SpawnContext::AddType::Creature:
                if (auto creature = create_creature_at(pos)) {
                    context.spawn_update_max_generation_from_circle(creature.get());
                    context.spawn_add_circle(std::move(creature));
                }
                break;
            case SpawnContext::AddType::FoodPellet:
            case SpawnContext::AddType::ToxicPellet:
            case SpawnContext::AddType::DivisionPellet:
                context.spawn_add_circle(create_eatable_for_add_type(*this, pos, add_type));
                break;
        }
        return true;
    };

    for (int i = 0; i < guaranteed; ++i) {
        if (!spawn_once()) {
            break;
        }
    }

    float roll = static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX);
    if (roll < remainder) {
        (void)spawn_once();
    }
}
