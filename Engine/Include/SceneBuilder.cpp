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

void SceneBuilder::BuildScene(const std::vector<SceneObject>& sceneObjects) {
	for (const auto& obj : sceneObjects) {
		Object3dManager::GetInstance()->AddObject(BuildObjectRecursive(obj, nullptr));
	}
}

std::unique_ptr<Object3d> SceneBuilder::BuildObjectRecursive(const SceneObject& sceneObj, Object3d* parent) {
	std::unique_ptr<Object3d> object = Object3dFactory::Create(sceneObj.className, sceneObj.name_);

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

	transform->GetRotation() = EulerDegree(sceneObj.transform.rotate);

	if (parent) {
		transform->SetParent(parent->GetWorldTransform());
	}

	// --- Rendererの生成と登録 ---
	if (sceneObj.fileName && sceneObj.fileName != "") {
		const std::string& fileName = sceneObj.fileName.value();
		ModelManager::GetInstance()->LoadModel(fileName);

		std::unique_ptr<ModelRenderer> renderer = std::make_unique<ModelRenderer>(sceneObj.name_, fileName);
		renderer->SetModel(fileName);

		BaseRenderer* rendererPtr = renderer.get(); // 登録前にポインタを保存
		RendererManager::GetInstance()->AddRenderer(std::move(renderer)); // 管理側に渡す

		object->AddRenderer(rendererPtr); // オブジェクトに追加

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