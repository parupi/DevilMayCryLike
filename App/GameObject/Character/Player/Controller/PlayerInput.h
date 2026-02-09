#pragma once
#include <vector>
#include "math/Vector2.h"

class Input;

enum class PlayerAction {
	Move,
	Jump,
	Attack,
	LockOn,
};

struct PlayerCommand {
	PlayerAction action;
	Vector2 stickDir = {0.0f, 0.0f};
};

struct PlayerInputContext {
	Vector2 move;        // -1〜1
	bool isMove;
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
	// コマンド一覧を取得
	const std::vector<PlayerCommand>& GetCommands() const { return commands_; }
	// 現在の入力状態を取得
	const PlayerInputContext& GetContext() const { return context_; }
private:
	Input* input_ = nullptr;
	std::vector<PlayerCommand> commands_;
	PlayerInputContext context_;
};

