#pragma once
#include <3d/Object/Model/ModelStructs.h>
#include <memory>
#include "PrimitiveRenderer.h"
#include <3d/Object/Model/Model.h>

class PrimitiveFactory {
public:
    static std::unique_ptr<Model> Create(PrimitiveType type, std::string textureName);
};

