#pragma once

#include <SFML/Graphics.hpp>

#include "game/game.hpp"
#include "game/game_components.hpp"

class UiFacade {
public:
    using CursorMode = Game::CursorMode;
    using AddType = Game::AddType;
    using SelectionMode = Game::SelectionMode;

    explicit UiFacade(Game& game_ref)
        : game(&game_ref),
          sim_ctrl(&game_ref.sim()),
          selection(&game_ref.selection_ctrl()),
          population(&game_ref.population_mgr()) {}

    Game& game_ref() { return *game; }
    const Game& game_ref() const { return *game; }
    GameSimulationController& sim() { return *sim_ctrl; }
    const GameSimulationController& sim() const { return *sim_ctrl; }
    GameSelectionController& selection_ctrl() { return *selection; }
    const GameSelectionController& selection_ctrl() const { return *selection; }
    GamePopulationManager& population_mgr() { return *population; }
    const GamePopulationManager& population_mgr() const { return *population; }

private:
    Game* game;
    GameSimulationController* sim_ctrl;
    GameSelectionController* selection;
    GamePopulationManager* population;
};
