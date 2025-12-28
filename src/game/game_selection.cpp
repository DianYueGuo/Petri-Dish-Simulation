#include <algorithm>
#include <limits>

#include "game.hpp"
#include "creature_circle.hpp"

namespace {
Game::SelectionMode sanitize_selection_mode(Game::SelectionMode mode) {
#ifndef NDEBUG
    switch (mode) {
        case Game::SelectionMode::Manual:
        case Game::SelectionMode::OldestLargest:
        case Game::SelectionMode::OldestMedian:
        case Game::SelectionMode::OldestSmallest:
            return mode;
    }
#else
    switch (mode) {
        case Game::SelectionMode::Manual:
        case Game::SelectionMode::OldestLargest:
        case Game::SelectionMode::OldestSmallest:
            return mode;
    }
#endif
    return Game::SelectionMode::Manual;
}
} // namespace

void Game::clear_selection() {
    selection.clear();
}

bool Game::select_circle_at_world(const b2Vec2& pos) {
    return selection.select_circle_at_world(pos);
}

const neat::Genome* Game::get_selected_brain() const {
    return selection.get_selected_brain();
}

const CreatureCircle* Game::get_selected_creature() const {
    return selection.get_selected_creature();
}

const CreatureCircle* Game::get_oldest_largest_creature() const {
    return selection.get_oldest_largest_creature();
}

const CreatureCircle* Game::get_oldest_smallest_creature() const {
    return selection.get_oldest_smallest_creature();
}

#ifndef NDEBUG
const CreatureCircle* Game::get_oldest_middle_creature() const {
    return selection.get_oldest_middle_creature();
}
#endif

const CreatureCircle* Game::get_follow_target_creature() const {
    return selection.get_follow_target_creature();
}

int Game::get_selected_generation() const {
    return selection.get_selected_generation();
}

void Game::set_follow_selected(bool v) {
    selection.set_follow_selected(v);
}

bool Game::get_follow_selected() const {
    return selection.get_follow_selected();
}

void Game::set_selection_mode(SelectionMode mode) {
    selection_mode = sanitize_selection_mode(mode);
    selection_dirty = true;
    apply_selection_mode();
}

Game::SelectionMode Game::get_selection_mode() const {
    return selection_mode;
}

void Game::update_follow_view(sf::View& view) const {
    selection.update_follow_view(view);
}

void Game::apply_selection_mode() {
    if (selection_mode == SelectionMode::Manual) {
        return;
    }

    if (!selection_dirty) {
        return;
    }

    switch (selection_mode) {
        case SelectionMode::OldestLargest:
            selection.set_selection_to_creature(selection.get_oldest_largest_creature());
            break;
#ifndef NDEBUG
        case SelectionMode::OldestMedian:
            selection.set_selection_to_creature(selection.get_oldest_middle_creature());
            break;
#endif
        case SelectionMode::OldestSmallest:
            selection.set_selection_to_creature(selection.get_oldest_smallest_creature());
            break;
        case SelectionMode::Manual:
        default:
            break;
    }

    selection_dirty = false;
}

void Game::update_max_generation_from_circle(const EatableCircle* circle) {
    if (circle && circle->get_kind() == CircleKind::Creature) {
        const auto* creature_circle = static_cast<const CreatureCircle*>(circle);
        if (creature_circle->get_generation() > generation.max_generation) {
            generation.max_generation = creature_circle->get_generation();
            generation.brain = creature_circle->get_brain();
        }
    }
}

void Game::recompute_max_generation() {
    int new_max = 0;
    std::optional<neat::Genome> new_brain;
    for (const auto& circle : circles) {
        if (circle && circle->get_kind() == CircleKind::Creature) {
            const auto* creature_circle = static_cast<const CreatureCircle*>(circle.get());
            if (creature_circle->get_generation() >= new_max) {
                new_max = creature_circle->get_generation();
                new_brain = creature_circle->get_brain();
            }
        }
    }
    generation.max_generation = new_max;
    generation.brain = std::move(new_brain);
}

void Game::update_max_ages() {
    if (age.dirty) {
        float creation_min = std::numeric_limits<float>::max();
        float division_min = std::numeric_limits<float>::max();
        bool found = false;
        for (const auto& circle : circles) {
            if (circle && circle->get_kind() == CircleKind::Creature) {
                const auto* creature_circle = static_cast<const CreatureCircle*>(circle.get());
                creation_min = std::min(creation_min, creature_circle->get_creation_time());
                division_min = std::min(division_min, creature_circle->get_last_division_time());
                found = true;
            }
        }
        if (found) {
            age.has_creature = true;
            age.min_creation_time = creation_min;
            age.min_division_time = division_min;
        } else {
            age.has_creature = false;
            age.min_creation_time = 0.0f;
            age.min_division_time = 0.0f;
        }
        age.dirty = false;
    }

    if (!age.has_creature) {
        age.max_age_since_creation = 0.0f;
        age.max_age_since_division = 0.0f;
        return;
    }

    age.max_age_since_creation = std::max(0.0f, timing.sim_time_accum - age.min_creation_time);
    age.max_age_since_division = std::max(0.0f, timing.sim_time_accum - age.min_division_time);
}

void Game::mark_age_dirty() {
    age.dirty = true;
}

void Game::mark_selection_dirty() {
    selection_dirty = true;
}

void Game::set_selection_to_creature(const CreatureCircle* creature) {
    selection.set_selection_to_creature(creature);
}

const CreatureCircle* Game::find_nearest_creature(const b2Vec2& pos) const {
    return selection.find_nearest_creature(pos);
}

void Game::refresh_generation_and_age() {
    recompute_max_generation();
    update_max_ages();
}
