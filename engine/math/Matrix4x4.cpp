#include "Matrix4x4.h"
#include <imgui.h>
#include <Quaternion.h>
#include <Vector3.h>

// デフォルトコンストラクタ
Matrix4x4::Matrix4x4() : m{ {0.0f} } {}

// イニシャライザリストを受け取るコンストラクタ
Matrix4x4::Matrix4x4(std::initializer_list<float> list) {
    if (list.size() != 16) {
        throw std::invalid_argument("Matrix4x4 initializer list must contain exactly 16 elements.");
    }
    auto it = list.begin();
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            m[i][j] = *it++;
        }
    }
}

// 行列の加算
Matrix4x4 Matrix4x4::operator+(const Matrix4x4& other) const {
    Matrix4x4 result;
    for (int i = 0; i < 4; ++i)
        for (int j = 0; j < 4; ++j)
            result.m[i][j] = m[i][j] + other.m[i][j];
    return result;
}

// 行列の減算
Matrix4x4 Matrix4x4::operator-(const Matrix4x4& other) const {
    Matrix4x4 result;
    for (int i = 0; i < 4; ++i)
        for (int j = 0; j < 4; ++j)
            result.m[i][j] = m[i][j] - other.m[i][j];
    return result;
}

// 行列の乗算
Matrix4x4 Matrix4x4::operator*(const Matrix4x4& other) const {
    Matrix4x4 result;
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            result.m[i][j] = 0.0f;
            for (int k = 0; k < 4; ++k) {
                result.m[i][j] += m[i][k] * other.m[k][j];
            }
        }
    }
    return result;
}

// スカラー倍
Matrix4x4 Matrix4x4::operator*(float scalar) const {
    Matrix4x4 result;
    for (int i = 0; i < 4; ++i)
        for (int j = 0; j < 4; ++j)
            result.m[i][j] = m[i][j] * scalar;
    return result;
}

// 加算代入
Matrix4x4& Matrix4x4::operator+=(const Matrix4x4& other) {
    for (int i = 0; i < 4; ++i)
        for (int j = 0; j < 4; ++j)
            m[i][j] += other.m[i][j];
    return *this;
}

// 減算代入
Matrix4x4& Matrix4x4::operator-=(const Matrix4x4& other) {
    for (int i = 0; i < 4; ++i)
        for (int j = 0; j < 4; ++j)
            m[i][j] -= other.m[i][j];
    return *this;
}

// 乗算代入
Matrix4x4& Matrix4x4::operator*=(const Matrix4x4& other) {
    *this = *this * other;
    return *this;
}

// スカラー倍代入
Matrix4x4& Matrix4x4::operator*=(float scalar) {
    for (int i = 0; i < 4; ++i)
        for (int j = 0; j < 4; ++j)
            m[i][j] *= scalar;
    return *this;
}

//Matrix4x4 MakeRotateMatrix(const Quaternion& quaternion) {
//    Matrix4x4 rotationMatrix;
//
//    float x = quaternion.x;
//    float y = quaternion.y;
//    float z = quaternion.z;
//    float w = quaternion.w;
//
//    rotationMatrix.m[0][0] = w * w + x * x - y * y -z * z;
//    rotationMatrix.m[0][1] = 2.0f * (x * y + w * z);
//    rotationMatrix.m[0][2] = 2.0f * (x * z - w * y);
//    rotationMatrix.m[0][3] = 0.0f;
//
//    rotationMatrix.m[1][0] = 2.0f * (x * y - w * z);
//    rotationMatrix.m[1][1] = w * w - x * x + y * y - z * z;
//    rotationMatrix.m[1][2] = 2.0f * (y * z + w * x);
//    rotationMatrix.m[1][3] = 0.0f;
//
//    rotationMatrix.m[2][0] = 2.0f * (x * z + w * y);
//    rotationMatrix.m[2][1] = 2.0f * (y * z - w * x);
//    rotationMatrix.m[2][2] = w * w - x * x - y * y + z * z;
//    rotationMatrix.m[2][3] = 0.0f;
//
//    rotationMatrix.m[3][0] = 0.0f;
//    rotationMatrix.m[3][1] = 0.0f;
//    rotationMatrix.m[3][2] = 0.0f;
//    rotationMatrix.m[3][3] = 1.0f;
//
//    return rotationMatrix;
//}

