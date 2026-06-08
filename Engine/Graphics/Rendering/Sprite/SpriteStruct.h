#pragma once
#include <Graphics/Rendering/PSO/PSOManager.h>

struct SpriteRenderState {
	bool isVisible = true;
	BlendMode blendMode = BlendMode::kNormal;
};

enum class SpriteLayer {
	Background,
	Game,
	UI,
	Debug,
	Count,
};