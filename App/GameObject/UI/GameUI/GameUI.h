#pragma once
#include <memory>
#include <Graphics/Rendering/Sprite/Sprite.h>
class GameUI
{
public:
	GameUI() = default;
	~GameUI() = default;

	void Initialize();

	void Update();

	// 描画は SpriteManager が UI レイヤーとして自動で行う

private:

	Sprite* attackUI_;
	Sprite* jumpUI_;
	Sprite* lockOnUI_;
};

