#pragma once

namespace EnemyStateName {
	constexpr const char* Air = "Air";
	constexpr const char* Idle = "Idle";
	constexpr const char* Move = "Move";
	constexpr const char* KnockBack = "KnockBack";
}

namespace GruntMeleeStateName {
	constexpr const char* Patrol      = "Patrol";
	constexpr const char* CombatIdle  = "CombatIdle";
	constexpr const char* Approach    = "Approach";
	constexpr const char* SideMove    = "SideMove";
	constexpr const char* Retreat     = "Retreat";
	constexpr const char* AttackNormal = "AttackNormal";
	constexpr const char* RushAttack  = "RushAttack";
}

namespace BossStateName {
	constexpr const char* CombatIdle  = "BossCombatIdle";
	constexpr const char* Approach    = "BossApproach";
	constexpr const char* Slash       = "BossSlash";       // 速い縦斬り
	constexpr const char* HeavySword  = "BossHeavySword";  // 遅い強力な叩きつけ
	constexpr const char* Rush        = "BossRush";        // 突進攻撃
	constexpr const char* KnockBack   = "BossKnockBack";   // 吹き飛び（着地後すぐ Rush へ移行）
}
