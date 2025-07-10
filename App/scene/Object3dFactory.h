#pragma once
#pragma once
#include <string>
#include "3d/Object/Object3d.h"
class Object3dFactory
{
public:
	static std::unique_ptr<Object3d> Create(const std::string& className, const std::string& objectName);
};

