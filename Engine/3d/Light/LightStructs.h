#pragma once
#include <math/Vector4.h>
#include <math/Vector3.h>

struct LightData
{
	uint32_t type;              // 0:Directional 1:Point 2:Spot
	uint32_t enabled;
	float intensity;
	float decay;           // point/spot only

	Vector4 color;

	Vector3 position;      // point/spot
	float radius;          // point only

	Vector3 direction;     // directional/spot
	float cosAngle;        // spot only

	float distance;        // spot only
	float padding[3];
};
