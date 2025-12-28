#include <algorithm>
#include <cmath>
#include <numeric>
#include <random>

#include "game/game_components.hpp"

#include "creature_circle.hpp"
#include "game.hpp"

GamePopulationManager::GamePopulationManager(Game& game) : game(game) {}

namespace {
void cleanup_circle(Game& game, EatableCircle& circle) {
    game.get_contact_graph().remove_circle(circle.get_id());
    game.get_circle_registry().unregister_circle(circle);
}
} // namespace

void GamePopulationManager::add_circle(std::unique_ptr<EatableCircle> circle) {
    game.selection_controller->update_max_generation_from_circle(circle.get());
    adjust_pellet_count(circle.get(), 1);
    if (circle) {
        game.get_circle_registry().register_capabilities(*circle);
    }
    if (!game.age.dirty && circle && circle->get_kind() == CircleKind::Creature) {
        auto* creature_circle = static_cast<CreatureCircle*>(circle.get());
        const float creation_time = creature_circle->get_creation_time();
        const float division_time = creature_circle->get_last_division_time();
        if (!game.age.has_creature) {
            game.age.has_creature = true;
            game.age.min_creation_time = creation_time;
            game.age.min_division_time = division_time;
        } else {
            game.age.min_creation_time = std::min(game.age.min_creation_time, creation_time);
            game.age.min_division_time = std::min(game.age.min_division_time, division_time);
        }
        game.age.max_age_since_creation = std::max(0.0f, game.timing.sim_time_accum - game.age.min_creation_time);
        game.age.max_age_since_division = std::max(0.0f, game.timing.sim_time_accum - game.age.min_division_time);
    }
    if (circle && circle->get_kind() == CircleKind::Creature) {
        game.selection_controller->mark_selection_dirty();
    }
    game.circles.push_back(std::move(circle));
}

std::size_t GamePopulationManager::get_creature_count() const {
    std::size_t count = 0;
    for (const auto& c : game.circles) {
        if (c && c->get_kind() == CircleKind::Creature) {
            ++count;
        }
    }
    return count;
}

GamePopulationManager::RemovalResult GamePopulationManager::evaluate_circle_removal(EatableCircle& circle, std::vector<std::unique_ptr<EatableCircle>>& spawned_cloud) {
    RemovalResult result{};
    if (circle.get_kind() == CircleKind::Creature) {
        const auto* creature_circle = static_cast<const CreatureCircle*>(&circle);
        if (creature_circle->is_poisoned()) {
            game.spawner.spawn_eatable_cloud(*creature_circle, spawned_cloud);
            result.should_remove = true;
            result.killer = creature_circle->get_eaten_by();
        } else if (creature_circle->is_eaten()) {
            result.should_remove = true;
            result.killer = creature_circle->get_eaten_by();
        }
    } else if (circle.is_eaten()) {
        result.should_remove = true;
    }
    return result;
}

GamePopulationManager::CullState GamePopulationManager::collect_removal_state(const SelectionManager::Snapshot& selection_snapshot, std::vector<std::unique_ptr<EatableCircle>>& spawned_cloud) {
    CullState state;
    state.remove_mask.assign(game.circles.size(), 0);

    for (std::size_t i = 0; i < game.circles.size(); ++i) {
        RemovalResult removal = evaluate_circle_removal(*game.circles[i], spawned_cloud);
        if (!removal.should_remove) {
            continue;
        }
        state.removed_any = true;
        if (game.circles[i]->get_kind() == CircleKind::Creature) {
            state.removed_creature = true;
        }
        if (selection_snapshot.circle && selection_snapshot.circle == game.circles[i].get()) {
            state.selected_was_removed = true;
            state.selected_killer = removal.killer;
        }
        adjust_pellet_count(game.circles[i].get(), -1);
        state.remove_mask[i] = 1;
    }

    return state;
}

void GamePopulationManager::compact_circles(const std::vector<char>& remove_mask) {
    if (remove_mask.empty()) {
        return;
    }

    for (std::size_t i = 0; i < game.circles.size(); ++i) {
        if (i < remove_mask.size() && remove_mask[i]) {
            if (game.circles[i]) {
                cleanup_circle(game, *game.circles[i]);
            }
        }
    }

    std::size_t write = 0;
    for (std::size_t read = 0; read < game.circles.size(); ++read) {
        if (!remove_mask[read]) {
            if (write != read) {
                game.circles[write] = std::move(game.circles[read]);
            }
            ++write;
        }
    }
    game.circles.resize(write);
}

