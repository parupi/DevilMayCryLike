#pragma once
#include <Vector3.h>
#include <Vector4.h>
#include <string>

class BaseLight {
public:
    BaseLight();
    virtual ~BaseLight() = default;
    virtual void Update() = 0;


    std::string name;
};