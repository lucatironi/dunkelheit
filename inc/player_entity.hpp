#pragma once

#include "fps_camera.hpp"
#include "glm/fwd.hpp"

#include <glm/glm.hpp>

class PlayerEntity
{
public:
    static constexpr float DEFAULT_SPEED = 5.0f;
    static constexpr float DEFAULT_COL_RADIUS = 0.60f;
    static constexpr float DEFAULT_HEAD_HEIGHT = 1.75f;

    FPSCamera* Camera;
    glm::vec3 Position;
    glm::vec3 Velocity;
    float CollisionRadius;
    float Speed;

    PlayerEntity(glm::vec3 position, FPSCamera* camera)
        : Camera(camera), Speed(DEFAULT_SPEED), CollisionRadius(DEFAULT_COL_RADIUS)
    {
        SetPosition(position);
    }

    void SetPosition(glm::vec3 position)
    {
        Position = position;
        Camera->Position = Position;
    }

    void Move(MovementDirection direction, const float deltaTime)
    {
         if (deltaTime <= 0.0f)
            return;

        Velocity = glm::vec3(0.0f);

        switch (direction)
        {
        case MOVE_FORWARD:
            Velocity += Camera->Front;
            break;
        case MOVE_BACKWARD:
            Velocity -= Camera->Front;
            break;
        case MOVE_LEFT:
            Velocity -= Camera->Right;
            break;
        case MOVE_RIGHT:
            Velocity += Camera->Right;
            break;
        default:
            break;
        }

        if (Velocity.length() > 0.0f)
            Velocity = glm::normalize(Velocity) * Speed;

        Position += Velocity * deltaTime;
        Position.y = DEFAULT_HEAD_HEIGHT;
        Camera->Position = Position;
    }

private:

};