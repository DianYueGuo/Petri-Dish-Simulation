#include "circle_physics.hpp"


CirclePhysics::CirclePhysics(b2WorldId &worldId, float position_x, float position_y, float radius, float density, float friction) :
    bodyId{} {
    b2BodyDef circleBodyDef = b2DefaultBodyDef();
    circleBodyDef.type = b2_dynamicBody;
    circleBodyDef.position = (b2Vec2){position_x, position_y};

    bodyId = b2CreateBody(worldId, &circleBodyDef);

    b2ShapeDef CircleShapeDef = b2DefaultShapeDef();
    CircleShapeDef.density = density;
    CircleShapeDef.material.friction = friction;

    b2Circle circle;
    circle.center = (b2Vec2){0.0f, 0.0f};
    circle.radius = radius;

    b2CreateCircleShape(bodyId, &CircleShapeDef, &circle);
}

CirclePhysics::~CirclePhysics() {
    if (b2Body_IsValid(bodyId)) {
        b2DestroyBody(bodyId);
    }
}

CirclePhysics::CirclePhysics(CirclePhysics&& other_circle_physics) noexcept :
    bodyId(other_circle_physics.bodyId) {
    other_circle_physics.bodyId = (b2BodyId){};
}

CirclePhysics& CirclePhysics::operator=(CirclePhysics&& other_circle_physics) noexcept {
    if (this == &other_circle_physics) {
        return *this;
    }

    if (b2Body_IsValid(bodyId)) b2DestroyBody(bodyId);
    bodyId = other_circle_physics.bodyId;
        other_circle_physics.bodyId = (b2BodyId){};

    return *this;
}

b2Vec2 CirclePhysics::getPosition() const {
    return b2Body_GetPosition(bodyId);
}

float CirclePhysics::getRadius() const {
    b2ShapeId shapeId;
    b2Body_GetShapes(bodyId, &shapeId, 1);
    b2Circle circle = b2Shape_GetCircle(shapeId);
    return circle.radius;
}
