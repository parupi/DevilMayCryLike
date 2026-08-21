#pragma once
#include "Math/Vector3.h"

class IShadowCaster {
public:
	virtual void DrawShadow() = 0;
	virtual ~IShadowCaster() = default;

	/// <summary>
	/// カスケードの外にいるかを判定するためのワールド境界球。
	/// false を返した場合はカリングせず必ず描く（形が変わるスキンモデルなど）。
	/// </summary>
	virtual bool GetShadowBoundingSphere([[maybe_unused]] Vector3& outCenter, [[maybe_unused]] float& outRadius) const { return false; }
};
