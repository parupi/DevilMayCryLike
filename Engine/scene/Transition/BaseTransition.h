#pragma once
#include <string>
class BaseTransition
{
public:
	BaseTransition() = default;
	virtual ~BaseTransition() = default;
	virtual void Start(bool isFadeOut) = 0;
	virtual void Update() = 0;
	virtual void Draw() = 0;
	virtual bool IsFinished() const = 0;

	std::string name;
};

