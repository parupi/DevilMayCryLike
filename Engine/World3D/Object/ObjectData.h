#pragma once
#include <Graphics/Rendering/PSO/PSOManager.h>

// どのパスで描画するか
enum class DrawPath{
	Forward,
	Deferred,
};