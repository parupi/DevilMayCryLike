#include "Quaternion.h"
#include <cmath>    // sqrtf
#include <stdexcept>
#ifdef USE_IMGUI
#include <imgui.h>
#endif // IMGUI
#include <math/Matrix4x4.h>
#include <math/Vector3.h>
#include <numbers>
#include <math/function.h>
// コンストラクタ
Quaternion::Quaternion(float x, float y, float z, float w) : x(x), y(y), z(z), w(w) {}

// クォータニオンの乗算
Quaternion Quaternion::operator*(const Quaternion& q) const {
    return Quaternion(
        w * q.x + x * q.w + y * q.z - z * q.y,
        w * q.y - x * q.z + y * q.w + z * q.x,
        w * q.z + x * q.y - y * q.x + z * q.w,
        w * q.w - x * q.x - y * q.y - z * q.z
    );
}

// 等価・非等価演算子
bool Quaternion::operator==(const Quaternion& q) const {
    return x == q.x && y == q.y && z == q.z && w == q.w;
}

bool Quaternion::operator!=(const Quaternion& q) const {
    return !(*this == q);
}

// 加算・減算演算子
Quaternion Quaternion::operator+(const Quaternion& q) const {
    return Quaternion(x + q.x, y + q.y, z + q.z, w + q.w);
}

Quaternion Quaternion::operator-(const Quaternion& q) const {
    return Quaternion(x - q.x, y - q.y, z - q.z, w - q.w);
}

Quaternion& Quaternion::operator+=(const Quaternion& q) {
    x += q.x;
    y += q.y;
    z += q.z;
    w += q.w;
    return *this;
}

Quaternion& Quaternion::operator-=(const Quaternion& q) {
    x -= q.x;
    y -= q.y;
    z -= q.z;
    w -= q.w;
    return *this;
}

// スカラー倍演算子
Quaternion Quaternion::operator*(float scalar) const {
    return Quaternion(x * scalar, y * scalar, z * scalar, w * scalar);
}

Quaternion& Quaternion::operator*=(float scalar) {
    x *= scalar;
    y *= scalar;
    z *= scalar;
    w *= scalar;
    return *this;
}

// 単位クォータニオン（乗法の単位元）
Quaternion Identity() {
    return Quaternion(0, 0, 0, 1);
}

// クォータニオンの共役
Quaternion Conjugate(const Quaternion& q) {
    return Quaternion(-q.x, -q.y, -q.z, q.w);
}

// クォータニオンのノルム
float Norm(const Quaternion& q) {
    return sqrtf(q.x * q.x + q.y * q.y + q.z * q.z + q.w * q.w);
}

// クォータニオンの正規化
Quaternion Normalize(const Quaternion& q) {
    float norm = Norm(q);
    if (norm == 0.0f) {
        throw std::runtime_error("Cannot normalize a zero-norm quaternion.");
    }
    return Quaternion(q.x / norm, q.y / norm, q.z / norm, q.w / norm);
}

// クォータニオンの逆
Quaternion Inverse(const Quaternion& q) {
    float norm = Norm(q);
    if (norm == 0.0f) {
        throw std::runtime_error("Cannot invert a zero-norm quaternion.");
    }
    Quaternion conjugate = Conjugate(q);
    return Quaternion(conjugate.x / (norm * norm), conjugate.y / (norm * norm), conjugate.z / (norm * norm), conjugate.w / (norm * norm));
}

Quaternion MakeRotateAxisAngleQuaternion(const Vector3& axis, float angle) {
    Vector3 normalAxis = Normalize(axis);
    float halfAngle = angle * 0.5f;
    float sinHalfAngle = sinf(halfAngle);
    return Quaternion(
        normalAxis.x * sinHalfAngle,
        normalAxis.y * sinHalfAngle,
        normalAxis.z * sinHalfAngle,
        cosf(halfAngle)
    );
}

Quaternion Slerp(Quaternion q0, Quaternion q1, float t)
{
    float dot = Dot(q0, q1);
    if (dot < 0.0f) {
        q0 = { -q0.x, -q0.y, -q0.z, -q0.w }; // 反対方向に補間
        dot = -dot;
    }

    // なす角を求める
    float theta = std::acos(dot);
    float sinTheta = std::sin(theta);

    // 補間係数を求める
    if (sinTheta > 0.001f) { // 数値安定性のための閾値
        float scale0 = std::sin((1 - t) * theta) / sinTheta;
        float scale1 = std::sin(t * theta) / sinTheta;

        // 補間後のQuaternionを計算
        return q0 * scale0 + q1 * scale1;
    }
    else {
        // ほぼ同じ方向の場合、線形補間
        return q0 * (1 - t) + q1 * t;
    }
}

float Dot(const Quaternion& q0, const Quaternion& q1)
{
    return q0.x * q1.x + q0.y * q1.y + q0.z * q1.z + q0.w * q1.w;
}


const Quaternion EulerDegree(const Vector3& rotate) noexcept {
    // ラジアン変換して定義
    return EulerRadian(rotate.x * float(std::numbers::pi) / 180.0f, rotate.y * float(std::numbers::pi) / 180.0f, rotate.z * float(std::numbers::pi) / 180.0f);
}

