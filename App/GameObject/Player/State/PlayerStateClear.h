#include "PlayerStateBase.h"
class PlayerStateClear : public PlayerStateBase
{
public:
	PlayerStateClear() = default;
	~PlayerStateClear() override = default;
	void Enter(Player& player) override;
	void Update(Player& player) override;
	void Exit(Player& player) override;
};
