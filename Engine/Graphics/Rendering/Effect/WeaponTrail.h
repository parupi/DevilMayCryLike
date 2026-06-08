#pragma once
#include <d3d12.h>
#include <wrl.h>
#include <deque>
#include "Math/Vector3.h"
#include "Math/Vector2.h"
#include "Math/Vector4.h"
#include "Math/Matrix4x4.h"
#include "Graphics/Resource/ResourceManager.h"

// 1頂点のレイアウト (Trail.VS.hlsl の InputLayout と一致させること)
struct TrailVertex {
    Vector3 position; // POSITION: R32G32B32_FLOAT
    Vector2 texcoord; // TEXCOORD0: R32G32_FLOAT
    Vector4 color;    // COLOR:     R32G32B32A32_FLOAT
};

// 武器の軌跡をリボンメッシュ (TriangleStrip) で描画するクラス
class WeaponTrail {
public:
    static const uint32_t kMaxPoints = 32;

    WeaponTrail() = default;
    ~WeaponTrail();

    // 初期化 (PlayerWeapon::Initialize() から呼ぶ)
    void Initialize();
    // 毎フレーム更新 (age を進め、寿命切れの点を削除)
    void Update(float deltaTime);
    // 攻撃中に毎フレーム刃先 (tip) と根本 (hilt) のワールド座標を追加
    void AddPoint(const Vector3& tip, const Vector3& hilt);
    // 軌跡を即座にリセット (攻撃終了後の残像を消す場合に呼ぶ)
    void Clear();
    // 描画 (PlayerWeapon::DrawEffect() から呼ぶ)
    void Draw();

    void SetTintColor(const Vector4& color) { tintColor_ = color; }
    void SetLifetime(float lifetime) { lifetime_ = lifetime; }

private:
    struct TrailPoint {
        Vector3 tip;
        Vector3 hilt;
        float age; // 0 = 最新、lifetime_ 以上で削除
    };
    // HLSL の cbuffer TrailCB (b0) と同じレイアウト
    struct TrailConstantData {
        Matrix4x4 viewProj;
        Vector4   tintColor;
    };

    void CreateVertexBuffer();
    void CreateConstantBuffer();
    void CreateTrailTexture();
    // points_ から mappedVB_ へリボン頂点を書き込む
    void BuildMesh();

    std::deque<TrailPoint> points_;
    float lifetime_ = 0.25f;
    Vector4 tintColor_ = { 0.5f, 0.85f, 1.0f, 1.0f }; // 青白色

    // 動的頂点バッファ (Upload ヒープ、毎フレーム CPU から書き込み)
    BufferHandle vbHandle_ = kInvalidBufferHandle;
    TrailVertex* mappedVB_ = nullptr;
    D3D12_VERTEX_BUFFER_VIEW vbView_{};

    // 定数バッファ (viewProj + tintColor)
    BufferHandle cbHandle_ = kInvalidBufferHandle;
    TrailConstantData* mappedCB_ = nullptr;

    // 軌跡専用グラデーションテクスチャ
    Microsoft::WRL::ComPtr<ID3D12Resource> trailTexture_;
    uint32_t textureSrvIndex_ = 0;

    uint32_t vertexCount_ = 0;
};