const Quaternion EulerRadian(float pitch, float yaw, float roll) noexcept {
    float cosPitch = std::cos(pitch / 2);
    float cos_yaw = std::cos(yaw / 2);
    float cos_roll = std::cos(roll / 2);
    float sin_pitch = std::sin(pitch / 2);
    float sin_yaw = std::sin(yaw / 2);
    float sin_roll = std::sin(roll / 2);
    Quaternion result;
    result.x = sin_pitch * cos_yaw * cos_roll - cosPitch * sin_yaw * sin_roll;
    result.y = cosPitch * sin_yaw * cos_roll + sin_pitch * cos_yaw * sin_roll;
    result.z = cosPitch * cos_yaw * sin_roll - sin_pitch * sin_yaw * cos_roll;
    result.w = cosPitch * cos_yaw * cos_roll + sin_pitch * sin_yaw * sin_roll;
    return result;
}

const Quaternion LookRotation(const Vector3& direction)
{
    Vector3 forward = Normalize(direction);
    Vector3 up = { 0.0f, 1.0f, 0.0f }; // ワールド上の上方向

    // forwardとupが並行にならないようにチェック
    if (std::abs(Dot(forward, up)) > 0.999f) {
        up = { 0.0f, 0.0f, 1.0f }; // 代わりのupベクトル
    }

    Vector3 right = Normalize(Cross(up, forward));
    up = Cross(forward, right);

    Matrix4x4 lookMatrix = {
        right.x,   right.y,   right.z,   0.0f,
        up.x,      up.y,      up.z,      0.0f,
        forward.x, forward.y, forward.z, 0.0f,
        0.0f,      0.0f,      0.0f,      1.0f,
    };

    return QuaternionFromMatrix(lookMatrix);
}

Quaternion QuaternionFromMatrix(const Matrix4x4& m)
{
    Quaternion q{};
    float trace = m.m[0][0] + m.m[1][1] + m.m[2][2];  // 対角成分の和

    if (trace > 0.0f) {
        float s = sqrtf(trace + 1.0f) * 2.0f; // s = 4 * qw
        q.w = 0.25f * s;
        q.x = (m.m[2][1] - m.m[1][2]) / s;
        q.y = (m.m[0][2] - m.m[2][0]) / s;
        q.z = (m.m[1][0] - m.m[0][1]) / s;
    } else if (m.m[0][0] > m.m[1][1] && m.m[0][0] > m.m[2][2]) {
        float s = sqrtf(1.0f + m.m[0][0] - m.m[1][1] - m.m[2][2]) * 2.0f; // s = 4 * qx
        q.w = (m.m[2][1] - m.m[1][2]) / s;
        q.x = 0.25f * s;
        q.y = (m.m[0][1] + m.m[1][0]) / s;
        q.z = (m.m[0][2] + m.m[2][0]) / s;
    } else if (m.m[1][1] > m.m[2][2]) {
        float s = sqrtf(1.0f + m.m[1][1] - m.m[0][0] - m.m[2][2]) * 2.0f; // s = 4 * qy
        q.w = (m.m[0][2] - m.m[2][0]) / s;
        q.x = (m.m[0][1] + m.m[1][0]) / s;
        q.y = 0.25f * s;
        q.z = (m.m[1][2] + m.m[2][1]) / s;
    } else {
        float s = sqrtf(1.0f + m.m[2][2] - m.m[0][0] - m.m[1][1]) * 2.0f; // s = 4 * qz
        q.w = (m.m[1][0] - m.m[0][1]) / s;
        q.x = (m.m[0][2] + m.m[2][0]) / s;
        q.y = (m.m[1][2] + m.m[2][1]) / s;
        q.z = 0.25f * s;
    }

    return q;
}

Quaternion FromToRotation(const Vector3& from, const Vector3& to)
{
    Vector3 f = from;
    Vector3 t = to;
    Normalize(f);
    Normalize(t);

    float dot = Dot(f, t);

    // ほぼ同じ方向
    if (dot > 0.9999f) {
        return Identity();
    }

    // ほぼ逆方向（180度回転）
    if (dot < -0.9999f) {
        // from と直交する軸を探す
        Vector3 axis = Cross(Vector3(1.0f, 0.0f, 0.0f), f);
        if (Length(axis) < 0.0001f) {
            axis = Cross(Vector3(0.0f, 1.0f, 0.0f), f);
        }
        Normalize(axis);
        return MakeRotateAxisAngleQuaternion(axis, std::numbers::pi);
    }

    // 通常ケース
    Vector3 axis = Cross(f, t);
    float s = std::sqrt((1.0f + dot) * 2.0f);
    float invS = 1.0f / s;

    return Quaternion(
        axis.x * invS,
        axis.y * invS,
        axis.z * invS,
        s * 0.5f
    );
}


// ImGuiを使ったクォータニオンの描画
void PrintOnImGui(const Quaternion& q, const char* label) {
#ifdef USE_IMGUI
    ImGui::Begin(label);
    ImGui::Text("%s: (x: %.2f, y: %.2f, z: %.2f, w: %.2f)", "Quaternion", q.x, q.y, q.z, q.w);
    ImGui::End();
#endif // IMGUI
}