void GamePopulationManager::cull_consumed() {
    std::vector<std::unique_ptr<EatableCircle>> spawned_cloud;
    auto selection_snapshot = game.selection.capture_snapshot();

    CullState state = collect_removal_state(selection_snapshot, spawned_cloud);

    if (state.removed_any) {
        compact_circles(state.remove_mask);
    }
    if (state.removed_creature) {
        game.selection_controller->mark_age_dirty();
        game.selection_controller->mark_selection_dirty();
    }

    game.selection.handle_selection_after_removal(selection_snapshot, state.selected_was_removed, state.selected_killer, selection_snapshot.position);
    game.selection_controller->refresh_generation_and_age();

    for (auto& c : spawned_cloud) {
        add_circle(std::move(c));
    }
}

void GamePopulationManager::erase_indices_descending(std::vector<std::size_t>& indices) {
    if (indices.empty()) {
        return;
    }

    auto snapshot = game.selection.capture_snapshot();

    std::sort(indices.begin(), indices.end(), std::greater<std::size_t>());
    indices.erase(std::unique(indices.begin(), indices.end()), indices.end());

    bool removed_creature = false;
    for (std::size_t idx : indices) {
        if (idx < game.circles.size()) {
            if (game.circles[idx]->get_kind() == CircleKind::Creature) {
                removed_creature = true;
            }
            adjust_pellet_count(game.circles[idx].get(), -1);
            cleanup_circle(game, *game.circles[idx]);
            game.circles.erase(game.circles.begin() + static_cast<std::ptrdiff_t>(idx));
        }
    }

    game.selection.revalidate_selection(snapshot.circle);
    if (removed_creature) {
        game.selection_controller->mark_age_dirty();
        game.selection_controller->mark_selection_dirty();
    }
    game.selection_controller->refresh_generation_and_age();
}

bool GamePopulationManager::is_circle_outside_dish(const EatableCircle& circle, float dish_radius) const {
    const double r = static_cast<double>(circle.getRadius());
    const double R = static_cast<double>(dish_radius);

    if (r <= 0.0 || R <= 0.0) {
        return false;
    }

    const b2Vec2 pos = circle.getPosition();
    const double dist_sq = static_cast<double>(pos.x) * static_cast<double>(pos.x) +
                           static_cast<double>(pos.y) * static_cast<double>(pos.y);
    const double d = std::sqrt(dist_sq);

    const double pi = std::acos(-1.0);
    const double circle_area = pi * r * r;

    double overlap_area = 0.0;
    if (d >= R + r) {
        overlap_area = 0.0; // No intersection
    } else if (d <= std::abs(R - r)) {
        const double min_radius = std::min(R, r);
        overlap_area = pi * min_radius * min_radius; // One circle fully inside the other
    } else {
        const double d2 = d * d;
        const double r2 = r * r;
        const double R2 = R * R;
        const double alpha = std::acos((d2 + r2 - R2) / (2.0 * d * r));
        const double beta = std::acos((d2 + R2 - r2) / (2.0 * d * R));
        const double term = (-d + r + R) * (d + r - R) * (d - r + R) * (d + r + R);
        overlap_area = r2 * alpha + R2 * beta - 0.5 * std::sqrt(std::max(0.0, term));
    }

    const double inside_ratio = std::clamp(overlap_area / circle_area, 0.0, 1.0);
    const double outside_ratio = 1.0 - inside_ratio;
    return outside_ratio >= 0.8;
}

bool GamePopulationManager::handle_outside_removal(const std::unique_ptr<EatableCircle>& circle,
                                  const SelectionManager::Snapshot& snapshot,
                                  float dish_radius,
                                  bool& selected_removed,
                                  bool& removed_creature) {
    float radius = circle->getRadius();
    if (!is_circle_outside_dish(*circle, dish_radius)) {
        return false;
    }

    if (snapshot.circle && circle.get() == snapshot.circle) {
        selected_removed = true;
    }
    if (radius < dish_radius && circle->get_kind() == CircleKind::Creature) {
        removed_creature = true;
    }
    adjust_pellet_count(circle.get(), -1);
    return true;
}

