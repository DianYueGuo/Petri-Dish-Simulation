#ifndef EATABLE_CIRCLE_HPP
#define EATABLE_CIRCLE_HPP

#include "drawable_circle.hpp"

class CreatureCircle;

class EatableCircle : public DrawableCircle {
public:
    EatableCircle(const b2WorldId &worldId,
                  float position_x = 0.0f,
                  float position_y = 0.0f,
                  float radius = 1.0f,
                  float density = 1.0f,
                  bool toxic = false,
                  bool division_pellet = false,
                  float angle = 0.0f,
                  bool boost_particle = false);
    void be_eaten();
    void set_eaten_by(const CreatureCircle* creature) { eaten_by = creature; }
    const CreatureCircle* get_eaten_by() const { return eaten_by; }
    bool is_eaten() const;
    bool is_toxic() const { return toxic; }
    void set_toxic(bool value) { toxic = value; update_kind_from_flags(); }
    bool is_division_pellet() const { return division_pellet; }
    void set_division_pellet(bool value) { division_pellet = value; update_kind_from_flags(); }
    bool is_boost_particle() const { return boost_particle; }
private:
    void update_kind_from_flags();
    bool eaten = false;
    bool toxic = false;
    bool division_pellet = false;
    bool boost_particle = false;
    const CreatureCircle* eaten_by = nullptr;
};

#endif
