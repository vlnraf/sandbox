#pragma once

#include <stdint.h>
#include <vector>
#include <glm/glm.hpp>

#include <ft2build.h>
#include FT_FREETYPE_H  

#include "core/arena.hpp"
#include "core/coreapi.hpp"

#define MAX_TEXTURES 2056

typedef uint32_t TextureHandle;

enum TextureFormat{
    TEXTURE_R8,
    TEXTURE_RG8,
    TEXTURE_RGB,
    TEXTURE_RGBA,
    TEXTURE_R32F,
    TEXTURE_RG32F,
    TEXTURE_RGB32F,
    TEXTURE_RGBA32F
};

enum TextureFilter{
    TEXTURE_FILTER_NEAREST,
    TEXTURE_FILTER_LINEAR
};

enum TextureWrap{
    TEXTURE_WRAP_REPEAT,
    TEXTURE_WRAP_CLAMP_TO_EDGE,
    TEXTURE_WRAP_MIRRORED_REPEAT
};

struct Texture{
    uint32_t id;
    int width, height, nrChannels;

    //glm::vec2 index;
    glm::vec2 size;
};

struct RenderTexture{
    uint32_t fbo;
    uint32_t rbo;
    Texture texture;
};

struct TextureManager{
    Arena* arena;
    //std::vector<Texture*> textures;
    Texture textures[MAX_TEXTURES];
};

CORE_API Texture loadTexture(const char* fileName);
CORE_API Texture loadTextureFullPath(const char* path);
CORE_API RenderTexture loadRenderTexture(int width, int height, uint16_t format = TEXTURE_RGBA);
CORE_API void destroyRenderTexture(RenderTexture* renderTexture);


CORE_API void getImageFromTexture(void* image, Texture* texture, uint16_t format = TEXTURE_RGBA);
CORE_API void setImageToTexture(void* image, Texture* texture, uint16_t format = TEXTURE_RGBA);
Texture loadFontTexture(const char* path, FT_Face face);
Texture getWhiteTexture();