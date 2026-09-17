#include "StageDocument.h"
#include "SceneLoader.h"

#include <algorithm>
#include <filesystem>

namespace {

std::string g_path = SceneLoader::kDefaultStagePath;

} // namespace

const std::string& StageDocument::GetPath()
{
	return g_path;
}

void StageDocument::SetPath(const std::string& path)
{
	if (path.empty()) {
		return;
	}
	g_path = path;
}

std::string StageDocument::GetName()
{
	return std::filesystem::path(g_path).stem().string();
}

std::string StageDocument::MakePath(const std::string& name)
{
	return std::string(kDirectory) + "/" + name + kExtension;
}

std::vector<std::string> StageDocument::ListNames()
{
	std::vector<std::string> names;

	std::error_code ec;
	if (!std::filesystem::is_directory(kDirectory, ec)) {
		return names;
	}
	for (const auto& entry : std::filesystem::directory_iterator(kDirectory, ec)) {
		if (!entry.is_regular_file() || entry.path().extension() != kExtension) {
			continue;
		}
		names.push_back(entry.path().stem().string());
	}

	std::sort(names.begin(), names.end());
	return names;
}

bool StageDocument::IsValidName(const std::string& name)
{
	if (name.empty()) {
		return false;
	}
	// ディレクトリを跨がせない。Resource/Stage の直下だけを扱う
	return name.find_first_of("/\\:*?\"<>|") == std::string::npos;
}

bool StageDocument::Exists(const std::string& name)
{
	std::error_code ec;
	return std::filesystem::is_regular_file(MakePath(name), ec);
}
