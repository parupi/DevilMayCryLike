#include "EditorAssetUtil.h"
#ifdef _DEBUG

#include "EditorContext.h"
#include "World3D/Object/Model/ModelManager.h"

#include <algorithm>
#include <filesystem>

std::vector<std::string> Editor::ScanModelFolders()
{
	std::vector<std::string> names;

	const std::filesystem::path root = "Resource/Models";
	std::error_code ec;
	if (!std::filesystem::is_directory(root, ec)) {
		return names;
	}

	for (const auto& entry : std::filesystem::directory_iterator(root, ec)) {
		if (!entry.is_directory()) {
			continue;
		}
		const std::string name = entry.path().filename().string();
		// ModelLoader と同じ規約で本体ファイルの実在を確かめる
		const std::filesystem::path base = entry.path() / name;
		for (const char* ext : { ".obj", ".gltf", ".fbx" }) {
			if (std::filesystem::exists(base.string() + ext)) {
				names.push_back(name);
				break;
			}
		}
	}

	std::sort(names.begin(), names.end());
	return names;
}

std::string Editor::FindLoadedModelName(const BaseModel* model)
{
	if (!model) {
		return {};
	}
	ModelManager* manager = Ctx().modelManager;
	if (!manager) {
		return {};
	}

	for (const auto& [name, entry] : manager->models) {
		if (entry.get() == model) {
			return name;
		}
	}
	for (const auto& [name, entry] : manager->skinnedModels) {
		if (entry.get() == model) {
			return name;
		}
	}
	return {};
}

#endif // _DEBUG
