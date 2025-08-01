#pragma once
#include <math/Vector3.h>
#include <math/Vector4.h>
#include <string>

class BaseLight {
public:
    BaseLight();
    virtual ~BaseLight() = default;
    virtual void Update() = 0;

    std::string name_;
};