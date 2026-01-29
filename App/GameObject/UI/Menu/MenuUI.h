#pragma once
#include "MenuDivider.h"
#include "MenuChoices.h"
#include <memory>
#include "MenuController.h"

class GameScene;

class MenuUI
{
public:
	MenuUI() = default;
	~MenuUI() = default;

	void Initialize(GameScene* scene);
	void Enter();
	void Exit();
	void Update();
	void Draw();

	bool IsExit() const { return isExit_; }
private:
	GameScene* scene_ = nullptr;

	std::unique_ptr<MenuDivider> divider_ = nullptr;
	std::unique_ptr<MenuChoices> choices_ = nullptr;
	std::unique_ptr<MenuController> controller_ = nullptr;

	bool isExit_ = false;
};

