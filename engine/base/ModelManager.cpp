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

void ModelManager::Initialize(DirectXManager* dxManager, SrvManager* srvManager)
{
	modelLoader = std::make_unique<ModelLoader>();
	modelLoader->Initialize(dxManager, srvManager);
}

void ModelManager::Finalize()
{
	delete instance;
	instance = nullptr;
}

void ModelManager::LoadModel(const std::string& directoryPath, const std::string& fileName)
{
	// 読み込み済みモデルを検索
	if (models.contains(fileName)) {
		// 読み込み済みなら早期return
		return;
	}

	std::unique_ptr<Model> model = std::make_unique<Model>();
	model->Initialize(modelLoader.get(), directoryPath, fileName);

	// モデルをmapコンテナに格納する
	models.insert(std::make_pair(fileName, std::move(model)));
}

Model* ModelManager::FindModel(const std::string& filePath)
{
	// 読み込み済みモデルを検索
	if (models.contains(filePath)) {
		// 読み込みモデルを戻り値としてreturn
		return models.at(filePath).get();
	}

	// ファイル名が一致しなければnull
	return nullptr;
}
