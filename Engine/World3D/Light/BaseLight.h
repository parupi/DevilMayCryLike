#pragma once
#include <string>
#include "LightStructs.h"

enum class LightType {
    Directional,
    Point,
    Spot,
};

class PrimitiveLineDrawer;

class BaseLight {
public:
    BaseLight() = default;
    virtual ~BaseLight() = default;
    // 初期化処理
    virtual void Initialize() = 0;
    // 更新処理
    virtual void Update() = 0;

#ifdef _DEBUG
    // エディター描画
    virtual void DrawLightEditor() = 0;
    // デバッグ描画
    virtual void DrawDebug(PrimitiveLineDrawer* drawer) = 0;
#endif // DEBUG

    // 名前を取得
    const std::string& GetName() const { return name_; }
    // ライトの情報を取得
    const LightData& GetLightData() const { return lightData_; }
    // 空気中で光が散る（ボリューメトリックライト）量の倍率。既定 0 = 散らない。
    // LightData の空き（padding[0]）に入れて GPU へ渡す
    void SetVolumetricScale(float scale) { lightData_.padding[0] = scale; }
    float GetVolumetricScale() const { return lightData_.padding[0]; }
    // 埃などのパーティクルを照らす倍率。既定 0 = 照らさない（padding[1]）
    void SetParticleLightingScale(float scale) { lightData_.padding[1] = scale; }
    float GetParticleLightingScale() const { return lightData_.padding[1]; }
    // スポットライトの内側の角度の cos（padding[2]）。ここより内側は最も明るく、cosAngle（外側）へ向けて暗くなる。
    // cosAngle 以下（既定 0）なら従来どおり smoothstep(cosAngle, 1) で中心だけが最も明るい
    void SetSpotInnerCos(float innerCos) { lightData_.padding[2] = innerCos; }
    float GetSpotInnerCos() const { return lightData_.padding[2]; }
protected:
    // ライトの名前
    std::string name_;
    // ライトの情報
    LightData lightData_{};
};