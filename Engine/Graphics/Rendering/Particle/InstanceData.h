#pragma once
#include <Math/Vector4.h>
#include <Math/Matrix4x4.h>

struct InstanceData {
    Matrix4x4 world;
    Matrix4x4 wvp;
    Vector4 color;
    // スプライトシートのコマ切り出し。xy = UVオフセット / zw = UVスケール。
    // アニメーション未使用なら (0,0,1,1) でテクスチャ全体を指す
    Vector4 uvOffsetScale{ 0.0f, 0.0f, 1.0f, 1.0f };
    // x = 粒ごとの乱数(0〜1) / y = 寿命の進み具合(0〜1)。ノイズの読み出し位置と削れ具合に使う
    Vector4 misc{ 0.0f, 0.0f, 0.0f, 0.0f };
};
