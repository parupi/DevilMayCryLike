#pragma once
#include "BaseRenderer.h"
#include "IDeferredDrawable.h"
#include "IShadowCaster.h"
#include <World3D/Object/Model/Model.h>
#include <World3D/Object/Model/Animation/SkinnedInstance.h>
#include "World3D/Camera/BaseCamera.h"
#include <World3D/Camera/CameraManager.h>

class ModelRenderer : public BaseRenderer, public IDeferredDrawable, public IShadowCaster
{
public:
	ModelRenderer(const std::string& renderName, const std::string& filePath);
	~ModelRenderer() = default;

	void Update(WorldTransform* parentTransform) override;
	void Draw() override;
	void DrawGBuffer() override;
	void DrawShadow() override;
	bool GetShadowBoundingSphere(Vector3& outCenter, float& outRadius) const override;

	void SetModel(const std::string& filePath);

#ifdef _DEBUG
	void DebugGui(size_t index) override;
#endif // DEBUG

	WorldTransform* GetWorldTransform() const override { return localTransform_.get(); }
	BaseModel* GetModel() const override { return model_; }
	SkinnedInstance* GetSkinnedInstance() const override { return skinnedInstance_.get(); }

	// スキンモデルのときだけ有効。アニメーションの再生はここから行う
	AnimationPlayer* GetAnimationPlayer() const;

private:
	BaseModel* model_;
	BaseCamera* camera_ = nullptr; // Update()で毎フレーム更新

	// スキンモデルのときだけ生成される、このレンダラー専用のポーズと変形後頂点。
	// モデル本体は ModelManager が共有しているので、可変状態はこちら側に置く
	std::unique_ptr<SkinnedInstance> skinnedInstance_;
};

