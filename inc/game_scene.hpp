#pragma once

#include "enemy.hpp"
#include "entity.hpp"
#include "item.hpp"
#include "level.hpp"
#include "object.hpp"
#include "random_generator.hpp"
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

    void Reset()
    {
        for (auto& enemy : enemies)
            enemy->Reset();
    }

    void AddItem(std::string& modelPath, std::string& texturePath,
         glm::vec3 posOffset, glm::vec3 rotOffset, glm::vec3 scaleFactor)
    {
        items.push_back(std::make_unique<Item>(modelPath, texturePath, posOffset, rotOffset, scaleFactor));
        refreshRenderList();
    }

    void AddEnemy(glm::vec3 position)
    {
        float angle = static_cast<float>(random.GetRandomInRange(0, 360));
        position.y = 0.0f;
        enemies.push_back(std::make_unique<Enemy>(settings.EnemyModelFile, position, angle, glm::vec3(0.5f)));
        refreshRenderList();
    }

    void AddObject(glm::vec3 position)
    {
        objects.push_back(std::make_unique<Object>(position));
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

    void Update(float deltaTime, FPSCamera& camera)
    {
        handleCollisions(camera);
        for (auto& enemy : enemies)
            enemy->Update(deltaTime, camera, *level);
        for (auto& item : items)
            item->Update(deltaTime, camera);

        for (auto& a : enemies)
        {
            for (auto& b : enemies)
            {
                if (&a == &b) continue;
                float dist = glm::distance(a->GetPosition(), b->GetPosition());
                if (dist < 1.5f)
                {
                    glm::vec3 escape = glm::normalize(a->GetPosition() - b->GetPosition());
                    a->SetPosition(a->GetPosition() + (escape * 0.1f)); // Small push away
                }
            }
        }
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

    void ToggleSounds(const bool pause)
    {
        for (auto& enemy : enemies)
            enemy->ToggleSound(pause);
    }

private:
    Level* level;
    std::vector<std::unique_ptr<Enemy>> enemies;
    std::vector<std::unique_ptr<Object>> objects;
    std::vector<std::unique_ptr<Item>> items;
    std::vector<Entity*> renderList;
    SettingsData settings;
    RandomGenerator& random = RandomGenerator::GetInstance();

    void refreshRenderList()
    {
        renderList.clear();

        renderList.push_back(level);

        for (auto& enemy : enemies)
            renderList.push_back(enemy.get());

        for (auto& object : objects)
            renderList.push_back(object.get());

        for (auto& item : items)
            renderList.push_back(item.get());

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