#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/string_cast.hpp>
#include <glm/gtc/quaternion.hpp>

// --- Types ---
using Vec2  = glm::vec2;
using Vec3  = glm::vec3;
using Vec4  = glm::vec4;
using IVec2 = glm::ivec2;
using Mat4  = glm::mat4;
using Quat  = glm::quat;

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

// --- math* wrappers (camelCase free functions) ---

// Geometry
inline Vec3  mathNormalize(Vec3 v)                                           { return glm::normalize(v); }
inline Vec2  mathNormalize(Vec2 v)                                           { return glm::normalize(v); }
inline float mathDot(Vec3 a, Vec3 b)                                         { return glm::dot(a, b); }
inline float mathDot(Vec2 a, Vec2 b)                                         { return glm::dot(a, b); }
inline Vec3  mathCross(Vec3 a, Vec3 b)                                       { return glm::cross(a, b); }
inline float mathLength(Vec3 v)                                              { return glm::length(v); }
inline float mathLength(Vec2 v)                                              { return glm::length(v); }

// Matrix transforms
inline Mat4  mathTranslate(Mat4 m, Vec3 v)                                   { return glm::translate(m, v); }
inline Mat4  mathRotate(Mat4 m, float angle, Vec3 axis)                      { return glm::rotate(m, angle, axis); }
inline Mat4  mathScale(Mat4 m, Vec3 v)                                       { return glm::scale(m, v); }
inline Mat4  mathPerspective(float fovy, float aspect, float zNear, float zFar){ return glm::perspective(fovy, aspect, zNear, zFar); }
inline Mat4  mathLookAt(Vec3 eye, Vec3 center, Vec3 up)                      { return glm::lookAt(eye, center, up); }

// Interop
inline const float* mathValuePtr(const Mat4& m)                              { return glm::value_ptr(m); }
inline const float* mathValuePtr(const Vec2& v)                              { return glm::value_ptr(v); }
inline const float* mathValuePtr(const Vec3& v)                              { return glm::value_ptr(v); }
inline const float* mathValuePtr(const Vec4& v)                              { return glm::value_ptr(v); }

// Scalar utils
inline float mathRadians(float deg)                                          { return glm::radians(deg); }
inline float mathDegrees(float rad)                                          { return glm::degrees(rad); }
inline float mathMix(float a, float b, float t)                              { return glm::mix(a, b, t); }
inline Vec2  mathMix(Vec2 a, Vec2 b, float t)                                { return glm::mix(a, b, t); }
inline Vec3  mathMix(Vec3 a, Vec3 b, float t)                                { return glm::mix(a, b, t); }
inline float mathClamp(float v, float lo, float hi)                          { return glm::clamp(v, lo, hi); }
inline int   mathClamp(int v, int lo, int hi)                                { return glm::clamp(v, lo, hi); }
inline float mathLerp(float a, float b, float t)                             { return glm::mix(a, b, t); }
inline Vec2  mathLerp(Vec2 a, Vec2 b, float t)                               { return glm::mix(a, b, t); }
inline Vec3  mathLerp(Vec3 a, Vec3 b, float t)                               { return glm::mix(a, b, t); }
