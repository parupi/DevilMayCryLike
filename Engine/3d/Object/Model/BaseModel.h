#pragma once
#include <vector>

class Object3d;
class ModelRenderer;
class Material;
class BaseModel
{
public:
	//virtual ~BaseModel() = default;
	virtual void Update() = 0;
	virtual void Draw() = 0;
	virtual void DrawGBuffer() = 0;
	virtual void DrawShadow() = 0;
	virtual std::vector<Material*> GetMaterials() = 0;
#ifdef _DEBUG
	virtual void DebugGui(ModelRenderer* render) = 0;
#endif // _DEBUG
};

