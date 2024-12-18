#pragma once

#include <string>
#include <vector>
#include <iostream>

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include "file_system.hpp"
#include "mesh.hpp"
#include "shader.hpp"
#include "texture2D.hpp"

class Model
{
public:
    // Constructor
    Model(const std::string& path)
    {
        loadModel(path);
    }

    // Render the model
    void Draw()
    {
        for (auto mesh : meshes)
            mesh.Draw();
    }

    void TextureOverride(const std::string& texturePath, const bool alpha = false)
    {
        Texture2D texture2D(FileSystem::GetPath(texturePath), alpha);
        Texture texture({texture2D, "texture_diffuse", texturePath});
        for (auto& mesh : meshes)
            mesh.AddTexture(texture);
    }

private:
    std::vector<Mesh> meshes;
    std::string directory;

    // Load a model from file using Assimp
    void loadModel(const std::string& path)
    {
        Assimp::Importer importer;
        const aiScene* scene = importer.ReadFile(path,
           aiProcessPreset_TargetRealtime_MaxQuality | aiProcess_FlipUVs);

        if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
        {
            std::cerr << "Error::Assimp: " << importer.GetErrorString() << std::endl;
            return;
        }

        directory = path.substr(0, path.find_last_of('/'));
        processNode(scene->mRootNode, scene);
    }

    // Recursively process nodes in the Assimp scene
    void processNode(aiNode* node, const aiScene* scene)
    {
        for (unsigned int i = 0; i < node->mNumMeshes; i++)
        {
            aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
            meshes.push_back(processMesh(mesh, scene));
        }

        for (unsigned int i = 0; i < node->mNumChildren; i++)
        {
            processNode(node->mChildren[i], scene);
        }
    }

    // Process an individual Assimp mesh and convert to our Mesh class
    Mesh processMesh(aiMesh* mesh, const aiScene* scene)
    {
        std::vector<Vertex> vertices;
        std::vector<GLuint> indices;
        std::vector<Texture> textures;

        // Process vertices
        for (unsigned int i = 0; i < mesh->mNumVertices; i++)
        {
            Vertex vertex;
            glm::vec3 position;
            glm::vec3 normal;
            glm::vec2 texCoords;

            position.x = mesh->mVertices[i].x;
            position.y = mesh->mVertices[i].y;
            position.z = mesh->mVertices[i].z;

            normal.x = mesh->mNormals[i].x;
            normal.y = mesh->mNormals[i].y;
            normal.z = mesh->mNormals[i].z;

            if (mesh->mTextureCoords[0])
            {
                texCoords.x = mesh->mTextureCoords[0][i].x;
                texCoords.y = mesh->mTextureCoords[0][i].y;
            }
            else
            {
                texCoords = glm::vec2(0.0f, 0.0f);
            }

            vertex.Position = position;
            vertex.Normal = normal;
            vertex.TexCoords = texCoords;

            vertices.push_back(vertex);
        }

        // Process indices
        for (unsigned int i = 0; i < mesh->mNumFaces; i++)
        {
            aiFace face = mesh->mFaces[i];
            for (unsigned int j = 0; j < face.mNumIndices; j++)
            {
                indices.push_back(face.mIndices[j]);
            }
        }

        // Process materials
        if (mesh->mMaterialIndex >= 0)
        {
            aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];
            std::vector<Texture> diffuseMaps = loadMaterialTextures(material, aiTextureType_DIFFUSE, "texture_diffuse");
            textures.insert(textures.end(), diffuseMaps.begin(), diffuseMaps.end());

            std::vector<Texture> specularMaps = loadMaterialTextures(material, aiTextureType_SPECULAR, "texture_specular");
            textures.insert(textures.end(), specularMaps.begin(), specularMaps.end());
        }

        return Mesh(vertices, indices, textures);
    }

    std::vector<Texture> loadMaterialTextures(aiMaterial* mat, aiTextureType type, const std::string& typeName)
    {
        std::vector<Texture> textures;
        for (unsigned int i = 0; i < mat->GetTextureCount(type); i++)
        {
            aiString str;
            mat->GetTexture(type, i, &str);

            std::string texturePath = directory + "/" + std::string(str.C_Str());

            // Use Texture2D to load the texture
            Texture2D texture2D(FileSystem::GetPath(texturePath));

            textures.push_back({ texture2D, typeName, texturePath });
        }
        return textures;
    }
};
