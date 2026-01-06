#define NOMINMAX
#include "Vector3.h"
#include <imgui/imgui.h>
#include <math/Quaternion.h>

Vector3 Vector3::operator-() const
{
    return Vector3(-x, -y, -z);
}

// ベクトルの加算
Vector3 Vector3::operator+(const Vector3& other) const {
    return { x + other.x, y + other.y, z + other.z };
}

// ベクトルの減算
Vector3 Vector3::operator-(const Vector3& other) const {
    return { x - other.x, y - other.y, z - other.z };
}

// ベクトルのスカラー倍
Vector3 Vector3::operator*(float scalar) const {
    return { x * scalar, y * scalar, z * scalar };
}

// ベクトルのスカラー除算
Vector3 Vector3::operator/(float scalar) const {
    return { x / scalar, y / scalar, z / scalar };
}

// ベクトルの加算代入
Vector3& Vector3::operator+=(const Vector3& other) {
    x += other.x;
    y += other.y;
    z += other.z;
    return *this;
}

// ベクトルの減算代入
Vector3& Vector3::operator-=(const Vector3& other) {
    x -= other.x;
    y -= other.y;
    z -= other.z;
    return *this;
}

Vector3& Vector3::operator*=(const Vector3& other)
{
    x *= other.x;
    y *= other.y;
    z *= other.z;
    return *this;
}

// ベクトルのスカラー倍代入
Vector3& Vector3::operator*=(float scalar) {
    x *= scalar;
    y *= scalar;
    z *= scalar;
    return *this;
}

// ベクトルのスカラー除算代入
Vector3& Vector3::operator/=(float scalar) {
    x /= scalar;
    y /= scalar;
    z /= scalar;
    return *this;
}

// ベクトルの等価比較
bool Vector3::operator==(const Vector3& other) const {
    return x == other.x && y == other.y && z == other.z;
}

// ベクトルの非等価比較
bool Vector3::operator!=(const Vector3& other) const {
    return !(*this == other);
}

// ヘルパー関数
float Dot(const Vector3& v1, const Vector3& v2) {
    return v1.x * v2.x + v1.y * v2.y + v1.z * v2.z;
}

float Length(const Vector3& v) {
    return sqrtf(v.x * v.x + v.y * v.y + v.z * v.z);
}

Vector3 Normalize(const Vector3& v) {
    float len = Length(v);
    return { v.x / len, v.y / len, v.z / len };
}

Vector3 RotateVector(const Vector3& v, const Quaternion& q) {
    // q * v * q^-1
    Quaternion vq = { v.x, v.y, v.z, 0.0f };
    Quaternion qInv = Inverse(q);
    Quaternion result = q * vq * qInv;
    return { result.x, result.y, result.z };
}

Vector3 Lerp(const Vector3& start, const Vector3& end, float t) {
    return start * (1.0f - t) + end * t;
}

void PrintOnImGui(const Vector3& v, const char* label)
{
    ImGui::Begin(label);
    ImGui::Text("%s: (x: %.2f, y: %.2f, z: %.2f)", "Vector3", v.x, v.y, v.z);
    ImGui::End();
}

Vector3 Min(const Vector3& a, const Vector3& b)
{
    return {
    std::min(a.x, b.x),
    std::min(a.y, b.y),
    std::min(a.z, b.z)
    };
}

Vector3 Max(const Vector3& a, const Vector3& b)
{
    return Vector3();
}
