#pragma once

class IDeferredDrawable {
public:
	virtual void DrawGBuffer() = 0;
	virtual ~IDeferredDrawable() = default;
};