void GamePopulationManager::remove_outside_petri() {
    if (game.circles.empty()) {
        return;
    }

    auto snapshot = game.selection.capture_snapshot();

    bool selected_removed = false;
    bool removed_creature = false;
    const float dish_radius = game.dish.radius;
    game.circles.erase(
        std::remove_if(
            game.circles.begin(),
            game.circles.end(),
            [&](const std::unique_ptr<EatableCircle>& circle) {
                bool remove = handle_outside_removal(circle, snapshot, dish_radius, selected_removed, removed_creature);
                if (remove) {
                    cleanup_circle(game, *circle);
                }
                return remove;
            }),
        game.circles.end());

    game.selection.handle_selection_after_removal(snapshot, selected_removed, nullptr, snapshot.position);
    if (removed_creature) {
        game.selection_controller->mark_age_dirty();
        game.selection_controller->mark_selection_dirty();
    }
    game.selection_controller->refresh_generation_and_age();
}

std::size_t GamePopulationManager::compute_target_removal_count(std::size_t available, float percentage) const {
    if (available == 0 || percentage <= 0.0f) {
        return 0;
    }
    float clamped = std::clamp(percentage, 0.0f, 100.0f);
    double ratio = static_cast<double>(clamped) / 100.0;
    return static_cast<std::size_t>(std::round(static_cast<double>(available) * ratio));
}

void GamePopulationManager::remove_random_percentage(float percentage) {
    if (game.circles.empty()) {
        return;
    }

    std::size_t target = compute_target_removal_count(game.circles.size(), percentage);
    if (target == 0) {
        return;
    }

    std::vector<std::size_t> indices(game.circles.size());
    std::iota(indices.begin(), indices.end(), 0);

    static std::mt19937 rng{std::random_device{}()};
    std::shuffle(indices.begin(), indices.end(), rng);

    indices.resize(target);
    erase_indices_descending(indices);
}

std::vector<std::size_t> GamePopulationManager::collect_pellet_indices(bool toxic, bool division_pellet) const {
    std::vector<std::size_t> indices;
    indices.reserve(game.circles.size());
    for (std::size_t i = 0; i < game.circles.size(); ++i) {
        if (auto* e = dynamic_cast<const EatableCircle*>(game.circles[i].get())) {
            if (e->is_boost_particle()) continue;
            if (e->get_kind() == CircleKind::Creature) continue;
            if (e->is_toxic() == toxic && e->is_division_pellet() == division_pellet) {
                indices.push_back(i);
            }
        }
    }
    return indices;
}

void GamePopulationManager::remove_percentage_pellets(float percentage, bool toxic, bool division_pellet) {
    if (game.circles.empty()) {
        return;
    }

    std::vector<std::size_t> indices = collect_pellet_indices(toxic, division_pellet);
    if (indices.empty()) {
        return;
    }

    std::size_t target = compute_target_removal_count(indices.size(), percentage);
    if (target == 0) {
        return;
    }

    static std::mt19937 rng{std::random_device{}()};
    std::shuffle(indices.begin(), indices.end(), rng);
    indices.resize(target);
    erase_indices_descending(indices);
}

std::size_t GamePopulationManager::count_pellets(bool toxic, bool division_pellet) const {
    std::size_t count = 0;
    const float dish_radius = game.dish.radius;
    for (const auto& c : game.circles) {
        if (auto* e = dynamic_cast<const EatableCircle*>(c.get())) {
            if (e->is_boost_particle()) continue;
            if (e->get_kind() == CircleKind::Creature) continue;
            // Only count pellets inside the petri dish.
            b2Vec2 pos = e->getPosition();
            float r = e->getRadius();
            if (r >= dish_radius) continue;
            float max_center_dist = dish_radius - r;
            float dist2 = pos.x * pos.x + pos.y * pos.y;
            if (dist2 > max_center_dist * max_center_dist) continue;
            if (e->is_toxic() == toxic && e->is_division_pellet() == division_pellet) {
                ++count;
            }
        }
    }
    return count;
}

std::size_t GamePopulationManager::get_cached_pellet_count(bool toxic, bool division_pellet) const {
    if (division_pellet) return game.pellets.division_count_cached;
    return toxic ? game.pellets.toxic_count_cached : game.pellets.food_count_cached;
}

void GamePopulationManager::adjust_pellet_count(const EatableCircle* circle, int delta) {
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
        apply(game.pellets.division_count_cached);
    } else if (circle->is_toxic()) {
        apply(game.pellets.toxic_count_cached);
    } else {
        apply(game.pellets.food_count_cached);
    }
}

std::size_t GamePopulationManager::get_food_pellet_count() const {
    return game.pellets.food_count_cached;
}

std::size_t GamePopulationManager::get_toxic_pellet_count() const {
    return game.pellets.toxic_count_cached;
}

std::size_t GamePopulationManager::get_division_pellet_count() const {
    return game.pellets.division_count_cached;
}

