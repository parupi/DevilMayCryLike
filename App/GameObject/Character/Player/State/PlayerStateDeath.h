#pragma once 
#include "PlayerStateBase.h"
#include "math/Vector3.h"
#include "math/Quaternion.h"
class PlayerStateDeath : public PlayerStateBase
{
public:
	PlayerStateDeath();
	~PlayerStateDeath() override = default;
	void Enter(Player& player) override;
	void Update(Player& player, float deltaTime) override;
	void Exit(Player& player) override;
	void ExecuteCommand(Player& player, const PlayerCommand& command) override;
	const char* GetDebugName() const override { return "Death"; };
private:
	float currentTime_ = 0.0f;
	float maxTime_ = 1.0f;

	Vector3 startScale_{};
	Quaternion startRotate_{};
	Vector3 endScale_{};
	Quaternion endRotate_{};
};

