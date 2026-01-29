#pragma once
#include <math/Vector2.h>
#include <cstdint>

enum class InputType {
	Y,
	A,
	B
};

enum class StickDir {
	Neutral,
	Up,
	Down,
	Left,
	Right
};

struct AttackBranch {
	uint32_t nextAttack;
	InputType input; // Y / A / 
	StickDir  dir; // Neutral / Up / Down / Left / Right
	bool requireLockOn;
};

struct BranchUIElement
{
	AttackBranch branch;
	Vector2 basePos;
};

// 入力状態
struct AttackInputState {
	bool y = false;
	bool rb = false;
	StickDir dir = StickDir::Neutral;
	bool isLockOn = false;
};

enum class AttackRequest {
	None,
	Air,
	Jump,
	ChangeAttack,
};

struct AttackRequestData {
	AttackRequest type = AttackRequest::None;
	std::string nextAttack;
};