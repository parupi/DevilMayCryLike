#pragma once
#include <memory>
#include "2d/Sprite.h"

enum class DividerState {
	Enter,
	Normal,
	Exit,
};

class MenuDivider
{
public:
	MenuDivider() = default;
	~MenuDivider() = default;

	void Initialize();
	void Enter();
	void Exit();
	void Update();
	void Draw();

private:
	std::unique_ptr<Sprite> upperDivider_ = nullptr;
	std::unique_ptr<Sprite> underDivider_ = nullptr;

	float alpha_;
	DividerState state_ = DividerState::Normal;
};

