#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/string_cast.hpp>

// --- Types ---
using Vec2  = glm::vec2;
using Vec3  = glm::vec3;
using Vec4  = glm::vec4;
using IVec2 = glm::ivec2;
using Mat4  = glm::mat4;

// --- Geometry ---
inline Vec3  vec3Normalize(Vec3 v)                                           { return glm::normalize(v); }
inline Vec2  vec2Normalize(Vec2 v)                                           { return glm::normalize(v); }

// --- Matrix transforms ---
inline Mat4  mat4Ortho(float l, float r, float b, float t, float n, float f) { return glm::ortho(l, r, b, t, n, f); }
inline Mat4  mat4Translate(Mat4 m, Vec3 v)                                   { return glm::translate(m, v); }
inline Mat4  mat4Rotate(Mat4 m, float angle, Vec3 axis)                      { return glm::rotate(m, angle, axis); }
inline Mat4  mat4Scale(Mat4 m, Vec3 v)                                       { return glm::scale(m, v); }

// --- Scalar utils ---
inline float toRadians(float degrees)                                        { return glm::radians(degrees); }
inline float clampF(float v, float lo, float hi)                             { return glm::clamp(v, lo, hi); }
inline int   clampI(int v, int lo, int hi)                                   { return glm::clamp(v, lo, hi); }
inline float minF(float a, float b)                                          { return glm::min(a, b); }
inline float maxF(float a, float b)                                          { return glm::max(a, b); }
inline float floorF(float v)                                                 { return glm::floor(v); }
inline float absF(float v)                                                   { return glm::abs(v); }

// --- Interop ---
inline const float* mat4Ptr(const Mat4& m)                                   { return glm::value_ptr(m); }
inline const float* vec2Ptr(const Vec2& v)                                   { return glm::value_ptr(v); }
inline const float* vec3Ptr(const Vec3& v)                                   { return glm::value_ptr(v); }
inline const float* vec4Ptr(const Vec4& v)                                   { return glm::value_ptr(v); }
