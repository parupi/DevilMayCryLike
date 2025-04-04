#pragma once

/// <summary>
/// 2次元ベクトル
/// </summary>
struct Vector2 final {
    float x;
    float y;

    // ベクトルの加算
    Vector2 operator+(const Vector2& other) const {
        return { x + other.x, y + other.y };
    }

    // ベクトルの減算
    Vector2 operator-(const Vector2& other) const {
        return { x - other.x, y - other.y };
    }

    // ベクトルのスカラー倍
    Vector2 operator*(float scalar) const {
        return { x * scalar, y * scalar };
    }

    // ベクトルのスカラー除算
    Vector2 operator/(float scalar) const {
        return { x / scalar, y / scalar };
    }

    // ベクトルの加算代入
    Vector2& operator+=(const Vector2& other) {
        x += other.x;
        y += other.y;
        return *this;
    }

    // ベクトルの減算代入
    Vector2& operator-=(const Vector2& other) {
        x -= other.x;
        y -= other.y;
        return *this;
    }

    // ベクトルのスカラー倍代入
    Vector2& operator*=(float scalar) {
        x *= scalar;
        y *= scalar;
        return *this;
    }

    // ベクトルのスカラー除算代入
    Vector2& operator/=(float scalar) {
        x /= scalar;
        y /= scalar;
        return *this;
    }

    // ベクトルの等価比較
    bool operator==(const Vector2& other) const {
        return x == other.x && y == other.y;
    }

    // ベクトルの非等価比較
    bool operator!=(const Vector2& other) const {
        return !(*this == other);
    }
};
