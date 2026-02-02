#pragma once
#include <vector>
#include "math/Vector3.h"

class Input;

enum class PlayerAction {
	Move,
	Jump,
	Attack,
	//Dodge,
	LockOn,
};

struct PlayerCommand {
	PlayerAction action;
	Vector3 moveDir;   // Move用
};

class PlayerInput
{
public:
	PlayerInput() = default;
	~PlayerInput() = default;

	// 初期化
	void Initialize(Input* input);
	// 更新
	void Update();

	const std::vector<PlayerCommand>& GetCommands() const { return commands_; }
private:
	Input* input_ = nullptr;
	std::vector<PlayerCommand> commands_;
};

