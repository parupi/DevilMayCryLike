#pragma once
#include "BaseOffScreen.h"
#include <Math/Vector2.h>

/// <summary>
/// 画面全体のブルーム。1つのチェーン要素の中で複数パスを実行する。
///   1. 明るい部分を抽出（半解像度）
///   2. 横ガウスブラー（半解像度）
///   3. 縦ガウスブラー（半解像度）
///   4. チェーンの出力先に元の絵をコピー → ぼかした光を加算ブレンドで重ねる
/// 半解像度の作業用RTを2枚自前で持つ。
/// </summary>
class BloomEffect : public BaseOffScreen
{
public:
    explicit BloomEffect(const std::string& name);
    ~BloomEffect();

    void Update() override;
    void Draw()   override;


    // 調整用パラメータ（ImGuiからも触れる）
    struct BloomSettings {
        float threshold = 0.75f;  // この輝度を超えた部分が光る
        float softKnee = 0.30f;   // しきい値付近をなめらかに繋ぐ幅
        float blurRadius = 1.60f; // ブラーの広がり（作業RTのテクセル単位）
        float intensity = 0.85f;  // 重ねる光の強さ
    };
    BloomSettings& GetSettings() { return settings_; }

private:
    // HLSL の cbuffer と一致させること（各16バイト）
    struct BrightParam {
        float threshold = 0.75f;
        float softKnee = 0.30f;
        float _pad0 = 0.0f;
        float _pad1 = 0.0f;
    };
    struct BlurParam {
        Vector2 direction{ 0.0f, 0.0f };
        float _pad0 = 0.0f;
        float _pad1 = 0.0f;
    };
    struct CompositeParam {
        float intensity = 0.85f;
        float _pad0 = 0.0f;
        float _pad1 = 0.0f;
        float _pad2 = 0.0f;
    };

    // 半解像度の作業用RT1枚分
    struct WorkTarget {
        Microsoft::WRL::ComPtr<ID3D12Resource> resource;
        D3D12_CPU_DESCRIPTOR_HANDLE rtv{};
        D3D12_GPU_DESCRIPTOR_HANDLE srv{};
    };

    void CreateWorkTargets();
    void CreateEffectResource();
    // settings_ を定数バッファへ書き込む。ImGui に触れないので、
    // フレームの外（コンストラクタ）から呼んでも安全
    void ApplyParams();
    // 作業用RTへ1パス描画する
    void DrawPass(OffScreenEffectType type, D3D12_GPU_DESCRIPTOR_HANDLE input,
        const WorkTarget& output, uint32_t cbvHandle);

    WorkTarget bright_{}; // 抽出結果 → 縦ブラーの出力先としても使う
    WorkTarget blur_{};   // 横ブラーの出力先

    uint32_t brightParamHandle_ = 0;
    uint32_t blurHParamHandle_ = 0;
    uint32_t blurVParamHandle_ = 0;
    uint32_t compositeParamHandle_ = 0;

    BrightParam* brightParam_ = nullptr;
    BlurParam* blurHParam_ = nullptr;
    BlurParam* blurVParam_ = nullptr;
    CompositeParam* compositeParam_ = nullptr;

    BloomSettings settings_;

    uint32_t workWidth_ = 0;
    uint32_t workHeight_ = 0;
    D3D12_VIEWPORT workViewport_{};
    D3D12_RECT workScissor_{};
};
