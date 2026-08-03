#pragma once
#include <memory>
#include "Graphics/Rendering/Sprite/Sprite.h"

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
	// 描画は SpriteManager が UI レイヤーとして自動で行う

private:
	Sprite* upperDivider_ = nullptr;
	Sprite* underDivider_ = nullptr;

	float alpha_;
	DividerState state_ = DividerState::Normal;
};

