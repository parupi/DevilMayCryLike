#pragma once
#include <string>
#include <memory>
#include <World3D/WorldTransform.h>
#include <GameData/CollisionCategory.h>
enum class CollisionShapeType {
	AABB,
	Sphere,
	OBB,
};

class Object3d;

class BaseCollider
{
public:
	virtual void Initialize() = 0;
	virtual void Update() = 0;
	virtual void DrawDebug() = 0;
	virtual CollisionShapeType GetShapeType() const = 0;
	virtual void SetOwner(Object3d* owner) { owner_ = owner; }

	// 判定の有効・無効。実体は形状ごとのデータ(AABBData/SphereData/OBBData)の isActive。
	// 形状を知らないまま切りたい場面（未出現の敵の判定を止める等）のために口を出しておく
	virtual void SetColliderActive(bool active) = 0;
	virtual bool IsColliderActive() const = 0;

	// 既定は None。所有者クラスが Initialize() で設定し忘れても、
	// 未初期化の値が Ground などに化けて「見えない壁」にならないようにする
	CollisionCategory category_ = CollisionCategory::None;
	std::string name_;
	Object3d* owner_ = nullptr;
	std::unique_ptr<WorldTransform> transform_;
	bool isAlive = true;
};

