#define NOMINMAX
#include "SceneBuilder.h"
#include "World3D/Object/Object3d.h"
#include "World3D/WorldTransform.h"
#include "World3D/Collider/AABBCollider.h"
#include "World3D/Collider/SphereCollider.h"
#include "World3D/Collider/CollisionManager.h"
#include "Scene/Object3dFactory.h"
#include "GameObject/Character/Enemy/Enemy.h"
#include "GameObject/Event/EventManager.h"
#include "GameObject/Event/EventFactory.h"
#include "GameObject/Event/EnemySpawnEvent.h"
#include "GameObject/Event/ClearEvent.h"

// ---------------------------------------------------------------------------
// helpers
// ---------------------------------------------------------------------------

bool SceneBuilder::IsEvent(const SceneObject& obj) {
    return obj.className.rfind("Event_", 0) == 0;
}

void SceneBuilder::ApplyTransform(WorldTransform* transform, const EulerTransform& src) {
    // Blenderエクスポート(Y-up)からエンジン座標系(Z-up)への変換
    Vector3 translate = src.translate;
    std::swap(translate.y, translate.z);
    transform->GetTranslation() = translate;

    Vector3 scale = src.scale;
    std::swap(scale.y, scale.z);
    transform->GetScale() = scale;
}

void SceneBuilder::ApplyCollider(Object3d* object, const std::string& name, const Collider& col) {
    const Vector3 scale = object->GetWorldTransform()->GetScale();

    if (col.type == ColliderType::AABB) {
        auto collider = std::make_unique<AABBCollider>(name);
        AABBData data = col.aabb;
        data.offsetMin *= scale * 2.0f;
        data.offsetMax *= scale * 2.0f;
        collider->GetColliderData() = data;
        BaseCollider* ptr = collider.get();
        CollisionManager::GetInstance()->AddCollider(std::move(collider));
        object->AddCollider(ptr);

    } else if (col.type == ColliderType::Sphere) {
        auto collider = std::make_unique<SphereCollider>(name);
        SphereData data = col.sphere;
        data.offset *= scale;
        collider->GetColliderData() = data;
        BaseCollider* ptr = collider.get();
        CollisionManager::GetInstance()->AddCollider(std::move(collider));
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

    ApplyTransform(object->GetWorldTransform(), sceneObj.transform);

    if (sceneObj.collider) {
        ApplyCollider(object.get(), sceneObj.name, sceneObj.collider.value());
    }

    object->Initialize();

    Object3dManager::GetInstance()->AddObject(std::move(object));
}

void SceneBuilder::BuildEvent(const SceneObject& sceneObj) {
    auto eventObject = EventFactory::Create(sceneObj.className, sceneObj.name);
    if (!eventObject) return;

    if (sceneObj.eventInfo.has_value()) {
        const EventInfo& info = sceneObj.eventInfo.value();

        if (info.type == "EnemySpawn") {
            auto* spawnEvent = dynamic_cast<EnemySpawnEvent*>(eventObject.get());
            if (spawnEvent) {
                for (const auto& enemyInfo : info.enemies) {
                    auto* enemy = dynamic_cast<Enemy*>(
                        Object3dManager::GetInstance()->FindObject(enemyInfo.name));
                    if (enemy) {
                        enemy->SetActive(false);
                        spawnEvent->AddEnemy(enemy);
                    }
                }
            }
        } else if (info.type == "Clear") {
            auto* clearEvent = dynamic_cast<ClearEvent*>(eventObject.get());
            if (clearEvent) {
                for (const auto& cond : info.conditions) {
                    if (cond.type == "DEFEAT_ENEMIES") {
                        for (const auto& targetName : cond.targets) {
                            auto* enemy = dynamic_cast<Enemy*>(
                                Object3dManager::GetInstance()->FindObject(targetName));
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

    ApplyTransform(eventObject->GetWorldTransform(), sceneObj.transform);

    if (sceneObj.collider) {
        ApplyCollider(eventObject.get(), sceneObj.name, sceneObj.collider.value());
    }

    eventObject->Initialize();
    Object3dManager::GetInstance()->AddObject(std::move(eventObject));
}
