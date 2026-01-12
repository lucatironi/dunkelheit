#pragma once

#include "audio_engine.hpp"
#include "shader.hpp"
#include "text_renderer.hpp"

#include <functional>
#include <string>
#include <vector>

class MainMenu
{
public:
    struct MenuItem {
        std::string label;
        std::function<void()> action;
    };

    bool Active = false;

    MainMenu(const std::string& menuItemClickSoundPath)
        : menuItemClickSoundPath(menuItemClickSoundPath)
    {}

    void AddItem(const std::string& label, std::function<void()> action)
    {
        items.push_back({ label, action });
    }

    void NavigateUp()
    {
        selectedIndex = (selectedIndex - 1 + (int)items.size()) % (int)items.size();
        AudioEngine::GetInstance().PlayOneShotSound(menuItemClickSoundPath);
    }

    void NavigateDown()
    {
        selectedIndex = (selectedIndex + 1) % (int)items.size();
        AudioEngine::GetInstance().PlayOneShotSound(menuItemClickSoundPath);
    }

    void Confirm()
    {
        if (items[selectedIndex].action)
            items[selectedIndex].action();
    }

    void Clear()
    {
        items.clear();
        selectedIndex = 0;
    }

    void Reset()
    {
        selectedIndex = 0;
    }

    void Render(TextRenderer& textRenderer, const Shader& shader, int screenW, int screenH) {
        if (!Active) return;

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glDisable(GL_DEPTH_TEST);

        float startX = screenW * 0.35f;
        float startY = screenH * 0.5f;
        float lineSpacing = 40.0f;

        textRenderer.RenderText("dunkelheit", shader, startX, startY + 40.0f, 3.0f, glm::vec3(1.0f));

        for (int i = 0; i < (int)items.size(); ++i)
        {
            bool isSelected = (i == selectedIndex);

            glm::vec3 color = isSelected ? glm::vec3(1.0f, 0.0f, 0.0f) : glm::vec3(1.0f);

            std::string text = items[i].label;
            if (isSelected) text = "> " + text + " <";

            float xPos = startX;
            float yPos = startY - (i * lineSpacing);

            textRenderer.RenderText(text, shader, xPos, yPos, 1.0f, color);
        }

        glEnable(GL_DEPTH_TEST);
    }

private:
    int selectedIndex = 0;
    std::vector<MenuItem> items;
    std::string menuItemClickSoundPath;
};