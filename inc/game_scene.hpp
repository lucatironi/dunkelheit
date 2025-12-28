#pragma once

#include "enemy.hpp"
#include "entity.hpp"
#include "item.hpp"
#include "level.hpp"
#include "object.hpp"
#include "settings.hpp"
#include "texture_2D.hpp"

class GameScene
{
public:
    GameScene(const SettingsData& settings)
        : settings(settings)
    {
        Texture2D levelTexture(settings.LevelTextureFile);
        level = new Level(settings.LevelMapFile, levelTexture);

        for (const auto& position : level->GetEnemyPositions())
            AddEnemy(position);
    }

    ~GameScene()
    {
        delete level;
    }

    void AddItem(std::string& modelPath, std::string& texturePath,
         glm::vec3 posOffset, glm::vec3 rotOffset, glm::vec3 scaleFactor)
    {
        items.emplace_back(modelPath, texturePath, posOffset, rotOffset, scaleFactor);
        refreshRenderList();
    }

    void AddEnemy(glm::vec3 position)
    {
        position.y = 0.0f;
        enemies.emplace_back(settings.EnemyModelFile, settings.EnemyTextureFile, position, 90.0f, glm::vec3(2.5f));
        refreshRenderList();
    }

    void AddObject(glm::vec3 position)
    {
        objects.emplace_back(position);
        refreshRenderList();
    }

    const glm::vec3 GetStartingPosition()
    {
        return level->StartingPosition;
    }

    void SetLights(Shader& shader)
    {
        level->SetLights(shader);
    }

    void Update(FPSCamera& camera)
    {
        handleCollisions(camera);
        for (auto& enemy : enemies)
            enemy.Update();
        for (auto& item : items)
            item.Update(camera);
    }

    void Draw(const Shader& shader)
    {
        for (Entity* entity : renderList)
        {
            if (entity->AlwaysOnTop)
                glClear(GL_DEPTH_BUFFER_BIT);

            entity->Draw(shader);
        }
    }

private:
    Level* level;
    std::vector<Item> items;
    std::vector<Enemy> enemies;
    std::vector<Object> objects;
    std::vector<Entity*> renderList;
    SettingsData settings;

    void refreshRenderList()
    {
        renderList.clear();

        renderList.push_back(level);

        for (auto& enemy : enemies)
            renderList.push_back(&enemy);

        for (auto& object : objects)
            renderList.push_back(&object);

        for (auto& item : items)
            renderList.push_back(&item);

        // Move AlwaysOnTop entities to the end of the vector
        std::stable_partition(renderList.begin(), renderList.end(), [](Entity* entity)
        {
            return !entity->AlwaysOnTop;
        });
    }

    void handleCollisions(FPSCamera& camera)
    {
        for (const auto& tile : level->GetNeighboringTiles(camera.Position))
        {
            if (tile.key == TileKey::COLOR_WALL || tile.key == TileKey::COLOR_EMPTY)
            {
                glm::vec3 nearestPoint;
                nearestPoint.x = glm::clamp(camera.Position.x, tile.aabb.min.x, tile.aabb.max.x);
                nearestPoint.z = glm::clamp(camera.Position.z, tile.aabb.min.z, tile.aabb.max.z);
                glm::vec3 rayToNearest = nearestPoint - camera.Position;
                rayToNearest.y = 0.0f; // y component is irrelevant
                float overlap = settings.PlayerCollisionRadius - glm::length(rayToNearest);
                if (std::isnan(overlap))
                    overlap = 0.0f;
                if (overlap > 0.0f)
                    camera.Position -= glm::normalize(rayToNearest) * overlap;
            }
        }
    }
};