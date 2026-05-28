#pragma once
#include <vector>
#include "math/Vector3.h"

class Object3d;
class ModelRenderer;
class Material;
class BaseModel
{
public:
	//virtual ~BaseModel() = default;
	virtual void Update(const Vector3& objectScale) = 0;
	virtual void Draw() = 0;
	virtual void DrawGBuffer() = 0;
	virtual void DrawShadow() = 0;
	virtual std::vector<Material*> GetMaterials() = 0;
#ifdef _DEBUG
	virtual void DebugGui(ModelRenderer* render) = 0;
#endif // _DEBUG
};