Matrix4x4 MakeRotateMatrix(const Quaternion& quaternion) {
    Matrix4x4 rotationMatrix;

    float x = quaternion.x;
    float y = quaternion.y;
    float z = quaternion.z;
    float w = quaternion.w;

    rotationMatrix.m[0][0] = 1.0f - 2.0f * (y * y + z * z);
    rotationMatrix.m[0][1] = 2.0f * (x * y + w * z);
    rotationMatrix.m[0][2] = 2.0f * (x * z - w * y);
    rotationMatrix.m[0][3] = 0.0f;

    rotationMatrix.m[1][0] = 2.0f * (x * y - w * z);
    rotationMatrix.m[1][1] = 1.0f - 2.0f * (x * x + z * z);
    rotationMatrix.m[1][2] = 2.0f * (y * z + w * x);
    rotationMatrix.m[1][3] = 0.0f;

    rotationMatrix.m[2][0] = 2.0f * (x * z + w * y);
    rotationMatrix.m[2][1] = 2.0f * (y * z - w * x);
    rotationMatrix.m[2][2] = 1.0f - 2.0f * (x * x + y * y);
    rotationMatrix.m[2][3] = 0.0f;

    rotationMatrix.m[3][0] = 0.0f;
    rotationMatrix.m[3][1] = 0.0f;
    rotationMatrix.m[3][2] = 0.0f;
    rotationMatrix.m[3][3] = 1.0f;

    return rotationMatrix;
}

Matrix4x4 MakeRotateAxisAngle(const Vector3& axis, float angle)
{
    Vector3 normAxis = axis;
    float axisLength = std::sqrt(axis.x * axis.x + axis.y * axis.y + axis.z * axis.z);
    if (axisLength != 0.0f) {
        normAxis.x /= axisLength;
        normAxis.y /= axisLength;
        normAxis.z /= axisLength;
    }

    float cosTheta = std::cos(angle);
    float sinTheta = std::sin(angle);
    float oneMinusCosTheta = 1.0f - cosTheta;

    Matrix4x4 rotateMatrix;

    rotateMatrix.m[0][0] = cosTheta + normAxis.x * normAxis.x * oneMinusCosTheta;
    rotateMatrix.m[0][1] = normAxis.x * normAxis.y * oneMinusCosTheta - normAxis.z * sinTheta;
    rotateMatrix.m[0][2] = normAxis.x * normAxis.z * oneMinusCosTheta + normAxis.y * sinTheta;
    rotateMatrix.m[0][3] = 0.0f;

    rotateMatrix.m[1][0] = normAxis.y * normAxis.x * oneMinusCosTheta + normAxis.z * sinTheta;
    rotateMatrix.m[1][1] = cosTheta + normAxis.y * normAxis.y * oneMinusCosTheta;
    rotateMatrix.m[1][2] = normAxis.y * normAxis.z * oneMinusCosTheta - normAxis.x * sinTheta;
    rotateMatrix.m[1][3] = 0.0f;

    rotateMatrix.m[2][0] = normAxis.z * normAxis.x * oneMinusCosTheta - normAxis.y * sinTheta;
    rotateMatrix.m[2][1] = normAxis.z * normAxis.y * oneMinusCosTheta + normAxis.x * sinTheta;
    rotateMatrix.m[2][2] = cosTheta + normAxis.z * normAxis.z * oneMinusCosTheta;
    rotateMatrix.m[2][3] = 0.0f;

    rotateMatrix.m[3][0] = 0.0f;
    rotateMatrix.m[3][1] = 0.0f;
    rotateMatrix.m[3][2] = 0.0f;
    rotateMatrix.m[3][3] = 1.0f;

    rotateMatrix = Transpose(rotateMatrix);

    return rotateMatrix;
}

