#pragma once

#include "entity.hpp"
#include "model.hpp"

#include <glad/gl.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <memory>
#include <string>

class Enemy : public Entity
{
public:
    Enemy(const std::string& modelPath, const std::string& texturePath,
          const glm::vec3 position, const float angleY, const glm::vec3 scaleFactor)
          : position(position), scaleFactor(scaleFactor), angleY(angleY)
    {
        enemyModel = std::make_unique<Model>(modelPath);
        if (!texturePath.empty())
            enemyModel->TextureOverride(texturePath);

        updateModelMatrix();
    }

    void Update()
    {
        updateModelMatrix();
    }

    void Draw(const Shader& shader) const override
    {
        shader.Use();
        shader.SetMat4("model", modelMatrix);

        enemyModel->Draw(shader);
    }

private:
    std::unique_ptr<Model> enemyModel;
    glm::vec3 position;
    glm::vec3 scaleFactor;
    float angleY;
    glm::mat4 modelMatrix;

    void updateModelMatrix()
    {
        glm::mat4 T = glm::translate(glm::mat4(1.0f), position);
        glm::mat4 R = glm::mat4_cast(glm::angleAxis(glm::radians(angleY), glm::vec3(0.0f, 1.0f, 0.0f)));
        glm::mat4 S = glm::scale(glm::mat4(1.0f), scaleFactor);

        // Model = Translation * Rotation * Scale
        modelMatrix = T * R * S;
    }
};