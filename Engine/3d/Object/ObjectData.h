#pragma once
#include <Graphics/Rendering/PSO/PSOManager.h>

// どのパスで描画するか
enum class DrawPath{
	Forward,
	Deferred,
};

struct DrawOption {
	BlendMode blendMode = BlendMode::kNormal;
	DrawPath drawPath = DrawPath::Deferred;
	bool isLighting = true;
};