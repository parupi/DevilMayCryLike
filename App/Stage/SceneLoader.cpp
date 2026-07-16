#include "SceneLoader.h"
#include <fstream>
#include <stdexcept>

using json = nlohmann::json;

std::vector<SceneObject> SceneLoader::Load(const std::string& path) {
	std::ifstream file(path);
	if (!file) {
		throw std::runtime_error("Failed to open JSON file: " + path);
	}

	json root;
	file >> root;

	std::vector<SceneObject> objects;
	for (const auto& obj : root["objects"]) {
		SceneObject sceneObj;
		ParseObject(obj, sceneObj);
		objects.push_back(sceneObj);
	}

	return objects;
}

void SceneLoader::ParseObject(const json& j, SceneObject& out) {
	out.name = j["name"];
	out.className = j.value("class", "Object3d");

	const auto& t = j["transform"];
	out.transform.translate = {t["translation"][0], t["translation"][1], t["translation"][2]};
	out.transform.rotate = {t["rotation"][0],    t["rotation"][1],    t["rotation"][2]};
	out.transform.scale = {t["scaling"][0],     t["scaling"][1],     t["scaling"][2]};

	if (j.contains("file_name")) {
		out.fileName = j["file_name"].get<std::string>();
	}

	if (j.contains("collider")) {
		out.collider = ParseCollider(j["collider"]);
	}

	if (j.contains("event")) {
		EventInfo info;
		const auto& ev = j["event"];

		info.type = ev.value("type", "");
		info.trigger = ev.value("trigger", "");

		if ((info.type == "EnemySpawn" || info.type == "ForceBattle") && ev.contains("enemies")) {
			for (const auto& e : ev["enemies"]) {
				info.enemies.push_back({e.value("name", ""), e.value("delay", 0.0f)});
			}
		} else if (info.type == "BossSpawn") {
			info.bossName = ev.value("boss", "");
		} else if (info.type == "Clear" && ev.contains("conditions")) {
			for (const auto& c : ev["conditions"]) {
				EventCondition cond;
				cond.type = c.value("type", "");
				if (c.contains("targets")) {
					for (const auto& target : c["targets"]) {
						cond.targets.push_back(target.get<std::string>());
					}
				}
				info.conditions.push_back(cond);
			}
		}

		out.eventInfo = info;
	}
}

Collider SceneLoader::ParseCollider(const json& j) {
	Collider col;
	const std::string type = j["type"];

	if (type == "BOX") {
		col.type = ColliderType::AABB;

		const Vector3 center = {j["center"][0], j["center"][1], j["center"][2]};
		const Vector3 half = {j["size"][0] * 0.5f, j["size"][1] * 0.5f, j["size"][2] * 0.5f};

		col.aabb.offsetMin = center - half;
		col.aabb.offsetMax = center + half;

	} else if (type == "SPHERE") {
		col.type = ColliderType::Sphere;
		col.sphere.offset = {j["center"][0], j["center"][1], j["center"][2]};
		col.sphere.radius = j["size"][0] * 0.5f;
	}

	return col;
}
