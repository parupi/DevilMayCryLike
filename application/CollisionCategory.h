#pragma once

/// <summary>
/// 当たり判定のカテゴリ
/// </summary>
enum class CollisionCategory {
	None,
	Player,
	PlayerWeapon,
	Enemy,
	Ground,
};