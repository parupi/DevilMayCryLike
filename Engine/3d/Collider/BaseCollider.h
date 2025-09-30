#pragma once
#include <string>
#include <memory>
#include <3d/WorldTransform.h>

#include <GameData/CollisionCategory.h>
enum class CollisionShapeType {
	AABB,
	Sphere,
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

	CollisionCategory category_;
	std::string name_;
	Object3d* owner_;
	std::unique_ptr<WorldTransform> transform_;
	bool isAlive;
};

