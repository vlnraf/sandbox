#ifndef __EMSCRIPTEN__
#include <glad/glad.h>
#else
#include <GLES3/gl3.h>
#endif

#include "texture.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include "core/tracelog.hpp"
#include "renderer.hpp"


//Texture* loadSubTexture(const char* filepath, glm::vec2 index, glm::vec2 size);
Texture createTexture(const char* filepath);
Texture getWhiteTexture();

Texture loadTextureFullPath(const char* path){
    //uint32_t hash = hashTextureName(path);
    //if(textureManager->textures[hash]){ //NOTE: free the memory of the old texture
        //delete textureManager->textures[hash];
    //}
    Texture t = createTexture(path);
    return t;
}

Texture loadTexture(const char* fileName){
    const char* assetsPath = "assets/sprites/%s.%s";
    char fullPath[512];
    std::snprintf(fullPath, sizeof(fullPath), assetsPath, fileName, "png");

    Texture t = createTexture(fullPath);
    return t;
}

unsigned char* loadImage(const char* filePath, Texture* texture){
    return stbi_load(filePath, &texture->width, &texture->height, &texture->nrChannels, 0);
}
Texture createTexture(const char* filePath){
    Texture texture = {};
    //stbi_set_flip_vertically_on_load(true);
    unsigned char* data = loadImage(filePath, &texture);

    if(data){
        GLenum format;
        switch(texture.nrChannels){
            case 3:
                format = GL_RGB;
                break;
            case 4:
                format = GL_RGBA;
                break;
        }

        genTexture(&texture, format, data);
        stbi_image_free(data);
        texture.size = {texture.width, texture.height};
        LOGINFO("Texture loaded: %s (id=%u, %dx%d, ch=%d)", filePath, texture.id, texture.width, texture.height, texture.nrChannels);
    }else{
        LOGERROR("Failed to load texture: %s", filePath);
    }

    return texture;
}

Texture getWhiteTexture(){
    Texture whiteTexture = {};
    static uint8_t white[4] = {255, 255, 255, 255};
    whiteTexture.width = 1;
    whiteTexture.height = 1;
    whiteTexture.nrChannels = 4;
    genTexture(&whiteTexture, GL_RGBA, (unsigned char*)white);
    whiteTexture.size = {whiteTexture.width, whiteTexture.height};

    return whiteTexture;
}

Texture loadFontTexture(const char* path, FT_Face face){
    Texture texture = {};
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1); // disable byte-alignment restriction
    glGenTextures(1, &texture.id);
    glBindTexture(GL_TEXTURE_2D, texture.id);
    for (unsigned char c = 0; c < 128; c++){
        if (FT_Load_Char(face, c, FT_LOAD_RENDER)) {
            continue; // Skip failed glyphs
        }
        texture.width += face->glyph->bitmap.width;
        texture.height = std::max(texture.height, (int)face->glyph->bitmap.rows);
    }

#ifdef __EMSCRIPTEN__
    // WebGL doesn't support texture swizzling, so we use RGBA texture
    // with white color (RGB=255) and alpha from the glyph
    texture.nrChannels = 4;
    glTexImage2D(
        GL_TEXTURE_2D,
        0,
        GL_RGBA,
        texture.width,
        texture.height,
        0,
        GL_RGBA,
        GL_UNSIGNED_BYTE,
        nullptr
    );
#else
    texture.nrChannels = 1;
    glTexImage2D(
        GL_TEXTURE_2D,
        0,
        GL_R8,
        texture.width,
        texture.height,
        0,
        GL_RED,
        GL_UNSIGNED_BYTE,
        nullptr
    );
    // Desktop OpenGL: use swizzle to map R channel to alpha
    GLint swizzle[] = {GL_ONE, GL_ONE, GL_ONE, GL_RED};
    glTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_RGBA, swizzle);
#endif
    // set texture options
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);

    return texture;
}

RenderTexture loadRenderTexture(int width, int height, uint16_t format){
    RenderTexture result = {};
    result.texture.width = width;
    result.texture.height = height;
    result.texture.size = {width, height};
    genFrameBuffer(&result.fbo);
    genRenderBuffer(&result.rbo);
    genRenderTexture(&result.texture.id, result.texture.width, result.texture.height, format);

    // Attach texture and renderbuffer to FBO once at creation
    bindFrameBuffer(result.fbo);
    attachFrameBuffer(result.texture.id);
    bindRenderBuffer(result.rbo);
    attachRenderBuffer(result.fbo, result.texture.width, result.texture.height);
    unbindFrameBuffer();

    return result;
}

void destroyRenderTexture(RenderTexture* renderTexture){
    if(renderTexture->texture.id != 0){
        deleteTexture(renderTexture->texture.id);
        renderTexture->texture.id = 0;
    }
    if(renderTexture->fbo != 0){
        deleteFrameBuffer(renderTexture->fbo);
        renderTexture->fbo = 0;
    }
    if(renderTexture->rbo != 0){
        deleteRenderBuffer(renderTexture->rbo);
        renderTexture->rbo = 0;
    }
}

#ifndef __EMSCRIPTEN__
void getImageFromTexture(void* image, Texture* texture, uint16_t format){
    GLenum internalFormat, pixelFormat, texType;
    toGLFormat(format, &internalFormat, &pixelFormat, &texType);
    glBindTexture(GL_TEXTURE_2D, texture->id);
    glGetTexImage(GL_TEXTURE_2D, 0, pixelFormat, texType, image);
}

void setImageToTexture(void* image, Texture* texture, uint16_t format){
    GLenum internalFormat, pixelFormat, texType;
    toGLFormat(format, &internalFormat, &pixelFormat, &texType);
    glBindTexture(GL_TEXTURE_2D, texture->id);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, texture->width, texture->height, pixelFormat, texType, image);
}
#endif