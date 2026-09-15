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
    // 覚えておく点（1フレーム1点）の最大数
    static const uint32_t kMaxPoints = 32;
    // 点と点の間を何分割して描くかの上限
    static const uint32_t kMaxSubdivisions = 4;

    WeaponTrail() = default;
    ~WeaponTrail();

    // 初期化
    void Initialize();
    // 毎フレーム更新 (age を進め、寿命切れの点を削除)
    void Update(float deltaTime);
    // 毎フレーム刃先 (tip) と根本 (hilt) のワールド座標を追加。前の点とほぼ同じ位置なら積まない（ヒットストップ中など）
    void AddPoint(const Vector3& tip, const Vector3& hilt);
    // 軌跡を即座にリセット
    void Clear();
    // 描画
    void Draw();

    void SetTintColor(const Vector4& color) { tintColor_ = color; }
    void SetLifetime(float lifetime) { lifetime_ = lifetime; }
    /// <summary>
    /// 点と点の間を Catmull-Rom で何分割するか（1 で分割なし）。
    /// 速く振るとフレーム間で刃が大きく動くので、分割しないと軌跡が折れ線になる
    /// </summary>
    void SetSubdivisions(uint32_t subdivisions);
    /// <summary>
    /// true で加算合成（光る帯）、false で半透明の合成（明るい床の上でも色が残る帯・ブレ）
    /// </summary>
    void SetAdditive(bool additive) { additive_ = additive; }

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

    // 分割後に描ける点の最大数
    static const uint32_t kMaxRenderPoints = (kMaxPoints - 1) * kMaxSubdivisions + 1;

    void CreateVertexBuffer();
    void CreateConstantBuffer();
    void CreateTrailTexture();
    // points_ から mappedVB_ へリボン頂点を書き込む
    void BuildMesh();
    // index 番目の描画点（刃先・根本の2頂点）を書き込む
    void WriteVertex(uint32_t index, uint32_t total, const Vector3& tip, const Vector3& hilt, float age);

    std::deque<TrailPoint> points_;
    float lifetime_ = 0.25f;
    Vector4 tintColor_ = { 0.5f, 0.85f, 1.0f, 1.0f }; // 青白色
    uint32_t subdivisions_ = 1;
    bool additive_ = true;

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
