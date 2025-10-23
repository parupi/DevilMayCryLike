#pragma once
#include <math/Vector3.h>
#include <math/Vector4.h>
#include <string>
#include <base/DirectXManager.h>

class BaseLight {
public:
    BaseLight(const std::string& name);
    virtual ~BaseLight();
    // 初期化処理
    virtual void Initialize(DirectXManager* dxManager);
    // 更新処理
    virtual void Update() = 0;
    // リソースの生成
    virtual void CreateLightResource() = 0;
    // リソースの更新
    virtual void UpdateLightResource() = 0;
    // 構造体のサイズを取得
    virtual size_t GetDataSize() const = 0;
    // シリアライズする
    virtual void SerializeTo(void* dest) const = 0;
    // 名前を取得
    const std::string& GetName() const { return name_; }
    // 変更があったかどうかを取得
    bool IsDirty() const { return isDirty_; }
    // 変更を記録
    void MarkDirty() { isDirty_ = true; }
    // 変更をなくす
    void ClearDirty() { isDirty_ = false; }
protected:
    std::string name_;
    Microsoft::WRL::ComPtr<ID3D12Resource> resource_;
    DirectXManager* dxManager_ = nullptr;
    bool isDirty_ = true; // 値が変更されたら true
};