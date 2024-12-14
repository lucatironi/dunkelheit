#pragma once

#include <vector>

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

// Defines several possible options for camera movement. Used as abstraction to stay away from window-system specific input methods
enum CameraMovement
{
    CAMERA_FORWARD,
    CAMERA_BACKWARD,
    CAMERA_LEFT,
    CAMERA_RIGHT,
    CAMERA_UP,
    CAMERA_DOWN
};

// Default camera values
const float CAMERA_YAW = -90.0f;
const float CAMERA_PITCH = 0.0f;
const float CAMERA_SPEED = 2.5f;
const float CAMERA_SENSITIVITY = 0.1f;
const float CAMERA_ZOOM = 45.0f;
const float CAMERA_HEAD_HEIGHT = 1.75f;
const glm::vec3 ZERO_VEC = glm::vec3(0.0f, 0.0f, 0.0f);
const glm::vec3 UP_VEC = glm::vec3(0.0f, 1.0f, 0.0f);
const glm::vec3 FRONT_VEC = glm::vec3(0.0f, 0.0f, -1.0f);

class FPSCamera
{
public:
    // Camera Attributes
    glm::vec3 Position;
    glm::vec3 Front;
    glm::vec3 Up;
    glm::vec3 Right;
    glm::vec3 WorldUp;
    // Euler Angles
    float Yaw;
    float Pitch;
    // Camera options
    float MovementSpeed;
    float MouseSensitivity;
    float Zoom;
    bool Constrained;

    FPSCamera(glm::vec3 position = ZERO_VEC,
        glm::vec3 up = UP_VEC,
        float yaw = CAMERA_YAW,
        float pitch = CAMERA_PITCH,
        bool constrained = true) :
            Front(FRONT_VEC),
            MovementSpeed(CAMERA_SPEED),
            MouseSensitivity(CAMERA_SENSITIVITY),
            Zoom(CAMERA_ZOOM)
    {
        Position = position;
        WorldUp = up;
        Yaw = yaw;
        Pitch = pitch;
        Constrained = constrained;
        updateCameraVectors();
    }

    // Constructor with scalar values
    FPSCamera(float posX = 0.0f, float posY = 0.0f, float posZ = 0.0f,
        float upX = 0.0f, float upY = 1.0f, float upZ = 0.0f,
        float yaw = CAMERA_YAW,
        float pitch = CAMERA_PITCH,
        bool constrained = true) :
            Front(FRONT_VEC),
            MovementSpeed(CAMERA_SPEED),
            MouseSensitivity(CAMERA_SENSITIVITY),
            Zoom(CAMERA_ZOOM)
    {
        Position = glm::vec3(posX, posY, posZ);
        WorldUp = glm::vec3(upX, upY, upZ);
        Yaw = yaw;
        Pitch = pitch;
        Constrained = constrained;
        updateCameraVectors();
    }

    // Returns the view matrix calculated using Euler Angles and the LookAt Matrix
    glm::mat4 GetViewMatrix()
    {
        return glm::lookAt(Position, Position + Front, Up);
    }

    // Processes input received from any keyboard-like input system.
    // Accepts input parameter in the form of camera defined ENUM (to abstract it from windowing systems)
    void ProcessInputMovement(CameraMovement direction, float deltaTime)
    {
        if (deltaTime <= 0.0f) return; // Early exit if deltaTime is invalid

        float velocity = MovementSpeed * deltaTime;
        glm::vec3 directionVector = glm::vec3(0.0f);

        glm::vec3 adjustedFront = Front;

        // Ignore the y component of Front for constrained movement
        if (Constrained)
        {
            adjustedFront.y = 0.0f;
            adjustedFront = glm::normalize(adjustedFront); // Re-normalize after projection
        }

        // Add movement contributions
        if (direction == CAMERA_FORWARD)
            directionVector += adjustedFront;
        if (direction == CAMERA_BACKWARD)
            directionVector -= adjustedFront;
        if (direction == CAMERA_LEFT)
            directionVector -= Right;
        if (direction == CAMERA_RIGHT)
            directionVector += Right;

        // Normalize direction vector to prevent diagonal speed boost
        if (glm::length(directionVector) > 0.0f)
            directionVector = glm::normalize(directionVector);

        // Update position
        Position += directionVector * velocity;

        // Constrain Y-axis position if needed
        if (Constrained)
            Position.y = CAMERA_HEAD_HEIGHT;
    }

    // Processes input received from a mouse input system. Expects the offset value in both the x and y direction.
    void ProcessMouseMovement(float xoffset, float yoffset, GLboolean constrainPitch = true)
    {
        xoffset *= MouseSensitivity;
        yoffset *= MouseSensitivity;

        Yaw += xoffset;
        Pitch += yoffset;

        // Make sure that when pitch is out of bounds, screen doesn't get flipped
        if (constrainPitch)
        {
            if (Pitch > 80.0f)
                Pitch = 80.0f;
            if (Pitch < -80.0f)
                Pitch = -80.0f;
        }

        // Update Front, Right and Up Vectors using the updated Euler angles
        updateCameraVectors();
    }

    // Processes input received from a mouse scroll-wheel event. Only requires input on the vertical wheel-axis
    void ProcessMouseZoom(float yoffset)
    {
        if (Zoom >= 1.0f && Zoom <= 45.0f)
            Zoom -= yoffset;
        if (Zoom <= 1.0f)
            Zoom = 1.0f;
        if (Zoom >= 45.0f)
            Zoom = 45.0f;
    }

private:
    // Calculates the front vector from the Camera's (updated) Euler Angles
    void updateCameraVectors()
    {
        // Calculate the new Front vector
        glm::vec3 front;
        front.x = cos(glm::radians(Yaw)) * cos(glm::radians(Pitch));
        front.y = sin(glm::radians(Pitch));
        front.z = sin(glm::radians(Yaw)) * cos(glm::radians(Pitch));
        Front = glm::normalize(front);
        // Also re-calculate the Right and Up vector
        Right = glm::normalize(glm::cross(Front, WorldUp)); // Normalize the vectors, because their length gets closer to 0 the more you look up or down which results in slower movement.
        Up = glm::normalize(glm::cross(Right, Front));
    }
};
