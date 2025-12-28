#include <algorithm>
#include <limits>

#include "game/game_components.hpp"

#include "creatures/creature_circle.hpp"
#include "game/game.hpp"

GameSelectionController::GameSelectionController(Game& game) : game(game) {}

Game::SelectionMode GameSelectionController::sanitize_selection_mode(Game::SelectionMode mode) {
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

void GameSelectionController::clear_selection() {
    game.selection.clear();
}

bool GameSelectionController::select_circle_at_world(const b2Vec2& pos) {
    return game.selection.select_circle_at_world(pos);
}

const neat::Genome* GameSelectionController::get_selected_brain() const {
    return game.selection.get_selected_brain();
}

const CreatureCircle* GameSelectionController::get_selected_creature() const {
    return game.selection.get_selected_creature();
}

const CreatureCircle* GameSelectionController::get_oldest_largest_creature() const {
    return game.selection.get_oldest_largest_creature();
}

const CreatureCircle* GameSelectionController::get_oldest_smallest_creature() const {
    return game.selection.get_oldest_smallest_creature();
}

#ifndef NDEBUG
const CreatureCircle* GameSelectionController::get_oldest_middle_creature() const {
    return game.selection.get_oldest_middle_creature();
}
#endif

const CreatureCircle* GameSelectionController::get_follow_target_creature() const {
    return game.selection.get_follow_target_creature();
}

int GameSelectionController::get_selected_generation() const {
    return game.selection.get_selected_generation();
}

void GameSelectionController::set_follow_selected(bool v) {
    game.selection.set_follow_selected(v);
}

bool GameSelectionController::get_follow_selected() const {
    return game.selection.get_follow_selected();
}

void GameSelectionController::set_selection_mode(Game::SelectionMode mode) {
    game.selection_mode = sanitize_selection_mode(mode);
    game.selection_dirty = true;
    apply_selection_mode();
}

Game::SelectionMode GameSelectionController::get_selection_mode() const {
    return game.selection_mode;
}

void GameSelectionController::update_follow_view(sf::View& view) const {
    game.selection.update_follow_view(view);
}

void GameSelectionController::apply_selection_mode() {
    if (game.selection_mode == Game::SelectionMode::Manual) {
        return;
    }

    if (!game.selection_dirty) {
        return;
    }

    switch (game.selection_mode) {
        case Game::SelectionMode::OldestLargest:
            game.selection.set_selection_to_creature(game.selection.get_oldest_largest_creature());
            break;
#ifndef NDEBUG
        case Game::SelectionMode::OldestMedian:
            game.selection.set_selection_to_creature(game.selection.get_oldest_middle_creature());
            break;
#endif
        case Game::SelectionMode::OldestSmallest:
            game.selection.set_selection_to_creature(game.selection.get_oldest_smallest_creature());
            break;
        case Game::SelectionMode::Manual:
        default:
            break;
    }

    game.selection_dirty = false;
}

void GameSelectionController::update_max_generation_from_circle(const EatableCircle* circle) {
    if (circle && circle->get_kind() == CircleKind::Creature) {
        const auto* creature_circle = static_cast<const CreatureCircle*>(circle);
        if (creature_circle->get_generation() > game.generation.max_generation) {
            game.generation.max_generation = creature_circle->get_generation();
            game.generation.brain = creature_circle->get_brain();
        }
    }
}

void GameSelectionController::recompute_max_generation() {
    int new_max = 0;
    std::optional<neat::Genome> new_brain;
    for (const auto& circle : game.circles) {
        if (circle && circle->get_kind() == CircleKind::Creature) {
            const auto* creature_circle = static_cast<const CreatureCircle*>(circle.get());
            if (creature_circle->get_generation() >= new_max) {
                new_max = creature_circle->get_generation();
                new_brain = creature_circle->get_brain();
            }
        }
    }
    game.generation.max_generation = new_max;
    game.generation.brain = std::move(new_brain);
}

void GameSelectionController::update_max_ages() {
    if (game.age.dirty) {
        float creation_min = std::numeric_limits<float>::max();
        float division_min = std::numeric_limits<float>::max();
        bool found = false;
        for (const auto& circle : game.circles) {
            if (circle && circle->get_kind() == CircleKind::Creature) {
                const auto* creature_circle = static_cast<const CreatureCircle*>(circle.get());
                creation_min = std::min(creation_min, creature_circle->get_creation_time());
                division_min = std::min(division_min, creature_circle->get_last_division_time());
                found = true;
            }
        }
        if (found) {
            game.age.has_creature = true;
            game.age.min_creation_time = creation_min;
            game.age.min_division_time = division_min;
        } else {
            game.age.has_creature = false;
            game.age.min_creation_time = 0.0f;
            game.age.min_division_time = 0.0f;
        }
        game.age.dirty = false;
    }

    if (!game.age.has_creature) {
        game.age.max_age_since_creation = 0.0f;
        game.age.max_age_since_division = 0.0f;
        return;
    }

    game.age.max_age_since_creation = std::max(0.0f, game.timing.sim_time_accum - game.age.min_creation_time);
    game.age.max_age_since_division = std::max(0.0f, game.timing.sim_time_accum - game.age.min_division_time);
}

void GameSelectionController::mark_age_dirty() {
    game.age.dirty = true;
}

void GameSelectionController::mark_selection_dirty() {
    game.selection_dirty = true;
}

void GameSelectionController::set_selection_to_creature(const CreatureCircle* creature) {
    game.selection.set_selection_to_creature(creature);
}

const CreatureCircle* GameSelectionController::find_nearest_creature(const b2Vec2& pos) const {
    return game.selection.find_nearest_creature(pos);
}

void GameSelectionController::refresh_generation_and_age() {
    recompute_max_generation();
    update_max_ages();
}
