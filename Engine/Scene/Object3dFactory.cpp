#include "Object3dFactory.h"
#include <algorithm>

std::unordered_map<std::string, Object3dFactory::Entry>& Object3dFactory::Registry() {
	static std::unordered_map<std::string, Entry> registry;
	return registry;
}

void Object3dFactory::Register(const std::string& className, Creator creator, bool usesModelName) {
	Registry()[className] = Entry{ std::move(creator), usesModelName };
}

std::unique_ptr<Object3d> Object3dFactory::Create(const std::string& className, const std::string& objectName) {
	auto& registry = Registry();
	auto it = registry.find(className);

	std::unique_ptr<Object3d> object = (it != registry.end())
		? it->second.creator(objectName)
		: std::make_unique<Object3d>(objectName);

	// 保存時に書き出す型名。生成経路をここ1本に絞っておけば入れ忘れが起きない
	object->SetClassName(className);
	return object;
}

std::vector<std::string> Object3dFactory::GetClassNames() {
	std::vector<std::string> names;
	names.reserve(Registry().size());
	for (const auto& [className, entry] : Registry()) {
		names.push_back(className);
	}
	std::sort(names.begin(), names.end());
	return names;
}

bool Object3dFactory::UsesModelName(const std::string& className) {
	auto it = Registry().find(className);
	return it != Registry().end() && it->second.usesModelName;
}
