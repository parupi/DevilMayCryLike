#pragma once
#include <Graphics/Rendering/PSO/PSOManager.h>
#include <Graphics/Rendering/RenderPath/ForwardRenderStruct.h>

struct SpriteOption {
	bool isDraw = true;
	BlendMode blendMode = BlendMode::kNormal;
	BlendCategory blendCategory = BlendCategory::Transparent;
};