#pragma once
#include <cstdint>

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