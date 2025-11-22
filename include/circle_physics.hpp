#ifndef CIRCLE_PHYSICS_HPP
#define CIRCLE_PHYSICS_HPP

#include <box2d/box2d.h>


class CirclePhysics {
public:
    CirclePhysics(b2WorldId &worldId, float position_x = 0.0f, float position_y = 0.0f, float radius = 1.0f, float density = 1.0f, float friction = 0.0f);

    ~CirclePhysics();

    CirclePhysics(const CirclePhysics&) = delete;
    CirclePhysics& operator=(const CirclePhysics&) = delete;

    CirclePhysics(CirclePhysics&& other_circle_physics) noexcept;

    CirclePhysics& operator=(CirclePhysics&& other_circle_physics) noexcept;

    b2Vec2 getPosition() const;

    float getRadius() const;
private:
    b2BodyId bodyId;
};

#endif
