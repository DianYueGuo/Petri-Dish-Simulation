#include <algorithm>
#include <cmath>
#include <numeric>
#include <random>

#include "game.hpp"
#include "creature_circle.hpp"

void Game::add_circle(std::unique_ptr<EatableCircle> circle) {
    update_max_generation_from_circle(circle.get());
    adjust_pellet_count(circle.get(), 1);
    if (!age.dirty && circle && circle->get_kind() == CircleKind::Creature) {
        auto* creature_circle = static_cast<CreatureCircle*>(circle.get());
        const float creation_time = creature_circle->get_creation_time();
        const float division_time = creature_circle->get_last_division_time();
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
    if (circle && circle->get_kind() == CircleKind::Creature) {
        mark_selection_dirty();
    }
    circles.push_back(std::move(circle));
}

std::size_t Game::get_creature_count() const {
    std::size_t count = 0;
    for (const auto& c : circles) {
        if (c && c->get_kind() == CircleKind::Creature) {
            ++count;
        }
    }
    return count;
}

Game::RemovalResult Game::evaluate_circle_removal(EatableCircle& circle, std::vector<std::unique_ptr<EatableCircle>>& spawned_cloud) {
    RemovalResult result{};
    if (circle.get_kind() == CircleKind::Creature) {
        const auto* creature_circle = static_cast<const CreatureCircle*>(&circle);
        if (creature_circle->is_poisoned()) {
            spawner.spawn_eatable_cloud(*creature_circle, spawned_cloud);
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

Game::CullState Game::collect_removal_state(const SelectionManager::Snapshot& selection_snapshot, std::vector<std::unique_ptr<EatableCircle>>& spawned_cloud) {
    CullState state;
    state.remove_mask.assign(circles.size(), 0);

    for (std::size_t i = 0; i < circles.size(); ++i) {
        RemovalResult removal = evaluate_circle_removal(*circles[i], spawned_cloud);
        if (!removal.should_remove) {
            continue;
        }
        state.removed_any = true;
        if (circles[i]->get_kind() == CircleKind::Creature) {
            state.removed_creature = true;
        }
        if (selection_snapshot.circle && selection_snapshot.circle == circles[i].get()) {
            state.selected_was_removed = true;
            state.selected_killer = removal.killer;
        }
        adjust_pellet_count(circles[i].get(), -1);
        state.remove_mask[i] = 1;
    }

    return state;
}

void Game::compact_circles(const std::vector<char>& remove_mask) {
    if (remove_mask.empty()) {
        return;
    }

    std::size_t write = 0;
    for (std::size_t read = 0; read < circles.size(); ++read) {
        if (!remove_mask[read]) {
            if (write != read) {
                circles[write] = std::move(circles[read]);
            }
            ++write;
        }
    }
    circles.resize(write);
}

void Game::cull_consumed() {
    std::vector<std::unique_ptr<EatableCircle>> spawned_cloud;
    auto selection_snapshot = selection.capture_snapshot();

    CullState state = collect_removal_state(selection_snapshot, spawned_cloud);

    if (state.removed_any) {
        compact_circles(state.remove_mask);
    }
    if (state.removed_creature) {
        mark_age_dirty();
        mark_selection_dirty();
    }

    selection.handle_selection_after_removal(selection_snapshot, state.selected_was_removed, state.selected_killer, selection_snapshot.position);
    refresh_generation_and_age();

    for (auto& c : spawned_cloud) {
        add_circle(std::move(c));
    }
}

void Game::erase_indices_descending(std::vector<std::size_t>& indices) {
    if (indices.empty()) {
        return;
    }

    auto snapshot = selection.capture_snapshot();

    std::sort(indices.begin(), indices.end(), std::greater<std::size_t>());
    indices.erase(std::unique(indices.begin(), indices.end()), indices.end());

    bool removed_creature = false;
    for (std::size_t idx : indices) {
        if (idx < circles.size()) {
            if (circles[idx]->get_kind() == CircleKind::Creature) {
                removed_creature = true;
            }
            adjust_pellet_count(circles[idx].get(), -1);
            circles.erase(circles.begin() + static_cast<std::ptrdiff_t>(idx));
        }
    }

    selection.revalidate_selection(snapshot.circle);
    if (removed_creature) {
        mark_age_dirty();
        mark_selection_dirty();
    }
    refresh_generation_and_age();
}

bool Game::is_circle_outside_dish(const EatableCircle& circle, float dish_radius) const {
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

bool Game::handle_outside_removal(const std::unique_ptr<EatableCircle>& circle,
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

void Game::remove_outside_petri() {
    if (circles.empty()) {
        return;
    }

    auto snapshot = selection.capture_snapshot();

    bool selected_removed = false;
    bool removed_creature = false;
    const float dish_radius = dish.radius;
    circles.erase(
        std::remove_if(
            circles.begin(),
            circles.end(),
            [&](const std::unique_ptr<EatableCircle>& circle) {
                return handle_outside_removal(circle, snapshot, dish_radius, selected_removed, removed_creature);
            }),
        circles.end());

    selection.handle_selection_after_removal(snapshot, selected_removed, nullptr, snapshot.position);
    if (removed_creature) {
        mark_age_dirty();
        mark_selection_dirty();
    }
    refresh_generation_and_age();
}

std::size_t Game::compute_target_removal_count(std::size_t available, float percentage) const {
    if (available == 0 || percentage <= 0.0f) {
        return 0;
    }
    float clamped = std::clamp(percentage, 0.0f, 100.0f);
    double ratio = static_cast<double>(clamped) / 100.0;
    return static_cast<std::size_t>(std::round(static_cast<double>(available) * ratio));
}

void Game::remove_random_percentage(float percentage) {
    if (circles.empty()) {
        return;
    }

    std::size_t target = compute_target_removal_count(circles.size(), percentage);
    if (target == 0) {
        return;
    }

    std::vector<std::size_t> indices(circles.size());
    std::iota(indices.begin(), indices.end(), 0);

    static std::mt19937 rng{std::random_device{}()};
    std::shuffle(indices.begin(), indices.end(), rng);

    indices.resize(target);
    erase_indices_descending(indices);
}

std::vector<std::size_t> Game::collect_pellet_indices(bool toxic, bool division_pellet) const {
    std::vector<std::size_t> indices;
    indices.reserve(circles.size());
    for (std::size_t i = 0; i < circles.size(); ++i) {
        if (auto* e = dynamic_cast<const EatableCircle*>(circles[i].get())) {
            if (e->is_boost_particle()) continue;
            if (e->get_kind() == CircleKind::Creature) continue;
            if (e->is_toxic() == toxic && e->is_division_pellet() == division_pellet) {
                indices.push_back(i);
            }
        }
    }
    return indices;
}

void Game::remove_percentage_pellets(float percentage, bool toxic, bool division_pellet) {
    if (circles.empty()) {
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

std::size_t Game::count_pellets(bool toxic, bool division_pellet) const {
    std::size_t count = 0;
    const float dish_radius = dish.radius;
    for (const auto& c : circles) {
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

std::size_t Game::get_cached_pellet_count(bool toxic, bool division_pellet) const {
    if (division_pellet) return pellets.division_count_cached;
    return toxic ? pellets.toxic_count_cached : pellets.food_count_cached;
}

void Game::adjust_pellet_count(const EatableCircle* circle, int delta) {
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

std::size_t Game::get_food_pellet_count() const {
    return pellets.food_count_cached;
}

std::size_t Game::get_toxic_pellet_count() const {
    return pellets.toxic_count_cached;
}

std::size_t Game::get_division_pellet_count() const {
    return pellets.division_count_cached;
}

float Game::desired_pellet_count(float density_target) const {
    constexpr float PI = 3.14159f;
    float area = PI * dish.radius * dish.radius;
    float pellet_area = std::max(creature.add_eatable_area, 1e-6f);
    float desired_area = std::max(0.0f, density_target) * area;
    return desired_area / pellet_area;
}

float Game::compute_cleanup_rate(std::size_t count, float desired) const {
    if (desired <= 0.0f) {
        return count > 0 ? 100.0f : 0.0f;
    }
    float count_f = static_cast<float>(count);
    if (count_f <= desired) return 0.0f;
    float ratio = (count_f - desired) / desired;
    float rate = ratio * 50.0f; // 50%/s when double the desired
    return std::clamp(rate, 0.0f, 100.0f);
}

Game::SpawnRates Game::calculate_spawn_rates(bool toxic, bool division_pellet, float density_target) const {
    float desired = desired_pellet_count(density_target);
    std::size_t count = get_cached_pellet_count(toxic, division_pellet);
    float diff = desired - static_cast<float>(count);
    float sprinkle_rate = (diff > 0.0f) ? std::min(diff * 0.5f, 200.0f) : 0.0f;
    float cleanup_rate = compute_cleanup_rate(count, desired);
    return {sprinkle_rate, cleanup_rate};
}

void Game::adjust_cleanup_rates() {
    auto food_rates = calculate_spawn_rates(false, false, pellets.food_density);
    auto toxic_rates = calculate_spawn_rates(true, false, pellets.toxic_density);
    auto division_rates = calculate_spawn_rates(false, true, pellets.division_density);
    pellets.sprinkle_rate_eatable = food_rates.sprinkle;
    pellets.cleanup_rate_food = food_rates.cleanup;
    pellets.sprinkle_rate_toxic = toxic_rates.sprinkle;
    pellets.cleanup_rate_toxic = toxic_rates.cleanup;
    pellets.sprinkle_rate_division = division_rates.sprinkle;
    pellets.cleanup_rate_division = division_rates.cleanup;
}

void Game::cleanup_pellets_by_rate(float timeStep) {
    adjust_cleanup_rates();
    // continuous pellet cleanup by rate (percent per second)
    if (pellets.cleanup_rate_food > 0.0f) {
        remove_percentage_pellets(pellets.cleanup_rate_food * timeStep, false, false);
    }
    if (pellets.cleanup_rate_toxic > 0.0f) {
        remove_percentage_pellets(pellets.cleanup_rate_toxic * timeStep, true, false);
    }
    if (pellets.cleanup_rate_division > 0.0f) {
        remove_percentage_pellets(pellets.cleanup_rate_division * timeStep, false, true);
    }
}

void Game::remove_stopped_boost_particles() {
    constexpr float vel_epsilon = 1e-3f;
    auto snapshot = selection.capture_snapshot();
    circles.erase(
        std::remove_if(
            circles.begin(),
            circles.end(),
            [&](const std::unique_ptr<EatableCircle>& circle) {
                if (!circle->is_boost_particle()) {
                    return false;
                }
                b2Vec2 v = circle->getLinearVelocity();
                return (std::fabs(v.x) <= vel_epsilon && std::fabs(v.y) <= vel_epsilon);
            }),
        circles.end());
    selection.handle_selection_after_removal(snapshot, false, nullptr, snapshot.position);
    refresh_generation_and_age();
}
