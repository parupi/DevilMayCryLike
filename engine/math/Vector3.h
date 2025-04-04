#pragma once
#include <cmath> // sqrtf関数を使うために必要
struct Quaternion;

/// <summary>
/// 3次元ベクトル
/// </summary>
struct Vector3 final {
    float x;
    float y;
    float z;

    // ベクトルの加算
    Vector3 operator+(const Vector3& other) const;
    // ベクトルの減算
    Vector3 operator-(const Vector3& other) const;
    // ベクトルのスカラー倍
    Vector3 operator*(float scalar) const;
    // ベクトルのスカラー除算
    Vector3 operator/(float scalar) const;
    // ベクトルの加算代入
    Vector3& operator+=(const Vector3& other);
    // ベクトルの減算代入
    Vector3& operator-=(const Vector3& other);
    // ベクトルのスカラー倍代入
    Vector3& operator*=(float scalar);
    // ベクトルのスカラー除算代入
    Vector3& operator/=(float scalar);
    // ベクトルの等価比較
    bool operator==(const Vector3& other) const;
    // ベクトルの非等価比較
    bool operator!=(const Vector3& other) const;
};

// ヘルパー関数
float Dot(const Vector3& v1, const Vector3& v2);
float Length(const Vector3& v);
Vector3 Normalize(const Vector3& v);
Vector3 RotateVector(const Vector3& vector, const Quaternion& quaternion);
Vector3 Lerp(const Vector3& start, const Vector3& end, float t);

// ImGuiを使ったVector3の描画
void PrintOnImGui(const Vector3& v, const char* label = "Vector3");
