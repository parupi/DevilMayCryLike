#define NOMINMAX
#include "SceneBuilder.h"
#include "World3D/Object/Object3d.h"
#include "World3D/WorldTransform.h"
#include "World3D/Collider/SphereCollider.h"
#include "World3D/Collider/OBBCollider.h"
#include "World3D/Collider/CollisionManager.h"
#include "World3D/Object/Model/ModelManager.h"
#include "World3D/Object/Renderer/ModelRenderer.h"
#include "World3D/Object/Renderer/RendererManager.h"
#include "Math/Quaternion.h"
#include "Scene/Object3dFactory.h"
#include "GameObject/Character/Enemy/Enemy.h"
#include "GameObject/Event/EnemySpawnEvent.h"
#include "GameObject/Event/ClearEvent.h"
#include "GameObject/Event/ForceBattleEvent.h"
#include "GameObject/Event/BossSpawnEvent.h"
#include "GameObject/Ground/Ground.h"
#include "GameObject/Prop/Prop.h"
#include "GameObject/Light/StagePointLight.h"

// ---------------------------------------------------------------------------
// helpers
// ---------------------------------------------------------------------------

bool SceneBuilder::IsEvent(const SceneObject& obj) {
	return obj.className.rfind("Event_", 0) == 0;
}

void SceneBuilder::ApplyTransform(WorldTransform* transform, const SceneObject& src) {
	// ステージデータはエンジン空間で保存されているので変換は不要
	transform->GetTranslation() = src.translate;
	transform->GetRotation() = src.rotate;
	transform->GetScale() = src.scale;
}

void SceneBuilder::ApplyCollider(Object3d* object, const std::string& name, const ColliderInfo& col) {
	if (col.shape == ColliderShape::OBB) {
		// 回転しても正しく機能するようOBBコライダーとして生成する。
		// スケールは OBBCollider::Update() がオーナーの WorldTransform から自動で掛けるので
		// ここでは掛けない
		auto collider = std::make_unique<OBBCollider>(name);
		OBBData data;
		data.offset = col.offset;
		data.halfExtents = col.halfExtents;
		data.isActive = col.isActive;
		collider->GetColliderData() = data;
		BaseCollider* ptr = collider.get();
		CollisionManager::GetInstance().AddCollider(std::move(collider));
		object->AddCollider(ptr);

	} else if (col.shape == ColliderShape::Sphere) {
		auto collider = std::make_unique<SphereCollider>(name);
		SphereData data;
		data.offset = col.offset;
		data.radius = col.radius;
		data.isActive = col.isActive;
		collider->GetColliderData() = data;
		BaseCollider* ptr = collider.get();
		CollisionManager::GetInstance().AddCollider(std::move(collider));
		object->AddCollider(ptr);
	}
}

// ---------------------------------------------------------------------------
// public entry point
// ---------------------------------------------------------------------------

void SceneBuilder::BuildScene(const std::vector<SceneObject>& sceneObjects) {
	std::vector<SceneObject> pendingEvents;

	// Pass 1: 通常オブジェクトを生成
	for (const auto& sceneObj : sceneObjects) {
		BuildObject(sceneObj, pendingEvents);
	}

	// Pass 2: イベントを生成（参照する敵が先に Object3dManager に存在している必要がある）
	for (const auto& eventObj : pendingEvents) {
		BuildEvent(eventObj);
	}
}

// ---------------------------------------------------------------------------
// private builders
// ---------------------------------------------------------------------------

