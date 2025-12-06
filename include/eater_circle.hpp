#ifndef EATER_CIRCLE_HPP
#define EATER_CIRCLE_HPP

#include "eatable_circle.hpp"
#include "eater_brain.hpp"

#include <algorithm>
#include <array>

class Game;


class EaterCircle : public EatableCircle {
public:
    EaterCircle(const b2WorldId &worldId, float position_x = 0.0f, float position_y = 0.0f, float radius = 1.0f, float density = 1.0f, float angle = 0.0f, int generation = 0);

    void set_minimum_area(float area) { minimum_area = area; }
    float get_minimum_area() const { return minimum_area; }
    int get_generation() const { return generation; }
    void set_generation(int g) { generation = std::max(0, g); }

    void process_eating(const b2WorldId &worldId, float poison_death_probability_toxic, float poison_death_probability_normal);

    void move_randomly(const b2WorldId &worldId, Game &game);
    void move_intelligently(const b2WorldId &worldId, Game &game);

    void boost_forward(const b2WorldId &worldId, Game& game);
    void divide(const b2WorldId &worldId, Game& game);

    bool is_poisoned() const { return poisoned; }

protected:
    bool should_draw_direction_indicator() const override { return true; }

private:
    void initialize_brain();
    void update_brain_inputs_from_touching();
    void update_color_from_brain();

    EaterBrain brain;
    float minimum_area = 1.0f;
    bool poisoned = false;
    int generation = 0;
};

#endif
