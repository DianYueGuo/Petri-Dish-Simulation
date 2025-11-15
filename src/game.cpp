#include "game.hpp"

void Game::render() const {
    window.clear(sf::Color::Black);

    sf::CircleShape shape(50.f);
    shape.setFillColor(sf::Color(100, 250, 50));
    window.draw(shape);

    window.display();
}
