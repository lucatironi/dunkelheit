#pragma once

#include <glm/glm.hpp>
#include <nlohmann/json.hpp>

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

// Specialization for glm::vec3
namespace nlohmann {
    template <>
    struct adl_serializer<glm::vec3>
    {
        // Serialize glm::vec3 to JSON
        static void to_json(json& j, const glm::vec3& vec)
        {
            j = json{ vec.x, vec.y, vec.z };
        }

        // Deserialize glm::vec3 from JSON
        static void from_json(const json& j, glm::vec3& vec)
        {
            if (!j.is_array() || j.size() != 3)
                throw std::runtime_error("Invalid JSON format for glm::vec3");

            vec.x = j[0].get<float>();
            vec.y = j[1].get<float>();
            vec.z = j[2].get<float>();
        }
    };
}

class JsonFile
{
public:
    // Delete copy constructor and assignment operator to enforce singleton
    JsonFile(const JsonFile&) = delete;
    JsonFile& operator=(const JsonFile&) = delete;

    // Access the singleton instance
    static JsonFile& GetInstance()
    {
        static JsonFile instance;
        return instance;
    }

    void Load(const std::string& path)
    {
        std::ifstream file(path);
        if (!file.is_open())
            throw std::ifstream::failure("Failed to open file: " + path);

        try
        {
            file >> jsonFile;
        }
        catch (const nlohmann::json::exception& e)
        {
            throw std::runtime_error("Error parsing config file: " + std::string(e.what()));
        }
    }

    template <typename T>
    T Get(const std::string& key) const
    {
        try
        {
            return jsonFile.at(key).get<T>();
        }
        catch (const nlohmann::json::exception& e)
        {
            throw std::runtime_error("Error retrieving key \"" + key + "\": " + std::string(e.what()));
        }
    }

    template <typename T>
    T GetNested(const std::string& nestedKey) const
    {
        try
        {
            auto keys = split(nestedKey, '.');
            nlohmann::json current = jsonFile;

            for (const auto& key : keys)
                current = current.at(key);

            return current.get<T>();
        }
        catch (const nlohmann::json::exception& e)
        {
            throw std::runtime_error("Error retrieving nested key \"" + nestedKey + "\": " + std::string(e.what()));
        }
    }

private:
    JsonFile() = default;
    nlohmann::json jsonFile;

    std::vector<std::string> split(const std::string& str, char delimiter) const
    {
        std::vector<std::string> tokens;
        std::string token;
        std::istringstream tokenStream(str);
        while (std::getline(tokenStream, token, delimiter))
            tokens.push_back(token);

        return tokens;
    }
};