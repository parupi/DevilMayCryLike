#pragma once
#include "BaseRenderer.h"
#include <3d/Object/Model/Model.h>
#include <3d/Camera/Camera.h>
#include <3d/Camera/CameraManager.h>
class ModelRenderer : public BaseRenderer
{
public:
	ModelRenderer(const std::string& renderName, const std::string& filePath);
	~ModelRenderer() = default;

	void Update(WorldTransform* parentTransform) override;

	void Draw() override;
	// GBufferに描画
	void DrawGBuffer();
	void SetModel(const std::string& filePath);

#ifdef _DEBUG
	void DebugGui(size_t index) override;
#endif // DEBUG


	WorldTransform* GetWorldTransform() const override { return localTransform_.get(); }

	BaseModel* GetModel() const override { return model_; }
private:
	BaseModel* model_;


	Camera* camera_ = CameraManager::GetInstance()->GetActiveCamera();
};

