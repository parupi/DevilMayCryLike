#pragma once
#include <math/Vector3.h>
#include "assert.h"
#include "cmath"
#include <math/Matrix4x4.h>
#include <math/Quaternion.h>
#include <imgui/imgui.h>
#include <vector>

static const int kColumnWidth = 60;
static const int kRowHeight = 20;

struct EulerTransform {
	Vector3 scale;
	Vector3 rotate;
	Vector3 translate;
};

struct QuaternionTransform {
	Vector3 scale;
	Quaternion rotate;
	Vector3 translate;
};


// 正規化
float Normalize(float value);

float Lerp(float a, float b, float t);


// 座標変換
//Vector3 Transform(const Vector3& vector, const Matrix4x4& matrix);
Vector3 Transform(const Vector3& vector, const Matrix4x4& matrix);

Matrix4x4 MakeRotateXYZMatrix(Vector3 rotate);

// tanθの逆数
float cotf(float theta);

// 透視投影行列
Matrix4x4 MakePerspectiveFovMatrix(float fovY, float aspectRatio, float nearClip, float farClip);

// 正射影行列
Matrix4x4 MakeOrthographicMatrix(float left, float top, float right, float bottom, float nearClip, float farClip);

// ビューポート変換行列
Matrix4x4 MakeViewPortMatrix(float left, float top, float width, float height, float minDepth, float maxDepth);

// クロス積
Vector3 Cross(const Vector3& v1, const Vector3& v2);

Matrix4x4 LookAt(const Vector3& eye, const Vector3& target, const Vector3& up);

Vector3 TransformNormal(const Vector3& v, const Matrix4x4& m);

Vector3 ExtractTranslation(const Matrix4x4& matrix);

Vector3 CatmullRomSpline(const std::vector<Vector3>& points, float t);

Matrix4x4 DirectionToDirection(const Vector3& from, const Vector3& to);

Vector3 MatrixToEulerYXZ(const Matrix4x4& m);