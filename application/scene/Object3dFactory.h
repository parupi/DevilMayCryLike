#pragma once
#pragma once
#include <string>
#include "Object/Object3d.h"
class Object3dFactory
{
public:
	static Object3d* Create(const std::string& className, const std::string& objectName);
};

