#pragma once
#include "Graphics/Rendering/Sprite/Sprite.h"
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

	void SetSelectedIndex(int index) { selectedIndex_ = index; }

	void SetConfirming(bool confirming) {
		if (confirming && !isConfirming_) blinkTimer_ = 0.0f;
		isConfirming_ = confirming;
	}

private:
	Sprite* toTitle_ = nullptr;
	Sprite* toContinue_ = nullptr;

	ChoicesState state_ = ChoicesState::Normal;

	float alpha_ = 0.0f;
	int selectedIndex_ = 0;
	bool isConfirming_ = false;
	float blinkTimer_ = 0.0f;
};

