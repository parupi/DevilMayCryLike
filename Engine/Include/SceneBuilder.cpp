#define NOMINMAX
#include "SceneBuilder.h"
#include "3d/Object/Object3d.h"
#include "3d/WorldTransform.h"
#include "3d/Object/Renderer/ModelRenderer.h"
#include "3d/Object/Model/ModelManager.h"
#include "3d/Collider/AABBCollider.h"
#include "3d/Collider/SphereCollider.h"
#include <3d/Collider/CollisionManager.h>
#include <3d/Object/Renderer/RendererManager.h>
#include "math/Vector3.h"
#include <scene/Object3dFactory.h>
#include <GameObject/Event/EventManager.h>
#include <GameObject/Event/EventFactory.h>
#include <GameObject/Event/EnemySpawnEvent.h>
#include <GameObject/Event/ClearEvent.h>


std::vector<const SceneObject*> SceneBuilder::pendingEvents_;

void SceneBuilder::BuildScene(const std::vector<SceneObject>& sceneObjects) {
	for (const auto& obj : sceneObjects) {
		std::unique_ptr<Object3d> object = BuildObjectRecursive(obj, nullptr);
		if (obj.className.find("Event_")) {
			Object3dManager::GetInstance()->AddObject(std::move(object));
		}
	}

	// 最後にイベントを生成
	BuildPendingEvents();
}

std::unique_ptr<Object3d> SceneBuilder::BuildObjectRecursive(const SceneObject& sceneObj, Object3d* parent) {
	std::unique_ptr<Object3d> object;

	// イベントは最後にまとめて生成
	if (sceneObj.className.find("Event_") == 0) {
		pendingEvents_.push_back(&sceneObj);
		return nullptr;
	}

	object = Object3dFactory::Create(sceneObj.className, sceneObj.name_);

	// トランスフォーム設定
	WorldTransform* transform = object->GetWorldTransform();
	// Translation の Y と Z を入れ替えてから代入
	Vector3 tempTranslate = sceneObj.transform.translate;
	std::swap(tempTranslate.y, tempTranslate.z);
	transform->GetTranslation() = tempTranslate;

	// Scale の Y と Z を入れ替えてから代入
	Vector3 tempScale = sceneObj.transform.scale;
	std::swap(tempScale.y, tempScale.z);
	transform->GetScale() = tempScale;

	//transform->GetRotation() = EulerDegree(sceneObj.transform.rotate);

	if (parent) {
		transform->SetParent(parent->GetWorldTransform());
	}

	// --- Colliderの生成と登録 ---
	if (sceneObj.collider) {
		const Collider& col = sceneObj.collider.value();

		if (col.type == ColliderType::AABB) {
			std::unique_ptr<AABBCollider> collider = std::make_unique<AABBCollider>(sceneObj.name_);
			AABBData scaledAABB = col.aabb;

			// スケールを適用
			scaledAABB.offsetMin *= object->GetWorldTransform()->GetScale() * 2.0f;
			scaledAABB.offsetMax *= object->GetWorldTransform()->GetScale() * 2.0f;

			collider->GetColliderData() = scaledAABB;

			BaseCollider* colPtr = collider.get();
			CollisionManager::GetInstance()->AddCollider(std::move(collider));
			object->AddCollider(colPtr);
		} else if (col.type == ColliderType::Sphere) {
			std::unique_ptr<SphereCollider> collider = std::make_unique<SphereCollider>(sceneObj.name_);

			SphereData scaledSphere = col.sphere;

			Vector3 scale = object->GetWorldTransform()->GetScale();
			// XYZの中で最大のスケールを使う（非均一スケール対応）
			//float maxScale = std::max(scale.x, scale.y, scale.z);
			//scaledSphere.radius *= maxScale;

			// offset もスケールに合わせて調整
			scaledSphere.offset *= scale;
			// 設定したデータを入れる
			collider->GetColliderData() = scaledSphere;
			// ポインタを取得しておく
			BaseCollider* colPtr = collider.get();
			CollisionManager::GetInstance()->AddCollider(std::move(collider));

			object->AddCollider(colPtr);
		}
	}

	object->Initialize();

	// 子オブジェクトの構築
	for (const auto& child : sceneObj.children) {
		BuildObjectRecursive(child, object.get());
	}

	return object;
}

void SceneBuilder::BuildPendingEvents()
{
	for (const SceneObject* sceneObj : pendingEvents_) {
		std::unique_ptr<BaseEvent> eventObject = EventFactory::Create(sceneObj->className, sceneObj->name_);

		if (sceneObj->eventInfo.has_value()) {
			const EventInfo& info = sceneObj->eventInfo.value();

			if (info.type == "EnemySpawn") {
				EnemySpawnEvent* spawnEvent = dynamic_cast<EnemySpawnEvent*>(eventObject.get());
				if (spawnEvent) {
					for (const auto& enemyInfo : info.enemies) {
						Enemy* enemy = dynamic_cast<Enemy*>(
							Object3dManager::GetInstance()->FindObject(enemyInfo.name)
							);
						if (enemy) {
							enemy->SetActive(false); // 最初は非アクティブ
							spawnEvent->AddEnemy(enemy);
						}
					}
				}
			} else if (info.type == "Clear") {
				ClearEvent* clearEvent = dynamic_cast<ClearEvent*>(eventObject.get());
				if (clearEvent) {
					for (const auto& cond : info.conditions) {
						if (cond.type == "DEFEAT_ENEMIES") {
							for (const auto& targetName : cond.targets) {
								Enemy* enemy = dynamic_cast<Enemy*>(
									Object3dManager::GetInstance()->FindObject(targetName)
									);
								if (enemy) {
									clearEvent->AddTargetEnemy(enemy);
								}
							}
						}
					}
				}
			}

			EventManager::GetInstance()->AddEvent(eventObject.get());
		}

		// Transform, Collider 設定は通常の Object3d と同じ処理
		WorldTransform* transform = eventObject->GetWorldTransform();
		Vector3 tempTranslate = sceneObj->transform.translate;
		std::swap(tempTranslate.y, tempTranslate.z);
		transform->GetTranslation() = tempTranslate;

		Vector3 tempScale = sceneObj->transform.scale;
		std::swap(tempScale.y, tempScale.z);
		transform->GetScale() = tempScale;

		//transform->GetRotation() = EulerDegree(sceneObj->transform.rotate);

		if (sceneObj->collider) {
			const Collider& col = sceneObj->collider.value();
			if (col.type == ColliderType::AABB) {
				auto collider = std::make_unique<AABBCollider>(sceneObj->name_);
				AABBData scaledAABB = col.aabb;
				scaledAABB.offsetMin *= transform->GetScale() * 2.0f;
				scaledAABB.offsetMax *= transform->GetScale() * 2.0f;
				collider->GetColliderData() = scaledAABB;
				BaseCollider* colPtr = collider.get();
				CollisionManager::GetInstance()->AddCollider(std::move(collider));
				eventObject->AddCollider(colPtr);

			} else if (col.type == ColliderType::Sphere) {
				auto collider = std::make_unique<SphereCollider>(sceneObj->name_);
				SphereData scaledSphere = col.sphere;
				scaledSphere.offset *= eventObject->GetWorldTransform()->GetScale();
				BaseCollider* colPtr = collider.get();
				CollisionManager::GetInstance()->AddCollider(std::move(collider));
				eventObject->AddCollider(colPtr);
			}
		}

		eventObject->Initialize();



		Object3dManager::GetInstance()->AddObject(std::move(eventObject));

	}

	pendingEvents_.clear();
}

