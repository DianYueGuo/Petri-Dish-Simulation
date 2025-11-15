#ifndef GAME_HPP
#define GAME_HPP

#include <SFML/Graphics.hpp>

class Game {
public:
    Game(sf::RenderWindow& window) :
        window(window) {};

    void handleInput();
    void render() const;
private:
    sf::RenderWindow& window;
};

#endif