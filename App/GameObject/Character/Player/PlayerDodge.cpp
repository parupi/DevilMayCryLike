#include "PlayerDodge.h"
#include <Debugger/GlobalVariables.h>

void PlayerDodgeParams::RegisterAndLoad() {
	GlobalVariables& global = GlobalVariables::GetInstance();
	// 保存済みの設定があれば読み込む（無ければ AddItem の既定値がそのまま残る）
	global.LoadFile(kDirectoryName, kGroupName);

	global.AddItem(kGroupName, "DodgeDuration", dodgeDuration);
	global.AddItem(kGroupName, "DodgeSpeed", dodgeSpeed);
	global.AddItem(kGroupName, "DodgeInvincibleTime", invincibleTime);
	global.AddItem(kGroupName, "DodgeCooldown", cooldown);

	global.AddItem(kGroupName, "DashSpeed", dashSpeed);
	global.AddItem(kGroupName, "DashDuration", dashDuration);
	global.AddItem(kGroupName, "DashTurnRate", dashTurnRate);
	global.AddItem(kGroupName, "DashInputGrace", dashInputGrace);

	global.AddItem(kGroupName, "JustDodgeSlowTime", justDodgeSlowTime);
	global.AddItem(kGroupName, "JustDodgeTimeScale", justDodgeTimeScale);
	global.AddItem(kGroupName, "JustDodgeShake", justDodgeShake);
	global.AddItem(kGroupName, "JustDodgeInvincibleAdd", justDodgeInvincibleAdd);

	global.AddItem(kGroupName, "DashFovPunch", dashFovPunch);
	global.AddItem(kGroupName, "TrailLifetime", trailLifetime);

	Apply();
}

void PlayerDodgeParams::Apply() {
	GlobalVariables& global = GlobalVariables::GetInstance();

	dodgeDuration = global.GetValueRef<float>(kGroupName, "DodgeDuration");
	dodgeSpeed = global.GetValueRef<float>(kGroupName, "DodgeSpeed");
	invincibleTime = global.GetValueRef<float>(kGroupName, "DodgeInvincibleTime");
	cooldown = global.GetValueRef<float>(kGroupName, "DodgeCooldown");

	dashSpeed = global.GetValueRef<float>(kGroupName, "DashSpeed");
	dashDuration = global.GetValueRef<float>(kGroupName, "DashDuration");
	dashTurnRate = global.GetValueRef<float>(kGroupName, "DashTurnRate");
	dashInputGrace = global.GetValueRef<float>(kGroupName, "DashInputGrace");

	justDodgeSlowTime = global.GetValueRef<float>(kGroupName, "JustDodgeSlowTime");
	justDodgeTimeScale = global.GetValueRef<float>(kGroupName, "JustDodgeTimeScale");
	justDodgeShake = global.GetValueRef<float>(kGroupName, "JustDodgeShake");
	justDodgeInvincibleAdd = global.GetValueRef<float>(kGroupName, "JustDodgeInvincibleAdd");

	dashFovPunch = global.GetValueRef<float>(kGroupName, "DashFovPunch");
	trailLifetime = global.GetValueRef<float>(kGroupName, "TrailLifetime");
}
