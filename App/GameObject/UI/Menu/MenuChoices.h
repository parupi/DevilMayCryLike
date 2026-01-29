#pragma once
#include "2d/Sprite.h"
#include <memory>

enum class ChoicesState {
	Enter,
	Normal,
	Exit,
};

class MenuChoices
{
public:
	MenuChoices() = default;
	~MenuChoices() = default;

	void Initialize();

	void Enter();

	void Exit();

	void Update();

	void Draw();

private:
	std::unique_ptr<Sprite> toTitle_ = nullptr;
	std::unique_ptr<Sprite> toContinue_ = nullptr;

	ChoicesState state_ = ChoicesState::Normal;

	float alpha_ = 0.0f;
};

