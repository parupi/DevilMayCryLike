#pragma once
#include <string>
#include "LightStructs.h"

enum class LightType {
    Directional,
    Point,
    Spot,
};

class BaseLight {
public:
    BaseLight() = default;
    virtual ~BaseLight() = default;
    // 初期化処理
    virtual void Initialize() = 0;
    // 更新処理
    virtual void Update() = 0;
    // 名前を取得
    const std::string& GetName() const { return name_; }
    // ライトの情報を取得
    const LightData& GetLightData() const { return lightData_; }
protected:
    // ライトの名前
    std::string name_;
    // ライトの情報
    LightData lightData_{};
};