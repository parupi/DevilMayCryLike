#pragma once
#include <Vector4.h>
#include <Vector3.h>

struct  alignas(16) DirectionalLightData {
	Vector4 color;		//!< ライトの色
	Vector3 direction;	//!< ライトの向き
	float intensity;	//!< 輝度
	int enabled;
};

// pointLight
struct alignas(16) PointLightData {
	Vector4 color; //!<ライトの色
	Vector3 position; //!<ライトの位置
	float intensity; //!< 輝度
	float radius; //!< ライトの届く最大距離
	float decay; //!< 減衰率
	int enabled;
	float padding[2];
};

// spotLight
struct  alignas(16) SpotLightData {
	Vector4 color; //!< ライトの色
	Vector3 position; //!< ライトの位置
	float intensity; //!< 輝度
	Vector3 direction; //!< ライトの向き
	float distance; //!< ライトの届く最大距離
	float decay; //!< 減衰率
	float cosAngle; //!< スポットライトの余弦
	int enabled;
	float padding[2];
};