// 逆行列を計算する関数
Matrix4x4 Inverse(const Matrix4x4& m) {
    float A =
        m.m[0][0] * m.m[1][1] * m.m[2][2] * m.m[3][3] + m.m[0][0] * m.m[1][2] * m.m[2][3] * m.m[3][1] + m.m[0][0] * m.m[1][3] * m.m[2][1] * m.m[3][2] - m.m[0][0] * m.m[1][3] * m.m[2][2] * m.m[3][1] -
        m.m[0][0] * m.m[1][2] * m.m[2][1] * m.m[3][3] - m.m[0][0] * m.m[1][1] * m.m[2][3] * m.m[3][2] - m.m[0][1] * m.m[1][0] * m.m[2][2] * m.m[3][3] - m.m[0][2] * m.m[1][0] * m.m[2][3] * m.m[3][1] -
        m.m[0][3] * m.m[1][0] * m.m[2][1] * m.m[3][2] + m.m[0][3] * m.m[1][0] * m.m[2][2] * m.m[3][1] + m.m[0][2] * m.m[1][0] * m.m[2][1] * m.m[3][3] + m.m[0][1] * m.m[1][0] * m.m[2][3] * m.m[3][2] +
        m.m[0][1] * m.m[1][2] * m.m[2][0] * m.m[3][3] + m.m[0][2] * m.m[1][3] * m.m[2][0] * m.m[3][1] + m.m[0][3] * m.m[1][1] * m.m[2][0] * m.m[3][2] - m.m[0][3] * m.m[1][2] * m.m[2][0] * m.m[3][1] -
        m.m[0][2] * m.m[1][1] * m.m[2][0] * m.m[3][3] - m.m[0][1] * m.m[1][3] * m.m[2][0] * m.m[3][2] - m.m[0][1] * m.m[1][2] * m.m[2][3] * m.m[3][0] - m.m[0][2] * m.m[1][3] * m.m[2][1] * m.m[3][0] -
        m.m[0][3] * m.m[1][1] * m.m[2][2] * m.m[3][0] + m.m[0][3] * m.m[1][2] * m.m[2][1] * m.m[3][0] + m.m[0][2] * m.m[1][1] * m.m[2][3] * m.m[3][0] + m.m[0][1] * m.m[1][3] * m.m[2][2] * m.m[3][0];

    return {
        (m.m[1][1] * m.m[2][2] * m.m[3][3] + m.m[1][2] * m.m[2][3] * m.m[3][1] + m.m[1][3] * m.m[2][1] * m.m[3][2] - m.m[1][3] * m.m[2][2] * m.m[3][1] - m.m[1][2] * m.m[2][1] * m.m[3][3] -
         m.m[1][1] * m.m[2][3] * m.m[3][2]) /
            A,

        (-m.m[0][1] * m.m[2][2] * m.m[3][3] - m.m[0][2] * m.m[2][3] * m.m[3][1] - m.m[0][3] * m.m[2][1] * m.m[3][2] + m.m[0][3] * m.m[2][2] * m.m[3][1] + m.m[0][2] * m.m[2][1] * m.m[3][3] +
         m.m[0][1] * m.m[2][3] * m.m[3][2]) /
            A,

        (m.m[0][1] * m.m[1][2] * m.m[3][3] + m.m[0][2] * m.m[1][3] * m.m[3][1] + m.m[0][3] * m.m[1][1] * m.m[3][2] - m.m[0][3] * m.m[1][2] * m.m[3][1] - m.m[0][2] * m.m[1][1] * m.m[3][3] -
         m.m[0][1] * m.m[1][3] * m.m[3][2]) /
            A,

        (-m.m[0][1] * m.m[1][2] * m.m[2][3] - m.m[0][2] * m.m[1][3] * m.m[2][1] - m.m[0][3] * m.m[1][1] * m.m[2][2] + m.m[0][3] * m.m[1][2] * m.m[2][1] + m.m[0][2] * m.m[1][1] * m.m[2][3] +
         m.m[0][1] * m.m[1][3] * m.m[2][2]) /
            A,

        (-m.m[1][0] * m.m[2][2] * m.m[3][3] - m.m[1][2] * m.m[2][3] * m.m[3][0] - m.m[1][3] * m.m[2][0] * m.m[3][2] + m.m[1][3] * m.m[2][2] * m.m[3][0] + m.m[1][2] * m.m[2][0] * m.m[3][3] +
         m.m[1][0] * m.m[2][3] * m.m[3][2]) /
            A,

        (m.m[0][0] * m.m[2][2] * m.m[3][3] + m.m[0][2] * m.m[2][3] * m.m[3][0] + m.m[0][3] * m.m[2][0] * m.m[3][2] - m.m[0][3] * m.m[2][2] * m.m[3][0] - m.m[0][2] * m.m[2][0] * m.m[3][3] -
         m.m[0][0] * m.m[2][3] * m.m[3][2]) /
            A,

        (-m.m[0][0] * m.m[1][2] * m.m[3][3] - m.m[0][2] * m.m[1][3] * m.m[3][0] - m.m[0][3] * m.m[1][0] * m.m[3][2] + m.m[0][3] * m.m[1][2] * m.m[3][0] + m.m[0][2] * m.m[1][0] * m.m[3][3] +
         m.m[0][0] * m.m[1][3] * m.m[3][2]) /
            A,

        (m.m[0][0] * m.m[1][2] * m.m[2][3] + m.m[0][2] * m.m[1][3] * m.m[2][0] + m.m[0][3] * m.m[1][0] * m.m[2][2] - m.m[0][3] * m.m[1][2] * m.m[2][0] - m.m[0][1] * m.m[1][0] * m.m[2][3] -
         m.m[0][0] * m.m[1][3] * m.m[2][2]) /
            A,

        (m.m[1][0] * m.m[2][1] * m.m[3][3] + m.m[1][1] * m.m[2][3] * m.m[3][0] + m.m[1][3] * m.m[2][0] * m.m[3][1] - m.m[1][3] * m.m[2][1] * m.m[3][0] - m.m[1][1] * m.m[2][0] * m.m[3][3] -
         m.m[1][0] * m.m[2][3] * m.m[3][1]) /
            A,

        (-m.m[0][0] * m.m[2][1] * m.m[3][3] - m.m[0][1] * m.m[2][3] * m.m[3][0] - m.m[0][3] * m.m[2][0] * m.m[3][1] + m.m[0][3] * m.m[2][1] * m.m[3][0] + m.m[0][1] * m.m[2][0] * m.m[3][3] +
         m.m[0][0] * m.m[2][3] * m.m[3][1]) /
            A,

        (m.m[0][0] * m.m[1][1] * m.m[3][3] + m.m[0][1] * m.m[1][3] * m.m[3][0] + m.m[0][3] * m.m[1][0] * m.m[3][1] - m.m[0][3] * m.m[1][1] * m.m[3][0] - m.m[0][1] * m.m[1][0] * m.m[3][3] -
         m.m[0][0] * m.m[1][3] * m.m[3][1]) /
            A,

        (-m.m[0][0] * m.m[1][1] * m.m[2][3] - m.m[0][1] * m.m[1][3] * m.m[2][0] - m.m[0][3] * m.m[1][0] * m.m[2][1] + m.m[0][3] * m.m[1][1] * m.m[2][0] + m.m[0][1] * m.m[1][0] * m.m[2][3] +
         m.m[0][0] * m.m[1][3] * m.m[2][1]) /
            A,

        (-m.m[1][0] * m.m[2][1] * m.m[3][2] - m.m[1][1] * m.m[2][2] * m.m[3][0] - m.m[1][2] * m.m[2][0] * m.m[3][1] + m.m[1][2] * m.m[2][1] * m.m[3][0] + m.m[1][1] * m.m[2][0] * m.m[3][2] +
         m.m[1][0] * m.m[2][2] * m.m[3][1]) /
            A,

        (m.m[0][0] * m.m[2][1] * m.m[3][2] + m.m[0][1] * m.m[2][2] * m.m[3][0] + m.m[0][2] * m.m[2][0] * m.m[3][1] - m.m[0][2] * m.m[2][1] * m.m[3][0] - m.m[0][1] * m.m[2][0] * m.m[3][2] -
         m.m[0][0] * m.m[2][2] * m.m[3][1]) /
            A,

        (-m.m[0][0] * m.m[1][1] * m.m[3][2] - m.m[0][1] * m.m[1][2] * m.m[3][0] - m.m[0][2] * m.m[1][0] * m.m[3][1] + m.m[0][2] * m.m[1][1] * m.m[3][0] + m.m[0][1] * m.m[1][0] * m.m[3][2] +
         m.m[0][0] * m.m[1][2] * m.m[3][1]) /
            A,

        (m.m[0][0] * m.m[1][1] * m.m[2][2] + m.m[0][1] * m.m[1][2] * m.m[2][0] + m.m[0][2] * m.m[1][0] * m.m[2][1] - m.m[0][2] * m.m[1][1] * m.m[2][0] - m.m[0][1] * m.m[1][0] * m.m[2][2] -
         m.m[0][0] * m.m[1][2] * m.m[2][1]) /
            A

    };
}

