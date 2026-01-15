#pragma once
#include <memory>
#include <2d/Sprite.h>
class GameUI
{
public:
	GameUI() = default;
	~GameUI() = default;

	void Initialize();

	void Update();

	void Draw();

private:

	std::unique_ptr<Sprite> attackUI_;
	std::unique_ptr<Sprite> jumpUI_;
	std::unique_ptr<Sprite> lockOnUI_;
};

