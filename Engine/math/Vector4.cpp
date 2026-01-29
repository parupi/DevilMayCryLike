#include "Vector4.h"
#ifdef USE_IMGUI
#include <imgui.h>
#endif // IMGUI
#include "math/Matrix4x4.h" // 必要なMatrix4x4の定義を含むヘッダーをインクルード

// ベクトルの加算
Vector4 Vector4::operator+(const Vector4& other) const {
    return { x + other.x, y + other.y, z + other.z, w + other.w };
}

// ベクトルの減算
Vector4 Vector4::operator-(const Vector4& other) const {
    return { x - other.x, y - other.y, z - other.z, w - other.w };
}

// ベクトルのスカラー倍
Vector4 Vector4::operator*(float scalar) const {
    return { x * scalar, y * scalar, z * scalar, w * scalar };
}

// ベクトルのスカラー除算
Vector4 Vector4::operator/(float scalar) const {
    return { x / scalar, y / scalar, z / scalar, w / scalar };
}

// ベクトルの加算代入
Vector4& Vector4::operator+=(const Vector4& other) {
    x += other.x;
    y += other.y;
    z += other.z;
    w += other.w;
    return *this;
}

// ベクトルの減算代入
Vector4& Vector4::operator-=(const Vector4& other) {
    x -= other.x;
    y -= other.y;
    z -= other.z;
    w -= other.w;
    return *this;
}

// ベクトルのスカラー倍代入
Vector4& Vector4::operator*=(float scalar) {
    x *= scalar;
    y *= scalar;
    z *= scalar;
    w *= scalar;
    return *this;
}

// ベクトルのスカラー除算代入
Vector4& Vector4::operator/=(float scalar) {
    x /= scalar;
    y /= scalar;
    z /= scalar;
    w /= scalar;
    return *this;
}

// ベクトルの等価比較
bool Vector4::operator==(const Vector4& other) const {
    return x == other.x && y == other.y && z == other.z && w == other.w;
}

// ベクトルの非等価比較
bool Vector4::operator!=(const Vector4& other) const {
    return !(*this == other);
}

// ベクトルと行列の積
Vector4 operator*(const Vector4& vec, const Matrix4x4& mat) {
    return {
        vec.x * mat.m[0][0] + vec.y * mat.m[1][0] + vec.z * mat.m[2][0] + vec.w * mat.m[3][0],
        vec.x * mat.m[0][1] + vec.y * mat.m[1][1] + vec.z * mat.m[2][1] + vec.w * mat.m[3][1],
        vec.x * mat.m[0][2] + vec.y * mat.m[1][2] + vec.z * mat.m[2][2] + vec.w * mat.m[3][2],
        vec.x * mat.m[0][3] + vec.y * mat.m[1][3] + vec.z * mat.m[2][3] + vec.w * mat.m[3][3]
    };
}

// ヘルパー関数
float Dot(const Vector4& v1, const Vector4& v2) {
    return v1.x * v2.x + v1.y * v2.y + v1.z * v2.z + v1.w * v2.w;
}

float Length(const Vector4& v) {
    return std::sqrt(v.x * v.x + v.y * v.y + v.z * v.z + v.w * v.w);
}

Vector4 Normalize(const Vector4& v) {
    float len = Length(v);
    return { v.x / len, v.y / len, v.z / len, v.w / len };
}

Vector4 Lerp(const Vector4& start, const Vector4& end, float t) {
    return start * (1.0f - t) + end * t;
}

void PrintOnImGui(const Vector4& v, const char* label) {
#ifdef USE_IMGUI
    ImGui::Begin(label);
    ImGui::Text("%s: (x: %.2f, y: %.2f, z: %.2f, w: %.2f)", "Vector4", v.x, v.y, v.z, v.w);
    ImGui::End();
#endif // IMGUI
}
