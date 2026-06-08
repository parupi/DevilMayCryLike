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

	void Draw();

private:

	Sprite* attackUI_;
	Sprite* jumpUI_;
	Sprite* lockOnUI_;
};

