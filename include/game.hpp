#ifndef GAME_HPP
#define GAME_HPP

#include <SFML/Graphics.hpp>
#include <box2d/box2d.h>

#include "cell.hpp"

class Game {
public:
    Game(sf::RenderWindow& window);

    void handleInput();
    void update();
    void render() const;
private:
    sf::RenderWindow& window;

    b2WorldId worldId;

    Cell cell;
};

#endif