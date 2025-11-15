#include "game.hpp"

#include <iostream>


void Game::handleInput() {
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Space)) {
        std::cout << "Space key pressed" << std::endl;
    }

    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Left)) {
        std::cout << "Left key pressed" << std::endl;
    }

    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Right)) {
        std::cout << "Right key pressed" << std::endl;
    }
}

void Game::render() const {
    window.clear(sf::Color::Black);

    sf::CircleShape shape(50.f);
    shape.setFillColor(sf::Color(100, 250, 50));
    window.draw(shape);

    window.display();
}
