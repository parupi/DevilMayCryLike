#pragma once
#include <Vector3.h>
#include <Vector4.h>
#include <Matrix4x4.h>
#include <vector>

// GPUに送るパーティクル1個分のデータ
struct ParticleForGPU {
    Matrix4x4 WVP;
    Matrix4x4 World;
    Vector4 color;
};

// CPUで制御するパーティクルの基本データ
struct Particle {
    Vector3 position;
    Vector3 velocity;
    Vector3 rotation;
    Vector3 scale;
    Vector4 color;
    float lifetime;
    float maxLifetime;
    bool isActive = false;
};

// パーティクルグループ（エミッタ + パラメータセット）
struct ParticleGroup {
    std::vector<Particle> particles;
    size_t instanceCount = 0;

    Vector3 emitterPosition = {};
    Vector3 emitterDirection = { 0, 1, 0 };

    float minLifetime = 1.0f;
    float maxLifetime = 2.0f;

    Vector3 minVelocity = { -1, 2, -1 };
    Vector3 maxVelocity = { 1, 4, 1 };

    Vector3 minScale = { 0.2f, 0.2f, 0.2f };
    Vector3 maxScale = { 1.0f, 1.0f, 1.0f };

    Vector3 minColor = { 1, 1, 1 };
    Vector3 maxColor = { 1, 1, 1 };
    float minAlpha = 1.0f;
    float maxAlpha = 1.0f;

    bool isBillboard = true;

    float emitTimer = 0.0f;
    float emitInterval = 0.05f;
};