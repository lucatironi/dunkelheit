#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/gtc/quaternion.hpp>

#include <vector>

// Enumerates camera movement directions.
enum MovementDirection
{
    MOVE_FORWARD,
    MOVE_BACKWARD,
    MOVE_LEFT,
    MOVE_RIGHT,
    MOVE_UP,
    MOVE_DOWN
};

class FPSCamera
{
public:
    // Default camera values.
    static constexpr float DEFAULT_YAW = -90.0f;
    static constexpr float DEFAULT_PITCH = 0.0f;
    static constexpr float DEFAULT_SPEED = 5.0f;
    static constexpr float DEFAULT_SENSITIVITY = 0.1f;
    static constexpr float DEFAULT_ZOOM = 45.0f;
    static constexpr float DEFAULT_HEAD_HEIGHT = 1.75f;
    static constexpr glm::vec3 DEFAULT_POSITION = glm::vec3(0.0f);
    static constexpr glm::vec3 DEFAULT_UP = glm::vec3(0.0f, 1.0f, 0.0f);
    static constexpr glm::vec3 DEFAULT_FRONT = glm::vec3(0.0f, 0.0f, -1.0f);

    // Camera Attributes
    glm::vec3 Position;
    glm::vec3 Velocity;
    glm::vec3 Front;
    glm::vec3 Up;
    glm::vec3 Right;
    glm::vec3 WorldUp;

    // Euler Angles
    float Yaw;
    float Pitch;

    // Camera options
    float HeadHeight;
    float MovementSpeed;
    float MouseSensitivity;
    float Zoom;
    bool Constrained;

    // Constructor
    FPSCamera(glm::vec3 position = DEFAULT_POSITION,
              glm::vec3 up = DEFAULT_UP,
              float yaw = DEFAULT_YAW,
              float pitch = DEFAULT_PITCH,
              bool constrained = true)
        : Position(position), WorldUp(up), HeadHeight(DEFAULT_HEAD_HEIGHT),
          Yaw(yaw), Pitch(pitch), Constrained(constrained),
          Front(DEFAULT_FRONT), MovementSpeed(DEFAULT_SPEED),
          MouseSensitivity(DEFAULT_SENSITIVITY), Zoom(DEFAULT_ZOOM)
    {
        updateCameraVectors();
    }

    // Returns the view matrix using Euler angles and the LookAt matrix.
    glm::mat4 GetViewMatrix() const
    {
        return glm::lookAt(Position, Position + Front, Up);
    }

    // Return a quaternion that represents the camera's rotation
    glm::quat GetRotation() const
    {
        return glm::quatLookAt(Front, Up);
    }

    // Processes keyboard-like input for camera movement.
    void Move(const MovementDirection direction, const float deltaTime)
    {
        if (deltaTime <= 0.0f)
            return;

        Velocity = glm::vec3(0.0f);

        switch (direction)
        {
        case MOVE_FORWARD:
            Velocity += Front;
            break;
        case MOVE_BACKWARD:
            Velocity -= Front;
            break;
        case MOVE_LEFT:
            Velocity -= Right;
            break;
        case MOVE_RIGHT:
            Velocity += Right;
            break;
        case MOVE_UP:
            if (!Constrained)
                Velocity += WorldUp;
            break;
        case MOVE_DOWN:
            if (!Constrained)
                Velocity -= WorldUp;
            break;
        }

        if (glm::length(Velocity) > 0.0f)
            Velocity = glm::normalize(Velocity) * MovementSpeed;

        Position += Velocity * deltaTime;

        if (Constrained)
            Position.y = HeadHeight;
    }

    // Processes mouse input for camera rotation.
    void ProcessMouseMovement(float xoffset, float yoffset, bool constrainPitch = true)
    {
        xoffset *= MouseSensitivity;
        yoffset *= MouseSensitivity;

        Yaw += xoffset;
        Pitch += yoffset;

        if (constrainPitch)
            Pitch = glm::clamp(Pitch, -80.0f, 80.0f); // Constrain pitch to avoid flipping.

        updateCameraVectors();
    }

    // Processes input from a mouse scroll-wheel to adjust zoom.
    void ProcessMouseZoom(float yoffset)
    {
        Zoom = glm::clamp(Zoom - yoffset, 1.0f, 45.0f);
    }

private:
    // Updates the camera's vectors based on the current yaw and pitch angles.
    void updateCameraVectors()
    {
        glm::vec3 front;
        front.x = cos(glm::radians(Yaw)) * cos(glm::radians(Pitch));
        front.y = sin(glm::radians(Pitch));
        front.z = sin(glm::radians(Yaw)) * cos(glm::radians(Pitch));
        Front = glm::normalize(front);

        // Recalculate Right and Up vectors.
        Right = glm::normalize(glm::cross(Front, WorldUp));
        Up = glm::normalize(glm::cross(Right, Front));
    }
};
