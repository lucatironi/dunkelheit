#pragma once

#include "shader.hpp"

class Entity
{
public:
    bool AlwaysOnTop = false;

    virtual void Draw(const Shader& shader) const = 0;
};