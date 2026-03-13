#pragma once

#include "math.hpp"
#include "coreapi.hpp"

struct OrtographicCamera{
    Vec3 position;
    Vec3 target;
    float width, height; //Resolution
    float aspectRatio;
    Mat4 projection;
    Mat4 view;

};

CORE_API OrtographicCamera createCamera(float left, float right, float bottom, float top);
CORE_API void setProjection(OrtographicCamera* camera, float left, float right, float bottom, float top);
CORE_API void updateCameraAspectRatio(OrtographicCamera* camera, float viewportWidth, float viewportHeight);
CORE_API void setPosition(OrtographicCamera* camera, const Vec3& position);
CORE_API void followTarget(OrtographicCamera* camera, const Vec3 targetPos);
CORE_API Vec2 worldToScreen(const OrtographicCamera& camera, const Vec3& worldPos);
CORE_API Vec2 worldToScreen(const OrtographicCamera& camera, const Vec2& worldPos);
CORE_API Vec2 convertScreenCoords(Vec2 pos, Vec2 size, Vec2 screenSize);
CORE_API Vec2 screenToWorld(const OrtographicCamera& camera, const Vec2& screenSize, const Vec2& screenPos);
CORE_API void setActiveCamera(OrtographicCamera* camera);
CORE_API OrtographicCamera* getActiveCamera();