#pragma once
#include "BaseRenderer.h"
#include "IDeferredDrawable.h"
#include "IShadowCaster.h"
#include <World3D/Object/Model/Model.h>
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

	void SetModel(const std::string& filePath);

#ifdef _DEBUG
	void DebugGui(size_t index) override;
#endif // DEBUG

	WorldTransform* GetWorldTransform() const override { return localTransform_.get(); }
	BaseModel* GetModel() const override { return model_; }

private:
	BaseModel* model_;
	BaseCamera* camera_ = nullptr; // Update()で毎フレーム更新
};

