#pragma once
#include "Graphics/Device/DirectXManager.h"
#include "Graphics/Resource/SrvManager.h"
#include "ModelStructs.h"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
class ModelLoader
{
public: // メンバ関数
	void Initialize(DirectXManager* dxManager, SrvManager* srvManager);

	// モデルを読む関数
	static ModelData LoadModelFile(const std::string& filename);
	static SkinnedModelData LoadSkinnedModel(const std::string& filename);


	// ノードをモデルデータに変換する関数
	static Node ReadNode(aiNode* node);
	// ボーンの保持を確認する
	static bool HasBones(const aiScene* scene);
private:

	DirectXManager* dxManager_ = nullptr;
	SrvManager* srvManager_ = nullptr;

public: // ゲッター//セッター//
	DirectXManager* GetDxManager() const { return dxManager_; }
	SrvManager* GetSrvManager() const { return srvManager_; }
};

