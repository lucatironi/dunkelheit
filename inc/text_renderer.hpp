#pragma once

#include "shader.hpp"

#include <glad/gl.h>
#include <glm/glm.hpp>
#include <ft2build.h>
#include FT_FREETYPE_H

#include <map>
#include <vector>
#include <string>
#include <algorithm>

struct Character {
    glm::ivec2 Size;      // Size of glyph
    glm::ivec2 Bearing;   // Offset from baseline to left/top of glyph
    unsigned int Advance; // Offset to advance to next glyph
    float txLower;        // X-axis texture start (UV)
    float txUpper;        // X-axis texture end (UV)
    float tyLower;        // Y-axis texture start (UV)
    float tyUpper;        // Y-axis texture end (UV)
};

class TextRenderer
{
public:
    TextRenderer(const std::string& fontPath, const int fontSize)
    {
        loadFont(fontPath, fontSize);
        initRenderData();
    }

    ~TextRenderer()
    {
        glDeleteTextures(1, &atlasTexture);
        glDeleteVertexArrays(1, &VAO);
        glDeleteBuffers(1, &VBO);
    }

    void RenderText(const std::string& text, const Shader& shader, float x, float y, float scale, glm::vec3 color)
    {
        shader.Use();
        shader.SetVec3("textColor", color);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, atlasTexture);
        glBindVertexArray(VAO);

        // We build a single vertex buffer for the entire string
        std::vector<float> vertices;
        vertices.reserve(text.size() * 6 * 4);

        for (auto c = text.begin(); c != text.end(); ++c)
        {
            Character ch = characters[*c];

            float xpos = x + ch.Bearing.x * scale;
            float ypos = y - (ch.Size.y - ch.Bearing.y) * scale;

            float w = ch.Size.x * scale;
            float h = ch.Size.y * scale;

            // Six vertices per character (two triangles)
            // Each vertex: x, y, u, v
            float quad[] = {
                xpos,     ypos + h,   ch.txLower, ch.tyLower,
                xpos,     ypos,       ch.txLower, ch.tyUpper,
                xpos + w, ypos,       ch.txUpper, ch.tyUpper,

                xpos,     ypos + h,   ch.txLower, ch.tyLower,
                xpos + w, ypos,       ch.txUpper, ch.tyUpper,
                xpos + w, ypos + h,   ch.txUpper, ch.tyLower
            };

            vertices.insert(vertices.end(), quad, quad + 24);

            // Advance cursor for next glyph
            x += (ch.Advance >> 6) * scale;
        }

        // Upload the entire string's vertex data at once
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_DYNAMIC_DRAW);

        // ONE draw call for the whole string
        glDrawArrays(GL_TRIANGLES, 0, (GLsizei)(vertices.size() / 4));

        glBindVertexArray(0);
        glBindTexture(GL_TEXTURE_2D, 0);
    }

    void BeginBatch()
    {
        batchVertices.clear();
    }

    void AddText(const std::string& text, float x, float y, float scale)
    {
        for (auto c = text.begin(); c != text.end(); ++c)
        {
            Character ch = characters[*c];
            float xpos = x + ch.Bearing.x * scale;
            float ypos = y - (ch.Size.y - ch.Bearing.y) * scale;
            float w = ch.Size.x * scale;
            float h = ch.Size.y * scale;

            float quad[] = {
                xpos,     ypos + h,   ch.txLower, ch.tyLower,
                xpos,     ypos,       ch.txLower, ch.tyUpper,
                xpos + w, ypos,       ch.txUpper, ch.tyUpper,
                xpos,     ypos + h,   ch.txLower, ch.tyLower,
                xpos + w, ypos,       ch.txUpper, ch.tyUpper,
                xpos + w, ypos + h,   ch.txUpper, ch.tyLower
            };
            batchVertices.insert(batchVertices.end(), quad, quad + 24);
            x += (ch.Advance >> 6) * scale;
        }
    }

    void FlushBatch(const Shader& shader, glm::vec3 color)
    {
        if (batchVertices.empty()) return;

        shader.Use();
        shader.SetVec3("textColor", color);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, atlasTexture);
        glBindVertexArray(VAO);

        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, batchVertices.size() * sizeof(float), batchVertices.data(), GL_DYNAMIC_DRAW);

        glDrawArrays(GL_TRIANGLES, 0, (GLsizei)(batchVertices.size() / 4));

        glBindVertexArray(0);
    }

private:
    std::map<char, Character> characters;
    unsigned int VAO, VBO, atlasTexture;
    int atlasWidth = 0;
    int atlasHeight = 0;
    std::vector<float> batchVertices;

    void loadFont(const std::string& path, const int size)
    {
        FT_Library ft;
        if (FT_Init_FreeType(&ft)) return;

        FT_Face face;
        if (FT_New_Face(ft, path.c_str(), 0, &face)) return;

        FT_Set_Pixel_Sizes(face, 0, size);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

        // 1. First pass: Determine total atlas dimensions
        for (unsigned char c = 0; c < 128; c++)
        {
            if (FT_Load_Char(face, c, FT_LOAD_RENDER)) continue;
            atlasWidth += face->glyph->bitmap.width;
            atlasHeight = std::max(atlasHeight, (int)face->glyph->bitmap.rows);
        }

        // 2. Create the empty Atlas texture
        glGenTextures(1, &atlasTexture);
        glBindTexture(GL_TEXTURE_2D, atlasTexture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, atlasWidth, atlasHeight, 0, GL_RED, GL_UNSIGNED_BYTE, nullptr);

        // 3. Second pass: Fill the atlas and store UVs
        int xOffset = 0;
        for (unsigned char c = 0; c < 128; c++)
        {
            if (FT_Load_Char(face, c, FT_LOAD_RENDER)) continue;

            // Upload individual glyph into the atlas
            glTexSubImage2D(GL_TEXTURE_2D, 0, xOffset, 0, face->glyph->bitmap.width, face->glyph->bitmap.rows,
                            GL_RED, GL_UNSIGNED_BYTE, face->glyph->bitmap.buffer);

            // Calculate UVs
            float txLower = (float)xOffset / (float)atlasWidth;
            float txUpper = (float)(xOffset + face->glyph->bitmap.width) / (float)atlasWidth;
            float tyLower = 0.0f;
            float tyUpper = (float)face->glyph->bitmap.rows / (float)atlasHeight;

            Character character = {
                glm::ivec2(face->glyph->bitmap.width, face->glyph->bitmap.rows),
                glm::ivec2(face->glyph->bitmap_left, face->glyph->bitmap_top),
                static_cast<unsigned int>(face->glyph->advance.x),
                txLower, txUpper, tyLower, tyUpper
            };
            characters.insert(std::pair<char, Character>(c, character));

            xOffset += face->glyph->bitmap.width;
        }

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

        FT_Done_Face(face);
        FT_Done_FreeType(ft);
    }

    void initRenderData()
    {
        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);
        glBindVertexArray(VAO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);

        // Setup vertex attributes (x, y, u, v)
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);

        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
    }
};
