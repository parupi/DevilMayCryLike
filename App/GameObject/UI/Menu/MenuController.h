#pragma once
#include <memory>
#include "2d/Sprite.h"

enum class MenuStates {
	SetUp,
	Enter,
	SelectFirst,
	SelectSecond,
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
	void Draw();
private:
	std::unique_ptr<Sprite> rightArrow_ = nullptr;
	std::unique_ptr<Sprite> leftArrow_ = nullptr;

	std::unique_ptr<Sprite> musk_ = nullptr;

	MenuStates states_ = MenuStates::SetUp;

	float alpha_ = 0.0f;
	float muskAlpha_ = 0.0f;

public:
	MenuStates GetStates() const { return states_; }
};

