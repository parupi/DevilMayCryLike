#pragma once
#include "BaseOffScreen.h"
#include <Math/Vector2.h>

/// <summary>
/// 画面の外へ向かって伸びる集中線。中心から放射状に白い筋を描いて速度感を出す。
/// 突進のように「こちらへ突っ込んでくる」瞬間に、中心をその相手の画面位置に合わせて使う。
/// 線はシェーダ内で作るのでテクスチャは要らない。既定では無効状態で生成される。
/// </summary>
class SpeedLineEffect : public BaseOffScreen
{
public:
    explicit SpeedLineEffect(const std::string& name);
    ~SpeedLineEffect();

    void Update() override;
    void Draw()   override;

    // HLSL の cbuffer と一致させること（32バイト）
    struct SpeedLineData {
        Vector2 center{ 0.5f, 0.5f }; // 線が集まる点（UV）
        float strength = 0.0f;        // 線の濃さ（0.0で無効）
        float time = 0.0f;            // ちらつき用の経過時間[s]（Update が進める）
        float lineCount = 90.0f;      // 線の本数
        float innerRadius = 0.28f;    // この半径より内側には線を出さない（中心を隠さない）
        float aspect = 16.0f / 9.0f;  // 画面の横/縦
        float _pad0 = 0.0f;
    };

    SpeedLineData& GetEffectData() { return effectData_; }

private:
    void CreateEffectResource();

    uint32_t effectHandle_ = 0;
    SpeedLineData  effectData_;
    SpeedLineData* effectDataPtr_ = nullptr;
};