// 転置行列
Matrix4x4 Transpose(const Matrix4x4& m) {
    return {
        m.m[0][0], m.m[1][0], m.m[2][0], m.m[3][0],
        m.m[0][1], m.m[1][1], m.m[2][1], m.m[3][1],
        m.m[0][2], m.m[1][2], m.m[2][2], m.m[3][2],
        m.m[0][3], m.m[1][3], m.m[2][3], m.m[3][3]
    };
}

// 単位行列
Matrix4x4 MakeIdentity4x4() {
    return { 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1 };
}


Matrix4x4 MakeTranslateMatrix(const Vector3& translate) { return { 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, translate.x, translate.y, translate.z, 1 }; }

Matrix4x4 MakeScaleMatrix(const Vector3& scale) { return { scale.x, 0, 0, 0, 0, scale.y, 0, 0, 0, 0, scale.z, 0, 0, 0, 0, 1 }; }


// X軸回転行列
Matrix4x4 MakeRotateXMatrix(float radian) {
    return { 1, 0, 0, 0, 0, cosf(radian), sinf(radian), 0, 0, -sinf(radian), cosf(radian), 0, 0, 0, 0, 1 };
}

// Y軸回転行列
Matrix4x4 MakeRotateYMatrix(float radian) {
    return { cosf(radian), 0, -sinf(radian), 0, 0, 1, 0, 0, sinf(radian), 0, cosf(radian), 0, 0, 0, 0, 1 };
}

