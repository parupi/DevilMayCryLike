#pragma once
#include <map>
#include <memory>
#include <string>
#include "Model.h"
#include "SkinnedModel.h"
#include <mutex>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include "Graphics/Resource/TextureManager.h"

class ModelManager
{
private:
	ModelManager() = default;
	ModelManager(const ModelManager&) = delete;
	ModelManager& operator=(const ModelManager&) = delete;
public:
	// シングルトンインスタンスの取得
	static ModelManager& GetInstance();
	// 初期化
	void Initialize(DirectXManager* dxManager);
	// 終了
	void Finalize();
public:
	// モデルファイル読み込み
	void LoadModel(const std::string& filePath);
	void LoadSkinnedModel(const std::string& filePath);
	// モデルの検索
	BaseModel* FindModel(const std::string& fileName);
	// モデルデータ
	std::map<std::string, std::unique_ptr<Model>> models;
	std::map<std::string, std::unique_ptr<SkinnedModel>> skinnedModels;

private:

	std::unique_ptr<ModelLoader> modelLoader = nullptr;

public:
	ModelLoader* GetModelLoader() {return modelLoader.get();}
};

