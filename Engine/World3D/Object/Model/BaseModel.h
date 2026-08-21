#pragma once
#include <vector>
#include "Math/Vector3.h"

class Object3d;
class ModelRenderer;
class Material;
class BaseModel
{
public:
	virtual ~BaseModel() = default;
	virtual void Update(const Vector3& objectScale) = 0;
	virtual void Draw() = 0;
	virtual void DrawGBuffer() = 0;
	virtual void DrawShadow() = 0;
	virtual std::vector<Material*> GetMaterials() = 0;

	/// <summary>
	/// モデルローカル空間のAABB。影のカリングなど、描くかどうかの判定に使う。
	/// 求まらないモデル（スキンのようにポーズで形が変わるもの）は false を返す＝常に描く扱い。
	/// </summary>
	virtual bool GetLocalBounds([[maybe_unused]] Vector3& outMin, [[maybe_unused]] Vector3& outMax) const { return false; }
#ifdef _DEBUG
	virtual void DebugGui(ModelRenderer* render) = 0;
#endif // _DEBUG
};

