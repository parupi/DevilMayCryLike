#pragma once
#include <World3D/WorldTransform.h>
#include <memory>
#include <string>
class BaseModel;

// 描画パス固有の機能は IDeferredDrawable / IShadowCaster を参照
class BaseRenderer
{
public:
	virtual ~BaseRenderer() = default;

	virtual void Update(WorldTransform* parentTransform) = 0;
	virtual void Draw() = 0;
#ifdef _DEBUG
	virtual void DebugGui(size_t index) = 0;
#endif // DEBUG

	virtual WorldTransform* GetWorldTransform() const = 0;
	virtual BaseModel* GetModel() const = 0;

	std::string name_;
	bool isAlive = true;

protected:
	std::unique_ptr<WorldTransform> localTransform_;
};

