#pragma once

#include "fps_camera.hpp"

#include <glm/glm.hpp>

class Torch
{
public:
    glm::vec3 Position;
    glm::vec3 PositionOffset;
    glm::vec3 Direction;

    Torch(const glm::vec3& offset = glm::vec3(0.0f))
        : PositionOffset(offset)
    {}

    void Update(const FPSCamera& camera)
    {
        Position = camera.Position + camera.Front * PositionOffset.z
                                   + camera.Up * PositionOffset.y
                                   + camera.Right * PositionOffset.x;
        Direction = camera.TargetFront;
    }

private:

};