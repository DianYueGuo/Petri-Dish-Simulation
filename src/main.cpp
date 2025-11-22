#include <iostream>

#include <vector>

#include <SFML/Graphics.hpp>
#include <imgui.h>
#include <imgui-SFML.h>

#include <box2d/box2d.h>

#include "circle_physics.hpp"


int main() {
    b2WorldDef worldDef = b2DefaultWorldDef();
    worldDef.gravity = (b2Vec2){0.0f, 0.0f};
    b2WorldId worldId = b2CreateWorld(&worldDef);

    std::vector<CirclePhysics> circle_physics;

    float timeStep = 1.0f / 60.0f;
    int subStepCount = 4;

    sf::RenderWindow window(sf::VideoMode({640, 480}), "ImGui + SFML = <3");
    window.setFramerateLimit(60);
    if (!ImGui::SFML::Init(window))
        return -1;

    sf::CircleShape shape(100.f);
    shape.setFillColor(sf::Color::Green);

    sf::Clock deltaClock;
    while (window.isOpen())
    {
        b2World_Step(worldId, timeStep, subStepCount);

        while (const auto event = window.pollEvent())
        {
            ImGui::SFML::ProcessEvent(window, *event);

            if (event->is<sf::Event::Closed>())
            {
                window.close();
            }

            if (ImGui::GetIO().WantCaptureMouse)
                continue;

            if (const auto* mouseButtonPressed = event->getIf<sf::Event::MouseButtonPressed>()) {
                if (mouseButtonPressed->button == sf::Mouse::Button::Left) {
                    circle_physics.push_back(
                        CirclePhysics(
                        worldId,
                        mouseButtonPressed->position.x,
                        mouseButtonPressed->position.y,
                        20.0f,
                        1.0f,
                        0.3f
                    ));
                }
            }
        }

        ImGui::SFML::Update(window, deltaClock.restart());

        ImGui::ShowDemoWindow();

        ImGui::Begin("Hello, world!");
        ImGui::Button("Look at this pretty button");
        ImGui::End();

        window.clear();

        for (const auto& circle_physics : circle_physics) {
            shape.setPosition({circle_physics.getPosition().x, circle_physics.getPosition().y});
            shape.setRadius(circle_physics.getRadius());
            window.draw(shape);
        }

        ImGui::SFML::Render(window);
        window.display();
    }

    ImGui::SFML::Shutdown();

    b2DestroyWorld(worldId);

    return 0;
}
