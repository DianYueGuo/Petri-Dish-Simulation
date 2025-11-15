#include "cell.hpp"

void Cell::draw(sf::RenderWindow& window) const {
    sf::CircleShape shape(50.f);
    shape.setFillColor(sf::Color(100, 250, 50));
    window.draw(shape);
}
