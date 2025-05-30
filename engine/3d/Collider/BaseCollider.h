#pragma once
#include <string>
#include <memory>
#include <WorldTransform.h>

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

	std::string name;
	Object3d* owner_;
	std::unique_ptr<WorldTransform> transform_;
};

