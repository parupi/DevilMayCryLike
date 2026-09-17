#include "SceneLoader.h"
#include <fstream>
#include <stdexcept>

using json = nlohmann::json;

namespace {

	Vector3 ReadVector3(const json& j, const Vector3& fallback) {
		if (!j.is_array() || j.size() < 3) {
			return fallback;
		}
		return { j[0].get<float>(), j[1].get<float>(), j[2].get<float>() };
	}

	Quaternion ReadQuaternion(const json& j) {
		if (!j.is_array() || j.size() < 4) {
			return Quaternion(); // 単位クォータニオン
		}
		return Quaternion(j[0].get<float>(), j[1].get<float>(), j[2].get<float>(), j[3].get<float>());
	}

} // namespace

std::vector<SceneObject> SceneLoader::Load(const std::string& path) {
	std::ifstream file(path);
	if (!file) {
		throw std::runtime_error("ステージデータを開けませんでした: " + path);
	}

	json root;
	file >> root;

	// 旧フォーマット（Blenderのレベルエディタが吐いていたもの）を黙って読むと
	// 座標系が違うまま生成されて原因が分かりにくいので、はっきり弾く
	const std::string format = root.value("format", "");
	const int version = root.value("version", 0);
	if (format != kFormatName || version != kFormatVersion) {
		throw std::runtime_error(
			"ステージデータの形式が違います: " + path +
			" (format=\"" + format + "\" version=" + std::to_string(version) +
			" / 期待: format=\"" + kFormatName + "\" version=" + std::to_string(kFormatVersion) + ")");
	}

	std::vector<SceneObject> objects;
	if (!root.contains("objects")) {
		return objects;
	}
	for (const auto& objectJson : root["objects"]) {
		objects.push_back(ParseObject(objectJson));
	}
	return objects;
}

SceneObject SceneLoader::ParseObject(const json& j) {
	SceneObject out;
	out.name = j.value("name", "");
	out.className = j.value("class", "Object3d");

	out.translate = ReadVector3(j.value("translate", json::array()), { 0.0f, 0.0f, 0.0f });
	out.rotate = ReadQuaternion(j.value("rotate", json::array()));
	out.scale = ReadVector3(j.value("scale", json::array()), { 1.0f, 1.0f, 1.0f });

	if (j.contains("model")) {
		out.modelName = j["model"].get<std::string>();
	}

	if (j.contains("collider")) {
		out.collider = ParseCollider(j["collider"]);
	}

	if (j.contains("light")) {
		const auto& l = j["light"];
		LightInfo info;
		info.color = ReadVector3(l.value("color", json::array()), info.color);
		info.offset = ReadVector3(l.value("offset", json::array()), info.offset);
		info.intensity = l.value("intensity", info.intensity);
		info.radius = l.value("radius", info.radius);
		info.decay = l.value("decay", info.decay);
		out.lightInfo = info;
	}

	if (j.contains("event")) {
		const auto& e = j["event"];
		EventInfo info;
		info.type = e.value("type", "");
		if (e.contains("targets")) {
			for (const auto& target : e["targets"]) {
				info.targets.push_back(target.get<std::string>());
			}
		}
		out.eventInfo = info;
	}

	return out;
}

ColliderInfo SceneLoader::ParseCollider(const json& j) {
	ColliderInfo info;

	const std::string shape = j.value("shape", "OBB");
	if (shape == "Sphere") {
		info.shape = ColliderShape::Sphere;
		info.radius = j.value("radius", info.radius);
	} else {
		info.shape = ColliderShape::OBB;
		info.halfExtents = ReadVector3(j.value("halfExtents", json::array()), info.halfExtents);
	}

	info.offset = ReadVector3(j.value("offset", json::array()), info.offset);
	info.isActive = j.value("isActive", true);

	return info;
}
