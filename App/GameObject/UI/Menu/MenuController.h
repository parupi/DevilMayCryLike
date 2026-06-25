#pragma once
#include <memory>
#include "Graphics/Rendering/Sprite/Sprite.h"

enum class MenuStates {
	SetUp,
	Enter,
	SelectFirst,
	SelectSecond,
	Confirm,
	Decision,
	Exit,
};

class MenuController
{
public:
	MenuController() = default;
	~MenuController() = default;

	void Initialize();
	void Enter();
	void Exit();
	void Update();
private:
	Sprite* rightArrow_ = nullptr;
	Sprite* leftArrow_ = nullptr;

	Sprite* musk_ = nullptr;

	MenuStates states_ = MenuStates::SetUp;

	float alpha_ = 0.0f;
	float muskAlpha_ = 0.0f;

	float currentY_ = 300.0f;
	float arrowTargetY_ = 300.0f;

public:
	MenuStates GetStates() const { return states_; }
};