// Z軸回転行列
Matrix4x4 MakeRotateZMatrix(float radian) {
    return { cosf(radian), sinf(radian), 0, 0, -sinf(radian), cosf(radian), 0, 0, 0, 0, 1, 0, 0, 0, 0, 1 };
}

Matrix4x4 MakeRotateXYZMatrix(Vector3 rotate) {
    Matrix4x4 rotateXMatrix = MakeRotateXMatrix(rotate.x);
    Matrix4x4 rotateYMatrix = MakeRotateYMatrix(rotate.y);
    Matrix4x4 rotateZMatrix = MakeRotateZMatrix(rotate.z);
    Matrix4x4 rotateMatrix = (rotateYMatrix * rotateXMatrix * rotateZMatrix);
    return rotateMatrix;
}

Matrix4x4 MakeAffineMatrix(const Vector3& scale, const Vector3& rotate, const Vector3& translate) {
    Matrix4x4 scaleMatrix = MakeScaleMatrix(scale);
    Matrix4x4 rotateXMatrix = MakeRotateXMatrix(rotate.x);
    Matrix4x4 rotateYMatrix = MakeRotateYMatrix(rotate.y);
    Matrix4x4 rotateZMatrix = MakeRotateZMatrix(rotate.z);
    Matrix4x4 translateMatrix = MakeTranslateMatrix(translate);
    Matrix4x4 rotateMatrix = rotateXMatrix * rotateYMatrix * rotateZMatrix;

    return { scaleMatrix * rotateMatrix * translateMatrix };
}

Matrix4x4 MakeAffineMatrix(const Vector3& scale, const Quaternion& rotate, const Vector3& translate) {
    Matrix4x4 result;
    Matrix4x4 scaleMatrix = MakeScaleMatrix(scale);
    Matrix4x4 rotateMatrix = MakeRotateMatrix(Normalize(rotate));
    Matrix4x4 translateMatrix = MakeTranslateMatrix(translate);

    result = scaleMatrix * rotateMatrix * translateMatrix;
    return result;
}

void PrintOnImGui(const Matrix4x4& matrix, const char* label) {
    ImGui::Begin("Matrix4x4");
    ImGui::Text("%s:", label);
    for (int row = 0; row < 4; ++row) {
        ImGui::Text("%.3f  %.3f  %.3f  %.3f",
            matrix.m[row][0], matrix.m[row][1], matrix.m[row][2], matrix.m[row][3]);
    }
    ImGui::End();
}