#include "game/game_components.hpp"

#include "game/game.hpp"

GameInputHandler::GameInputHandler(Game& game) : game(game) {}

sf::Vector2f GameInputHandler::pixel_to_world(sf::RenderWindow& window, const sf::Vector2i& pixel) const {
    sf::Vector2f viewPos = window.mapPixelToCoords(pixel);
    return viewPos;
}

void GameInputHandler::start_view_drag(const sf::Event::MouseButtonPressed& e, bool is_right_button) {
    game.view_drag.dragging = true;
    game.view_drag.right_dragging = is_right_button;
    game.view_drag.last_drag_pixels = e.position;
}

void GameInputHandler::pan_view(sf::RenderWindow& window, const sf::Event::MouseMoved& e) {
    if (!game.view_drag.dragging) {
        return;
    }

    sf::View view = window.getView();
    sf::Vector2f pixels_to_world = {
        view.getSize().x / static_cast<float>(window.getSize().x),
        view.getSize().y / static_cast<float>(window.getSize().y)
    };

    sf::Vector2i current_pixels = e.position;
    sf::Vector2i delta_pixels = game.view_drag.last_drag_pixels - current_pixels;
    sf::Vector2f delta_world = {
        static_cast<float>(delta_pixels.x) * pixels_to_world.x,
        static_cast<float>(delta_pixels.y) * pixels_to_world.y
    };

    view.move(delta_world);
    window.setView(view);
    game.view_drag.last_drag_pixels = current_pixels;
}

void GameInputHandler::handle_mouse_press(sf::RenderWindow& window, const sf::Event::MouseButtonPressed& e) {
    if (e.button == sf::Mouse::Button::Left) {
        sf::Vector2f worldPos = pixel_to_world(window, e.position);

        if (game.cursor.mode == Game::CursorMode::Add) {
            game.spawner.spawn_selected_type_at(worldPos);
            game.spawner.begin_add_drag_if_applicable(worldPos);
        } else if (game.cursor.mode == Game::CursorMode::Select) {
            game.selection_ctrl().select_circle_at_world({worldPos.x, worldPos.y});
        }
    } else if (e.button == sf::Mouse::Button::Right) {
        start_view_drag(e, true);
    }
}

void GameInputHandler::handle_mouse_release(const sf::Event::MouseButtonReleased& e) {
    if (e.button == sf::Mouse::Button::Right) {
        game.view_drag.dragging = false;
        game.view_drag.right_dragging = false;
    }
    if (e.button == sf::Mouse::Button::Left) {
        game.spawner.reset_add_drag_state();
    }
}

void GameInputHandler::handle_mouse_move(sf::RenderWindow& window, const sf::Event::MouseMoved& e) {
    game.spawner.continue_add_drag(pixel_to_world(window, {e.position.x, e.position.y}));
    pan_view(window, e);
}

void GameInputHandler::handle_key_press(sf::RenderWindow& window, const sf::Event::KeyPressed& e) {
    sf::View view = window.getView();
    const float pan_fraction = 0.02f;
    const float pan_x = view.getSize().x * pan_fraction;
    const float pan_y = view.getSize().y * pan_fraction;
    constexpr float zoom_step = 1.05f;

    switch (e.scancode) {
        case sf::Keyboard::Scancode::W:
            view.move({0.0f, -pan_y});
            break;
        case sf::Keyboard::Scancode::S:
            view.move({0.0f, pan_y});
            break;
        case sf::Keyboard::Scancode::A:
            view.move({-pan_x, 0.0f});
            break;
        case sf::Keyboard::Scancode::D:
            view.move({pan_x, 0.0f});
            break;
        case sf::Keyboard::Scancode::Q:
            view.zoom(1.0f / zoom_step);
            break;
        case sf::Keyboard::Scancode::E:
            view.zoom(zoom_step);
            break;
        case sf::Keyboard::Scancode::Left:
            game.possesing.left_key_down = true;
            break;
        case sf::Keyboard::Scancode::Right:
            game.possesing.right_key_down = true;
            break;
        case sf::Keyboard::Scancode::Up:
            game.possesing.up_key_down = true;
            break;
        case sf::Keyboard::Scancode::Space:
            game.possesing.space_key_down = true;
            break;
        default:
            break;
    }

    window.setView(view);
}

void GameInputHandler::handle_key_release(const sf::Event::KeyReleased& e) {
    switch (e.scancode) {
        case sf::Keyboard::Scancode::Left:
            game.possesing.left_key_down = false;
            break;
        case sf::Keyboard::Scancode::Right:
            game.possesing.right_key_down = false;
            break;
        case sf::Keyboard::Scancode::Up:
            game.possesing.up_key_down = false;
            break;
        case sf::Keyboard::Scancode::Space:
            game.possesing.space_key_down = false;
            break;
        default:
            break;
    }
}

void GameInputHandler::process_input_events(sf::RenderWindow& window, const std::optional<sf::Event>& event) {
    if (!event) {
        return;
    }

    if (const auto* mouseButtonPressed = event->getIf<sf::Event::MouseButtonPressed>()) {
        handle_mouse_press(window, *mouseButtonPressed);
    }

    if (const auto* mouseButtonReleased = event->getIf<sf::Event::MouseButtonReleased>()) {
        handle_mouse_release(*mouseButtonReleased);
    }

    if (const auto* mouseMoved = event->getIf<sf::Event::MouseMoved>()) {
        handle_mouse_move(window, *mouseMoved);
    }

    if (const auto* mouseWheel = event->getIf<sf::Event::MouseWheelScrolled>()) {
        sf::View view = window.getView();
        constexpr float zoom_factor = 1.05f;
        if (mouseWheel->delta > 0) {
            view.zoom(1.0f / zoom_factor);
        } else if (mouseWheel->delta < 0) {
            view.zoom(zoom_factor);
        }
        window.setView(view);
    }

    if (const auto* keyPressed = event->getIf<sf::Event::KeyPressed>()) {
        handle_key_press(window, *keyPressed);
    }

    if (const auto* keyReleased = event->getIf<sf::Event::KeyReleased>()) {
        handle_key_release(*keyReleased);
    }
}
