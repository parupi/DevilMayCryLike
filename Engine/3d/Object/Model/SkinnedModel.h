#pragma once

#include "BaseModel.h"
#include "ModelLoader.h"
#include "Mesh/Mesh.h"
#include "Material/Material.h"
#include "Animation/Animation.h"
#include "Animation/SkinCluster.h"

class WorldTransform;
class Object3d;
class ModelRenderer;

class SkinnedModel : public BaseModel
{
public: // メンバ関数
	// 初期化
	void Initialize(ModelLoader* modelLoader, const std::string& fileName);

	void Update() override;
	// 描画
	void Draw() override;
	// CSによるSkinningの更新
	void UpdateSkinningWithCS();

	// GBufferに描画
	void DrawGBuffer() override;

	void DrawShadow() override;
#ifdef _DEBUG
	void DebugGui(ModelRenderer* render) override;
#endif // _DEBUG
private:
	std::vector<std::unique_ptr<Mesh>> meshes_;
	std::vector<std::unique_ptr<Material>> materials_;
	std::unique_ptr<Animation> animation_;
	std::unique_ptr<Skeleton> skeleton_;
	//std::vector<std::unique_ptr<SkinCluster>> skinClusters_;

	ModelLoader* modelLoader_ = nullptr;

	SkinnedModelData modelData_;

	std::string directoryPath_;

	// 現在のアニメーションタイム
	float animationTime = 0.0f;

public:
	SkinnedModelData GetModelData() { return modelData_; }
	Animation* GetAnimation() { return animation_.get(); }
	Skeleton* GetSkeleton() { return skeleton_.get(); }
	std::string GetDirectoryPath() { return directoryPath_; }
	DirectXManager* GetDxManager() { return modelLoader_->GetDxManager(); }
	SrvManager* GetSrvManager() { return modelLoader_->GetSrvManager(); }
	void SetLocalMatrix(const Matrix4x4& matrix) { modelData_.rootNode.localMatrix = matrix; }

	Material* GetMaterials(uint32_t index) { return materials_[index].get(); }
};

