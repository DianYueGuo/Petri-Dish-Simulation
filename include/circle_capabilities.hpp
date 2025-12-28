#pragma once

#include <array>

#include <box2d/box2d.h>

class CreatureCircle;

struct ISenseable {
    virtual ~ISenseable() = default;
    virtual b2Vec2 sense_position() const = 0;
    virtual float sense_radius() const = 0;
    virtual const std::array<float, 3>& sense_color() const = 0;
};

struct IEdible {
    virtual ~IEdible() = default;
    virtual bool edible_is_eaten() const = 0;
    virtual bool edible_is_toxic() const = 0;
    virtual bool edible_is_division_pellet() const = 0;
    virtual bool edible_is_boost_particle() const = 0;
    virtual float edible_area() const = 0;
    virtual void edible_mark_eaten(CreatureCircle* eater) = 0;
};
