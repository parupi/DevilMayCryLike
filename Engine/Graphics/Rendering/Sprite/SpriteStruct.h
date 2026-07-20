#pragma once
#include <Graphics/Rendering/PSO/PSOManager.h>
#include <Math/Vector4.h>
#include <Math/Matrix4x4.h>

struct SpriteMaterial {
	Vector4 color; // offset  0 (16 bytes)
	Matrix4x4 uvTransform; // offset 16 (64 bytes)
	float dissolveThreshold; // offset 80 : -1.0 = disabled, 0.0-1.0 = dissolve amount
	float dissolveEdgeWidth; // offset 84 : edge glow width in noise space
	float radialFill; // offset 88 : -1.0 = disabled, 0.0-1.0 = 表示割合（上から時計回りに欠ける円形ゲージ）
	float padding; // offset 92 : HLSL float4 alignment
	Vector4 dissolveEdgeColor; // offset 96 : rgb = emissive color, a = intensity multiplier
};

struct SpriteRenderState {
	bool isVisible = true;
	BlendMode blendMode = BlendMode::kNormal;
};

enum class SpriteLayer {
	Background,
	Game,
	UI,
	Persistent, // シーン切り替えで削除されない常駐スプライト用
	Debug,
	Count,
};