float GamePopulationManager::desired_pellet_count(float density_target) const {
    constexpr float PI = 3.14159f;
    float area = PI * game.dish.radius * game.dish.radius;
    float pellet_area = std::max(game.creature.add_eatable_area, 1e-6f);
    float desired_area = std::max(0.0f, density_target) * area;
    return desired_area / pellet_area;
}

float GamePopulationManager::compute_cleanup_rate(std::size_t count, float desired) const {
    if (desired <= 0.0f) {
        return count > 0 ? 100.0f : 0.0f;
    }
    float count_f = static_cast<float>(count);
    if (count_f <= desired) return 0.0f;
    float ratio = (count_f - desired) / desired;
    float rate = ratio * 50.0f; // 50%/s when double the desired
    return std::clamp(rate, 0.0f, 100.0f);
}

GamePopulationManager::SpawnRates GamePopulationManager::calculate_spawn_rates(bool toxic, bool division_pellet, float density_target) const {
    float desired = desired_pellet_count(density_target);
    std::size_t count = get_cached_pellet_count(toxic, division_pellet);
    float diff = desired - static_cast<float>(count);
    float sprinkle_rate = (diff > 0.0f) ? std::min(diff * 0.5f, 200.0f) : 0.0f;
    float cleanup_rate = compute_cleanup_rate(count, desired);
    return {sprinkle_rate, cleanup_rate};
}

void GamePopulationManager::adjust_cleanup_rates() {
    auto food_rates = calculate_spawn_rates(false, false, game.pellets.food_density);
    auto toxic_rates = calculate_spawn_rates(true, false, game.pellets.toxic_density);
    auto division_rates = calculate_spawn_rates(false, true, game.pellets.division_density);
    game.pellets.sprinkle_rate_eatable = food_rates.sprinkle;
    game.pellets.cleanup_rate_food = food_rates.cleanup;
    game.pellets.sprinkle_rate_toxic = toxic_rates.sprinkle;
    game.pellets.cleanup_rate_toxic = toxic_rates.cleanup;
    game.pellets.sprinkle_rate_division = division_rates.sprinkle;
    game.pellets.cleanup_rate_division = division_rates.cleanup;
}

void GamePopulationManager::cleanup_pellets_by_rate(float timeStep) {
    adjust_cleanup_rates();
    // continuous pellet cleanup by rate (percent per second)
    if (game.pellets.cleanup_rate_food > 0.0f) {
        remove_percentage_pellets(game.pellets.cleanup_rate_food * timeStep, false, false);
    }
    if (game.pellets.cleanup_rate_toxic > 0.0f) {
        remove_percentage_pellets(game.pellets.cleanup_rate_toxic * timeStep, true, false);
    }
    if (game.pellets.cleanup_rate_division > 0.0f) {
        remove_percentage_pellets(game.pellets.cleanup_rate_division * timeStep, false, true);
    }
}

void GamePopulationManager::remove_stopped_boost_particles() {
    constexpr float vel_epsilon = 1e-3f;
    auto snapshot = game.selection.capture_snapshot();
    game.circles.erase(
        std::remove_if(
            game.circles.begin(),
            game.circles.end(),
            [&](const std::unique_ptr<EatableCircle>& circle) {
                if (!circle->is_boost_particle()) {
                    return false;
                }
                b2Vec2 v = circle->getLinearVelocity();
                bool should_remove = (std::fabs(v.x) <= vel_epsilon && std::fabs(v.y) <= vel_epsilon);
                if (should_remove) {
                    cleanup_circle(game, *circle);
                }
                return should_remove;
            }),
        game.circles.end());
    game.selection.handle_selection_after_removal(snapshot, false, nullptr, snapshot.position);
    game.selection_controller->refresh_generation_and_age();
}

void Game::add_circle(std::unique_ptr<EatableCircle> circle) {
    population->add_circle(std::move(circle));
}

std::size_t Game::get_creature_count() const {
    return population->get_creature_count();
}

void Game::remove_random_percentage(float percentage) {
    population->remove_random_percentage(percentage);
}

void Game::remove_percentage_pellets(float percentage, bool toxic, bool division_pellet) {
    population->remove_percentage_pellets(percentage, toxic, division_pellet);
}

void Game::remove_outside_petri() {
    population->remove_outside_petri();
}

std::size_t Game::get_food_pellet_count() const {
    return population->get_food_pellet_count();
}

std::size_t Game::get_toxic_pellet_count() const {
    return population->get_toxic_pellet_count();
}

std::size_t Game::get_division_pellet_count() const {
    return population->get_division_pellet_count();
}
