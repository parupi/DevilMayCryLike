#pragma once
#include <cmath> // sqrtf関数を使うために必要
#include <math/Matrix4x4.h>
struct Quaternion;

/// <summary>
/// 4次元ベクトル
/// </summary>
struct Vector4 final {
	float x;
	float y;
	float z;
	float w;

	// ベクトルの加算
	Vector4 operator+(const Vector4& other) const;
	// ベクトルの減算
	Vector4 operator-(const Vector4& other) const;
	// ベクトルのスカラー倍
	Vector4 operator*(float scalar) const;
	// ベクトルのスカラー除算
	Vector4 operator/(float scalar) const;
	// ベクトルの加算代入
	Vector4& operator+=(const Vector4& other);
	// ベクトルの減算代入
	Vector4& operator-=(const Vector4& other);
	// ベクトルのスカラー倍代入
	Vector4& operator*=(float scalar);
	// ベクトルのスカラー除算代入
	Vector4& operator/=(float scalar);
	// ベクトルの等価比較
	bool operator==(const Vector4& other) const;
	// ベクトルの非等価比較
	bool operator!=(const Vector4& other) const;
};
// ベクトルと行列の積
Vector4 operator*(const Vector4& vec, const Matrix4x4& mat);
// ヘルパー関数
float Dot(const Vector4& v1, const Vector4& v2);
float Length(const Vector4& v);
Vector4 Normalize(const Vector4& v);
Vector4 Lerp(const Vector4& start, const Vector4& end, float t);

// ImGuiを使ったVector4の描画
void PrintOnImGui(const Vector4& v, const char* label = "Vector4");