void SceneBuilder::BuildObject(const SceneObject& sceneObj, std::vector<SceneObject>& outPendingEvents) {
	if (IsEvent(sceneObj)) {
		outPendingEvents.push_back(sceneObj);
		return;
	}

	auto object = Object3dFactory::Create(sceneObj.className, sceneObj.name);
	// ステージから作ったものは保存対象
	object->SetStageObject(true);

	// 使用するモデル名（Ground / Prop などモデルを差し替えられるクラスにだけ効く）
	if (sceneObj.modelName.has_value()) {
		object->SetModelName(sceneObj.modelName.value());
	}

	// 小物(Prop)のオプションのポイントライト（ランタンなど）
	if (auto* prop = dynamic_cast<Prop*>(object.get())) {
		if (sceneObj.lightInfo.has_value()) {
			const LightInfo& li = sceneObj.lightInfo.value();
			prop->SetLight(li.color, li.offset, li.intensity, li.radius, li.decay);
		}
	}

	// ステージに配置したポイントライト
	if (auto* stageLight = dynamic_cast<StagePointLight*>(object.get())) {
		if (sceneObj.lightInfo.has_value()) {
			const LightInfo& li = sceneObj.lightInfo.value();
			stageLight->SetLight(li.color, li.offset, li.intensity, li.radius, li.decay);
		}
	}

	ApplyTransform(object->GetWorldTransform(), sceneObj);

	if (sceneObj.collider) {
		ApplyCollider(object.get(), sceneObj.name, sceneObj.collider.value());
	}

	object->Initialize();

	// Ground / Prop は Initialize() の中でレンダラーを作るが、
	// 素の Object3d は作らない。エディタでモデルを貼ったものはここで復元する
	if (object->GetRenderers().empty() && !object->GetModelName().empty()) {
		const std::string& modelName = object->GetModelName();
		ModelManager::GetInstance().LoadModel(modelName);

		auto renderer = std::make_unique<ModelRenderer>(sceneObj.name, modelName);
		BaseRenderer* rawRenderer = renderer.get();
		RendererManager::GetInstance().AddRenderer(std::move(renderer));
		object->AddRenderer(rawRenderer);
	}

	Object3dManager::GetInstance().AddObject(std::move(object));
}

void SceneBuilder::BuildEvent(const SceneObject& sceneObj) {
	auto object = Object3dFactory::Create(sceneObj.className, sceneObj.name);
	// "Event_" で始まるのにイベントクラスとして登録されていない場合はここで弾く
	auto* eventObject = dynamic_cast<BaseEvent*>(object.get());
	if (!eventObject) return;

	eventObject->SetStageObject(true);

	if (sceneObj.eventInfo.has_value()) {
		const EventInfo& info = sceneObj.eventInfo.value();
		// 保存時に書き戻すための対象名。実行中の状態には左右されない
		eventObject->SetTargetNames(info.targets);

		if (info.type == "EnemySpawn") {
			auto* spawnEvent = dynamic_cast<EnemySpawnEvent*>(eventObject);
			if (spawnEvent) {
				for (const auto& targetName : info.targets) {
					auto* enemy = dynamic_cast<Enemy*>(
						Object3dManager::GetInstance().FindObject(targetName));
					if (enemy) {
						enemy->SetActive(false);
						spawnEvent->AddEnemy(enemy);
					}
				}
			}
		} else if (info.type == "Clear") {
			auto* clearEvent = dynamic_cast<ClearEvent*>(eventObject);
			if (clearEvent) {
				for (const auto& targetName : info.targets) {
					auto* enemy = dynamic_cast<Enemy*>(
						Object3dManager::GetInstance().FindObject(targetName));
					if (enemy) {
						clearEvent->AddTargetEnemy(enemy);
					}
				}
			}
		} else if (info.type == "ForceBattle") {
			auto* battleEvent = dynamic_cast<ForceBattleEvent*>(eventObject);
			if (battleEvent) {
				for (const auto& targetName : info.targets) {
					auto* enemy = dynamic_cast<Enemy*>(
						Object3dManager::GetInstance().FindObject(targetName));
					if (enemy) {
						enemy->SetActive(false); // 発動まで待機させる
						battleEvent->AddEnemy(enemy);
					}
				}
			}
		} else if (info.type == "BossSpawn") {
			auto* bossEvent = dynamic_cast<BossSpawnEvent*>(eventObject);
			if (bossEvent && !info.targets.empty()) {
				const std::string& bossName = info.targets.front();
				auto* boss = dynamic_cast<Enemy*>(
					Object3dManager::GetInstance().FindObject(bossName));
				if (boss) {
					boss->SetActive(false); // 発動まで待機させる
					bossEvent->SetBossName(bossName);
				}
			}
		}
	}

	ApplyTransform(eventObject->GetWorldTransform(), sceneObj);

	if (sceneObj.collider) {
		ApplyCollider(eventObject, sceneObj.name, sceneObj.collider.value());
	}

	eventObject->Initialize();
	Object3dManager::GetInstance().AddObject(std::move(object));
}
