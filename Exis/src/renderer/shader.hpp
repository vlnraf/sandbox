#pragma once

#include "core/math.hpp"
#include <string>
#include <fstream>
#include <sstream>
#include <iostream>

#include "core/coreapi.hpp"
#include "core/arena.hpp"

//enum AssetType{
//    ASSET_NONE = 0,
//    ASSET_TEXTURE,
//    ASSET_AUDIO,
//    ASSET_SHADER,
//    ASSET_FONT
//
//};

// -------------------------------------------------------------------------------------------------
// Shader 
// -------------------------------------------------------------------------------------------------

struct Shader{
    unsigned int id;
};

CORE_API Shader loadShader(Arena* arena, char* vertexPath, char* fragmentPath);
CORE_API void unloadShader(Shader* shader);
CORE_API void setUniform(const Shader* shader, const char* name , const float value);
CORE_API void setUniform(const Shader* shader, const char* name , const bool value);
CORE_API void setUniform(const Shader* shader, const char* name , const int value);
CORE_API void setUniform(const Shader* shader, const char* name , const uint32_t value);
CORE_API void setUniform(const Shader* shader, const char* name , const Mat4 value);
CORE_API void setUniform(const Shader* shader, const char* name , const Vec3 value);
CORE_API void setUniform(const Shader* shader, const char* name , const Vec2 value);
CORE_API void setUniform(const Shader* shader, const char* name , const int* value, int size);

CORE_API void bindTextureToShader(const Shader* shader, const char* name, const uint32_t texture, int unit = 0);

void useShader(const Shader* shader);  // Internal: Use beginShaderMode() instead
void unBindShader();