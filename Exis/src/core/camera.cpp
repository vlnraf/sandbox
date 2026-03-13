#include "camera.hpp"
#include "renderer/renderer.hpp"

#include "math.hpp"

//OrtographicCamera* activeCamera;

OrtographicCamera createCamera(float left, float right, float bottom, float top){
    OrtographicCamera camera = {};
    camera.position = Vec3(0.0f, 0.0f, 0.0f);
    camera.width = right - left;
    camera.height = top - bottom;
    //camera.aspectRatio = camera.width / camera.height;

    camera.projection = mat4Ortho(left, right, bottom, top, -100.0f, 100.0f);
    camera.view = mat4Translate(Mat4(1.0f), -camera.position);

    return camera;
}

void setProjection(OrtographicCamera* camera, float left, float right, float bottom, float top){
    camera->width = right - left;
    camera->height = top - bottom;
    camera->projection = mat4Ortho(left, right, bottom, top, -100.0f, 100.0f);
}

void updateCameraAspectRatio(OrtographicCamera* camera, float viewportWidth, float viewportHeight){
    // Keep vertical height constant, adjust horizontal width based on aspect ratio
    float verticalSize = camera->height;  // This was set in createCamera
    float aspectRatio = viewportWidth / viewportHeight;

    float halfHeight = verticalSize / 2.0f;
    float halfWidth = halfHeight * aspectRatio;

    // Update projection to maintain aspect ratio
    camera->width = halfWidth * 2.0f;
    camera->projection = mat4Ortho(-halfWidth, halfWidth, -halfHeight, halfHeight, -100.0f, 100.0f);
}

void setPosition(OrtographicCamera* camera, const Vec3& position){
    camera->position = position;
    camera->view = mat4Translate(Mat4(1.0f), -camera->position);
}

void followTarget(OrtographicCamera* camera, const Vec3 targetPos){
    // For Hazel-style centered cameras: just set position to target
    // The centered projection automatically centers the view on this position
    camera->position = targetPos;
    camera->view = mat4Translate(Mat4(1.0f), -camera->position);
}

Vec2 worldToScreen(const OrtographicCamera& camera, const Vec3& worldPos) {
    // Compute the combined VP matrix
    Mat4 vp = camera.projection * camera.view;

    // Transform the world position to clip space
    Vec4 clipSpacePos = vp * Vec4(worldPos, 1.0f);

    // Perform perspective division to get NDC (normalized device coordinates)
    Vec3 ndc = Vec3(clipSpacePos) / clipSpacePos.w;

    Vec2 screenSize = getScreenSize();

    // Convert NDC to screen space
    float screenX = (ndc.x * 0.5f + 0.5f) * screenSize.x;
    float screenY = (ndc.y * 0.5f + 0.5f) * screenSize.y;
    screenY = screenSize.y - screenY;

    return Vec2(screenX, screenY);
}

Vec2 worldToScreen(const OrtographicCamera& camera, const Vec2& worldPos) {
    return worldToScreen(camera, Vec3(worldPos, 0.0f));
}

Vec2 convertScreenCoords(Vec2 pos, Vec2 size, Vec2 screenSize){
    Vec2 screenPos = {pos.x, screenSize.y - (pos.y + size.y)};
    return screenPos;
}


Vec2 screenToWorld(const OrtographicCamera& camera, const Vec2& screenSize, const Vec2& screenPos){
    // Convert from top-left origin to bottom-left origin
    Vec2 flipped = {screenPos.x, screenPos.y};

    // Normalize to [0, 1]
    Vec2 normalized = flipped / screenSize;

    // Scale to camera world size
    Vec2 worldPos = (normalized - 0.5f) * Vec2(camera.width, camera.height);

    // Offset by camera position 
    worldPos += Vec2(camera.position.x, camera.position.y);

    return worldPos;
}

//void setActiveCamera(OrtographicCamera* camera){
//    activeCamera = camera;
//}
//
//OrtographicCamera* getActiveCamera(){
//    return activeCamera;
//}