#include "SceneSaver.h"

#include <fstream>
#include <nlohmann/json.hpp>

#include "Utility/Logger.h"
#include "World3D/Collider/BaseCollider.h"
#include "World3D/Collider/OBBCollider.h"
#include "World3D/Collider/SphereCollider.h"
#include "World3D/Object/Object3d.h"
#include "World3D/Object/Object3dManager.h"
#include "World3D/WorldTransform.h"

#include "GameObject/Event/BaseEvent.h"
#include "GameObject/Light/StagePointLight.h"
#include "GameObject/Prop/Prop.h"

// 書き出しは挿入順を保つ ordered_json を使う。
// 既定の json は キーを辞書順に並べ替えてしまい、差分が読みにくくなるため
using json = nlohmann::ordered_json;

namespace {

	json ToJson(const Vector3& v) {
		return json::array({ v.x, v.y, v.z });
	}

	json ToJson(const Quaternion& q) {
		return json::array({ q.x, q.y, q.z, q.w });
	}

	const char* ToString(EventType type) {
		switch (type) {
		case EventType::EnemySpawn:  return "EnemySpawn";
		case EventType::Clear:       return "Clear";
		case EventType::ForceBattle: return "ForceBattle";
		case EventType::BossSpawn:   return "BossSpawn";
		}
		return "";
	}

	// オブジェクトに付いている最初のコライダーを拾う。
	// ステージデータは1オブジェクト1コライダーなので先頭だけでよい
	std::optional<ColliderInfo> CaptureCollider(Object3d* object) {
		for (BaseCollider* collider : object->GetColliders()) {
			if (!collider || !collider->isAlive) {
				continue;
			}
			if (auto* obb = dynamic_cast<OBBCollider*>(collider)) {
				ColliderInfo info;
				info.shape = ColliderShape::OBB;
				info.offset = obb->GetColliderData().offset;
				info.halfExtents = obb->GetColliderData().halfExtents;
				info.isActive = obb->GetColliderData().isActive;
				return info;
			}
			if (auto* sphere = dynamic_cast<SphereCollider*>(collider)) {
				ColliderInfo info;
				info.shape = ColliderShape::Sphere;
				info.offset = sphere->GetColliderData().offset;
				info.radius = sphere->GetColliderData().radius;
				info.isActive = sphere->GetColliderData().isActive;
				return info;
			}
		}
		return std::nullopt;
	}

	std::optional<LightInfo> CaptureLight(Object3d* object) {
		if (auto* stageLight = dynamic_cast<StagePointLight*>(object)) {
			LightInfo info;
			info.color = stageLight->GetLightColor();
			info.offset = stageLight->GetLightOffset();
			info.intensity = stageLight->GetLightIntensity();
			info.radius = stageLight->GetLightRadius();
			info.decay = stageLight->GetLightDecay();
			return info;
		}
		if (auto* prop = dynamic_cast<Prop*>(object)) {
			if (!prop->HasLight()) {
				return std::nullopt;
			}
			LightInfo info;
			info.color = prop->GetLightColor();
			info.offset = prop->GetLightOffset();
			info.intensity = prop->GetLightIntensity();
			info.radius = prop->GetLightRadius();
			info.decay = prop->GetLightDecay();
			return info;
		}
		return std::nullopt;
	}

	std::optional<EventInfo> CaptureEvent(Object3d* object) {
		auto* event = dynamic_cast<BaseEvent*>(object);
		if (!event) {
			return std::nullopt;
		}
		EventInfo info;
		info.type = ToString(event->GetType());
		info.targets = event->GetTargetNames();
		return info;
	}

	json ToJson(const SceneObject& object) {
		json j;
		j["name"] = object.name;
		j["class"] = object.className;
		j["translate"] = ToJson(object.translate);
		j["rotate"] = ToJson(object.rotate);
		j["scale"] = ToJson(object.scale);

		if (object.modelName.has_value()) {
			j["model"] = object.modelName.value();
		}

		if (object.collider.has_value()) {
			const ColliderInfo& col = object.collider.value();
			json c;
			c["shape"] = (col.shape == ColliderShape::Sphere) ? "Sphere" : "OBB";
			c["offset"] = ToJson(col.offset);
			if (col.shape == ColliderShape::Sphere) {
				c["radius"] = col.radius;
			} else {
				c["halfExtents"] = ToJson(col.halfExtents);
			}
			if (!col.isActive) {
				c["isActive"] = false;
			}
			j["collider"] = c;
		}

		if (object.lightInfo.has_value()) {
			const LightInfo& li = object.lightInfo.value();
			json l;
			l["color"] = ToJson(li.color);
			l["offset"] = ToJson(li.offset);
			l["intensity"] = li.intensity;
			l["radius"] = li.radius;
			l["decay"] = li.decay;
			j["light"] = l;
		}

		if (object.eventInfo.has_value()) {
			const EventInfo& ev = object.eventInfo.value();
			json e;
			e["type"] = ev.type;
			e["targets"] = ev.targets;
			j["event"] = e;
		}

		return j;
	}

} // namespace

SceneObject SceneSaver::CaptureObject(Object3d* object) {
	SceneObject out;
	out.name = object->name_;
	out.className = object->GetClassName();

	WorldTransform* transform = object->GetWorldTransform();
	out.translate = transform->GetTranslation();
	out.rotate = transform->GetRotation();
	out.scale = transform->GetScale();

	const std::string modelName = object->GetModelName();
	if (!modelName.empty()) {
		out.modelName = modelName;
	}

	out.collider = CaptureCollider(object);
	out.lightInfo = CaptureLight(object);
	out.eventInfo = CaptureEvent(object);

	return out;
}

std::vector<SceneObject> SceneSaver::Capture() {
	std::vector<SceneObject> objects;

	for (Object3d* object : Object3dManager::GetInstance().GetAllObject()) {
		if (!object || !object->isAlive || !object->IsStageObject()) {
			continue;
		}
		objects.push_back(CaptureObject(object));
	}

	return objects;
}

bool SceneSaver::Save(const std::string& path) {
	const std::vector<SceneObject> objects = Capture();

	json root;
	root["format"] = SceneLoader::kFormatName;
	root["version"] = SceneLoader::kFormatVersion;
	root["objects"] = json::array();
	for (const SceneObject& object : objects) {
		root["objects"].push_back(ToJson(object));
	}

	std::ofstream file(path);
	if (!file) {
		Logger::Log("[SceneSaver] ステージデータを書き出せませんでした: " + path);
		return false;
	}
	file << root.dump(4) << std::endl;

	Logger::Log("[SceneSaver] " + std::to_string(objects.size()) + " 個のオブジェクトを保存しました: " + path);
	return true;
}
