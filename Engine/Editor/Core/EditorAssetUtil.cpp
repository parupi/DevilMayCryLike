#include "EditorAssetUtil.h"
#ifdef _DEBUG

#include "EditorContext.h"
#include "World3D/Object/Model/ModelManager.h"

#include <algorithm>
#include <filesystem>

namespace {
	// <フォルダ>/<フォルダ名>.obj|.gltf|.fbx が実在するならモデル名として拾う。
	// 無いフォルダは種類分けの入れ物とみなして、その中をもう一段だけ掘る
	// （"Enemys/Skeleton" のような名前で ModelLoader に渡せる）。
	void CollectModelFolders(const std::filesystem::path& dir, const std::string& prefix,
		int remainingDepth, std::vector<std::string>& names) {
		std::error_code ec;
		for (const auto& entry : std::filesystem::directory_iterator(dir, ec)) {
			if (!entry.is_directory()) {
				continue;
			}
			const std::string folder = entry.path().filename().string();
			const std::string name = prefix.empty() ? folder : prefix + "/" + folder;

			// ModelLoader と同じ規約で本体ファイルの実在を確かめる
			const std::filesystem::path base = entry.path() / folder;
			bool found = false;
			for (const char* ext : { ".obj", ".gltf", ".fbx" }) {
				if (std::filesystem::exists(base.string() + ext)) {
					names.push_back(name);
					found = true;
					break;
				}
			}
			if (!found && remainingDepth > 0) {
				CollectModelFolders(entry.path(), name, remainingDepth - 1, names);
			}
		}
	}
}

std::vector<std::string> Editor::ScanModelFolders()
{
	std::vector<std::string> names;

	const std::filesystem::path root = "Resource/Models";
	std::error_code ec;
	if (!std::filesystem::is_directory(root, ec)) {
		return names;
	}

	CollectModelFolders(root, "", 1, names);

	std::sort(names.begin(), names.end());
	return names;
}

const std::vector<std::string>& Editor::CachedModelFolders(bool rescan)
{
	static std::vector<std::string> cache;
	static bool scanned = false;

	if (rescan || !scanned) {
		cache = ScanModelFolders();
		scanned = true;
	}
	return cache;
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
