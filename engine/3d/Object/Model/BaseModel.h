#pragma once

class Object3d;
class ModelRenderer;
class BaseModel
{
public:
	//virtual ~BaseModel() = default;
	virtual void Update() = 0;
	virtual void Draw() = 0;
#ifdef _DEBUG
	virtual void DebugGui(ModelRenderer* render) = 0;
#endif // _DEBUG
};

