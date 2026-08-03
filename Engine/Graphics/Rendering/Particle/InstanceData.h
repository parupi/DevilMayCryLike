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
};