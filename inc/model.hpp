#pragma once

#include "mesh.hpp"
#include "shader.hpp"
#include "texture2D.hpp"

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

class Model
{
public:
    Model(const std::string& path)
    {
        loadModel(path);
    }

    void Draw(const Shader& shader) const
    {
        for (const auto& mesh : meshes)
            mesh.Draw(shader);
    }

    void TextureOverride(const std::string& texturePath, bool alpha = false)
    {
        Texture2D texture2D(texturePath, alpha);
        Texture texture({ texture2D, "texture_diffuse", texturePath });

        for (auto& mesh : meshes)
            mesh.AddTexture(texture);
    }

private:
    std::vector<Mesh> meshes;
    std::string directory;

    void loadModel(const std::string& path)
    {
        Assimp::Importer importer;
        const aiScene* scene = importer.ReadFile(path,
            aiProcessPreset_TargetRealtime_Fast | aiProcess_FlipUVs);

        if (!scene || (scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE) || !scene->mRootNode)
            throw std::runtime_error("ERROR::ASSIMP: " + std::string(importer.GetErrorString()));

        directory = path.substr(0, path.find_last_of('/'));
        processNode(scene->mRootNode, scene);
    }

    // Process a node recursively and convert it to meshes
    void processNode(aiNode* node, const aiScene* scene)
    {
        for (unsigned int i = 0; i < node->mNumMeshes; i++)
        {
            aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
            meshes.emplace_back(processMesh(mesh, scene));
        }

        for (unsigned int i = 0; i < node->mNumChildren; i++)
            processNode(node->mChildren[i], scene);
    }

    Mesh processMesh(aiMesh* mesh, const aiScene* scene)
    {
        std::vector<Vertex> vertices;
        std::vector<GLuint> indices;
        std::vector<Texture> textures;

        // Process vertices
        for (unsigned int i = 0; i < mesh->mNumVertices; i++)
        {
            Vertex vertex = {
                glm::vec3(
                    mesh->mVertices[i].x,
                    mesh->mVertices[i].y,
                    mesh->mVertices[i].z
                ),
                glm::vec3(
                    mesh->mNormals[i].x,
                    mesh->mNormals[i].y,
                    mesh->mNormals[i].z
                ),
                mesh->mTextureCoords[0]
                    ? glm::vec2(mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y)
                    : glm::vec2(0.0f, 0.0f)
            };

            vertices.emplace_back(vertex);
        }

        // Process indices
        for (unsigned int i = 0; i < mesh->mNumFaces; i++)
        {
            const aiFace& face = mesh->mFaces[i];
            for (unsigned int j = 0; j < face.mNumIndices; j++)
                indices.emplace_back(face.mIndices[j]);
        }

        // Process materials
        if (mesh->mMaterialIndex >= 0)
        {
            aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];

            auto diffuseMaps = loadMaterialTextures(material, aiTextureType_DIFFUSE, "texture_diffuse");
            textures.insert(textures.end(), diffuseMaps.begin(), diffuseMaps.end());

            auto specularMaps = loadMaterialTextures(material, aiTextureType_SPECULAR, "texture_specular");
            textures.insert(textures.end(), specularMaps.begin(), specularMaps.end());
        }
        return Mesh(std::move(vertices), std::move(indices), std::move(textures));
    }

    // Load material textures from Assimp
    std::vector<Texture> loadMaterialTextures(aiMaterial* mat, aiTextureType type, const std::string& typeName)
    {
        std::vector<Texture> textures;
        for (unsigned int i = 0; i < mat->GetTextureCount(type); i++)
        {
            aiString str;
            mat->GetTexture(type, i, &str);
            std::string texturePath = directory + "/" + std::string(str.C_Str());

            // Load texture using Texture2D
            Texture2D texture2D(texturePath);
            textures.emplace_back(Texture{ texture2D, typeName, texturePath });
        }
        return textures;
    }
};
