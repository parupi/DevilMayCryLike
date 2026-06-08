#include "Object3dFactory.h"
#include "GameObject/Character/Enemy/Hellkaina/Hellkaina.h"
#include "GameObject/Character/Player/Player.h"
#include "GameObject/Ground/Ground.h"

std::unordered_map<std::string, Object3dFactory::Creator>& Object3dFactory::Registry() {
	static std::unordered_map<std::string, Creator> registry;
	return registry;
}

void Object3dFactory::Register(const std::string& className, Creator creator) {
	Registry()[className] = std::move(creator);
}

std::unique_ptr<Object3d> Object3dFactory::Create(const std::string& className, const std::string& objectName) {
	auto& reg = Registry();
	auto it = reg.find(className);
	if (it != reg.end()) {
		return it->second(objectName);
	}
	return std::make_unique<Object3d>(objectName);
}
