#include "ModelManager.h"

ModelManager* ModelManager::instance = nullptr;
std::once_flag ModelManager::initInstanceFlag;

ModelManager* ModelManager::GetInstance()
{
	std::call_once(initInstanceFlag, []() {
		instance = new ModelManager();
		});
	return instance;
}

void ModelManager::Initialize(DirectXManager* dxManager)
{
	modelLoader = std::make_unique<ModelLoader>();
	modelLoader->Initialize(dxManager, dxManager->GetSrvManager());
}

void ModelManager::Finalize()
{
	delete instance;
	instance = nullptr;
}

void ModelManager::LoadModel(const std::string& fileName)
{
	// 読み込み済みモデルを検索
	if (models.contains(fileName)) {
		// 読み込み済みなら早期return
		return;
	}

	std::unique_ptr<Model> model = std::make_unique<Model>();
	model->Initialize(modelLoader.get(), fileName);

	// モデルをmapコンテナに格納する
	models.insert(std::make_pair(fileName, std::move(model)));
}

void ModelManager::LoadSkinnedModel(const std::string& filePath)
{
	// 読み込み済みモデルを検索
	if (skinnedModels.contains(filePath)) {
		// 読み込み済みなら早期return
		return;
	}

	std::unique_ptr<SkinnedModel> skinnedModel = std::make_unique<SkinnedModel>();
	skinnedModel->Initialize(modelLoader.get(), filePath);

	// モデルをmapコンテナに格納する
	skinnedModels.insert(std::make_pair(filePath, std::move(skinnedModel)));
}

BaseModel* ModelManager::FindModel(const std::string& fileName)
{
	// まず通常モデルから探す
	if (models.contains(fileName)) {
		return models.at(fileName).get();
	}
	// 次にスキンモデルから探す
	if (skinnedModels.contains(fileName)) {
		return skinnedModels.at(fileName).get();
	}
	return nullptr;
}