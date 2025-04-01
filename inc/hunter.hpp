#pragma once

#include "entity.hpp"
#include "model.hpp"

class Hunter : public Entity
{
public:
    Hunter(const std::string& modelPath)
    {

    }

    void Draw(const Shader& shader) const override
    {

    }

private:
    std::unique_ptr<Model> hunterModel;
    glm::vec3 position;
};