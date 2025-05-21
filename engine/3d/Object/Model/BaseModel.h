#pragma once

class Object3d;
class BaseModel
{
public:
	//virtual ~BaseModel() = default;
	virtual void Update() = 0;
	virtual void Draw() = 0;
#ifdef _DEBUG
	virtual void DebugGui(Object3d* object) = 0;
#endif // _DEBUG
};

