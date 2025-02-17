#pragma once

#include "mesh.hpp"
#include "shader.hpp"
#include "texture_2D.hpp"

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

static inline glm::vec3 AiToGlmVec3(const aiVector3D& vec) { return glm::vec3(vec.x, vec.y, vec.z); }

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

    void TextureOverride(const std::string& texturePath)
    {
        Texture2D texture2D(texturePath);
        Texture texture({ texture2D, "texture_diffuse" });

        for (auto& mesh : meshes)
            mesh.AddTexture(texture);
    }

private:
    std::vector<Mesh> meshes;
    std::string directory;
    std::vector<Texture> cachedTextures;

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

    Mesh processMesh(const aiMesh* mesh, const aiScene* scene)
    {
        std::vector<Vertex> vertices;
        std::vector<GLuint> indices;
        std::vector<Texture> textures;

        // Process vertices
        for (unsigned int i = 0; i < mesh->mNumVertices; ++i)
        {
            vertices.emplace_back(Vertex{
                .Position    = AiToGlmVec3(mesh->mVertices[i]),
                .Normal      = AiToGlmVec3(mesh->mNormals[i]),
                .TexCoords   = mesh->mTextureCoords[0]
                                ? glm::vec2(mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y)
                                : glm::vec2(0.0f, 0.0f),
                .BoneIDs     = glm::ivec4(-1),
                .BoneWeights = glm::vec4(0.0f) // Bone Weights are initialised in another loop
            });
        }

        // Process indices
        for (unsigned int i = 0; i < mesh->mNumFaces; ++i)
        {
            const aiFace& face = mesh->mFaces[i];
            for (unsigned int j = 0; j < face.mNumIndices; ++j)
                indices.emplace_back(face.mIndices[j]);
        }

        // Process textures
        if (mesh->mMaterialIndex >= 0)
        {
            aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];

            auto diffuseTextures = ExtractTextures(scene, material, aiTextureType_DIFFUSE, "texture_diffuse");
            textures.insert(textures.end(), diffuseTextures.begin(), diffuseTextures.end());
            auto specularTextures = ExtractTextures(scene, material, aiTextureType_SPECULAR, "texture_specular");
            textures.insert(textures.end(), specularTextures.begin(), specularTextures.end());
            auto normalTextures = ExtractTextures(scene, material, aiTextureType_HEIGHT, "texture_normal");
            textures.insert(textures.end(), normalTextures.begin(), normalTextures.end());
        }
        return Mesh(std::move(vertices), std::move(indices), std::move(textures));
    }

    // Load material textures from Assimp
    std::vector<Texture> ExtractTextures(const aiScene* scene, aiMaterial* material, aiTextureType textureType, const std::string& typeName)
    {
        std::vector<Texture> textures;
        for (unsigned int i = 0; i < material->GetTextureCount(textureType); ++i)
        {
            aiString textureFilename;
            material->GetTexture(textureType, i, &textureFilename);

            // check if texture was loaded before and if so, continue to next iteration: skip loading a new texture
            bool skip = false;
            for (unsigned int j = 0; j < cachedTextures.size(); ++j)
            {
                if (std::strcmp(cachedTextures[j].path.data(), textureFilename.C_Str()) == 0)
                {
                    textures.emplace_back(cachedTextures[j]);
                    skip = true; // a texture with the same filepath has already been loaded, continue to next one. (optimization)
                    break;
                }
            }

            if (!skip) // if texture hasn't been loaded already, load it
            {
                Texture2D texture2D;
                if (const auto& texture = scene->GetEmbeddedTexture(textureFilename.C_Str()))
                    texture2D = Texture2D(reinterpret_cast<unsigned char*>(texture->pcData), texture->mWidth, texture->mHeight);
                else
                    texture2D = Texture2D(std::string(directory + "/" + std::string(textureFilename.C_Str())));

                Texture tex = { texture2D, typeName, std::string(textureFilename.C_Str()) };
                textures.push_back(tex);
                cachedTextures.push_back(tex);
            }
        }
        return textures;
    }
};
