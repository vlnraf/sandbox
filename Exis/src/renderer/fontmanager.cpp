#include "fontmanager.hpp"
#include "texture.hpp"
#include "core/tracelog.hpp"

#ifndef __EMSCRIPTEN__
#include <glad/glad.h>
#else
#include <GLES3/gl3.h>
#endif

#include <ft2build.h>
#include FT_FREETYPE_H  

FontManager* fontManager;

//TODO: refactor everything in this file
Font generateTextureFont(const char* filePath, int characterSize){ //Watch the function signature at top for default characterSize
    Font font = {};
    FT_Library ft;
    if (FT_Init_FreeType(&ft))
    {
       LOGERROR("ERROR::FREETYPE: Could not init FreeType Library");
        return font;
    }

    FT_Face face;
    if (FT_New_Face(ft, filePath, 0, &face))
    {
        LOGERROR("ERROR::FREETYPE: Failed to load font");
        return font;
    }
    FT_Set_Pixel_Sizes(face, 0, characterSize);

    if (FT_Load_Char(face, 'X', FT_LOAD_RENDER))
    {
        LOGERROR("ERROR::FREETYTPE: Failed to load Glyph");
        return font;
    }

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1); // disable byte-alignment restriction
    // generate texture
    font.ascender = face->size->metrics.ascender >> 6;
    font.descender = face->size->metrics.descender >> 6;
    font.maxHeight = font.ascender - font.descender;

    font.texture = loadFontTexture(filePath, face);

    // Rebind the texture to upload glyph data
    //Texture* fontTexture = getTextureByHandle(font.texture);
    glBindTexture(GL_TEXTURE_2D, font.texture.id);

    int xOffset = 0;

    for (unsigned char c = 0; c < 128; c++)
    {
        // load character glyph
        if (FT_Load_Char(face, c, FT_LOAD_RENDER))
        {
            LOGERROR("ERROR::FREETYTPE: Failed to load Glyph");
            continue;
        }

        // Upload glyph to the texture (skip if no bitmap data, e.g., space character)
        if (face->glyph->bitmap.buffer != nullptr && face->glyph->bitmap.width > 0) {
#ifdef __EMSCRIPTEN__
            // WebGL: convert single-channel glyph to RGBA (white with alpha)
            int pixelCount = face->glyph->bitmap.width * face->glyph->bitmap.rows;
            unsigned char* rgbaBuffer = (unsigned char*)malloc(pixelCount * 4);
            for (int i = 0; i < pixelCount; i++) {
                rgbaBuffer[i * 4 + 0] = 255; // R
                rgbaBuffer[i * 4 + 1] = 255; // G
                rgbaBuffer[i * 4 + 2] = 255; // B
                rgbaBuffer[i * 4 + 3] = face->glyph->bitmap.buffer[i]; // A
            }
            glTexSubImage2D(
                GL_TEXTURE_2D,
                0,
                xOffset,
                0,
                face->glyph->bitmap.width,
                face->glyph->bitmap.rows,
                GL_RGBA,
                GL_UNSIGNED_BYTE,
                rgbaBuffer
            );
            free(rgbaBuffer);
#else
            glTexSubImage2D(
                GL_TEXTURE_2D,
                0,
                xOffset,
                0,
                face->glyph->bitmap.width,
                face->glyph->bitmap.rows,
                GL_RED,
                GL_UNSIGNED_BYTE,
                face->glyph->bitmap.buffer
            );
#endif
        }
        Character character = {
            glm::ivec2(face->glyph->bitmap.width, face->glyph->bitmap.rows),
            glm::ivec2(face->glyph->bitmap_left, face->glyph->bitmap_top),
            (unsigned int)face->glyph->advance.x,
            xOffset // Store the xOffset for UV calculations
        };
        font.characters[c] = character;
        xOffset += face->glyph->bitmap.width;
    }

    FT_Done_Face(face);
    FT_Done_FreeType(ft);
    return font;
}

Font loadFont(const char* fileName, int characterSize){
    Font f;
    const char* fontPath = "assets/fonts/%s.%s";
    char fullPath[512];
    std::snprintf(fullPath, sizeof(fullPath), fontPath, fileName, "ttf");
    f = generateTextureFont(fullPath, characterSize);
    return f;
}

uint32_t calculateTextWidth(Font* font, const char* text, float scale){
    uint32_t result = 0;
    for(int i = 0; text[i] != '\0'; i++){
        Character ch = font->characters[(unsigned char) text[i]];
        result += (ch.advance >> 6) * scale; // bitshift by 6 to get value in pixels (2^6 = 64)
    }
    return result;
}