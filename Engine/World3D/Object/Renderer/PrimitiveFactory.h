#pragma once
#include <World3D/Object/Model/ModelStructs.h>
#include <memory>
#include "PrimitiveRenderer.h"
#include <World3D/Object/Model/Model.h>

class PrimitiveFactory {
public:
	static std::unique_ptr<Model> Create(PrimitiveType type, std::string textureName);
};