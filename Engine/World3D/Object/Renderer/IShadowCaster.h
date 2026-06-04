#pragma once

class IShadowCaster {
public:
	virtual void DrawShadow() = 0;
	virtual ~IShadowCaster() = default;
};
