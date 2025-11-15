#include <iostream>

#include <SFML/Graphics.hpp>


int main() {
    sf::RenderWindow window(sf::VideoMode({800, 600}), "My window");

    while (window.isOpen())
    {
        while (const std::optional event = window.pollEvent())
        {
            if (event->is<sf::Event::Closed>())
                window.close();
        }

        window.clear(sf::Color::Black);

        sf::CircleShape shape(50.f);
        shape.setFillColor(sf::Color(100, 250, 50));
        window.draw(shape);

        window.display();
    }

    return 0;
}
