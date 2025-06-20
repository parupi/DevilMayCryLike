#pragma once
#include <Model/ModelStructs.h>
#include <memory>
#include <Model/Model.h>
#include "PrimitiveRenderer.h"

class PrimitiveFactory {
public:
    static std::unique_ptr<Model> Create(PrimitiveType type, std::string textureName);
};

