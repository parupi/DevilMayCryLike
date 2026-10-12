#pragma once
#include <string>
#include <memory>
// 前方宣言
class Player;
struct PlayerCommand;
// プレイヤーの状態を管理する基底クラス
class PlayerStateBase {
public:
	virtual ~PlayerStateBase() = default;
	// ステートに入るときの処理
	virtual void Enter(Player& player) = 0;
	// ステートの更新処理
	virtual void Update(Player& player, float deltaTime) = 0;
	// ステートから出るときの処理
	virtual void Exit(Player& player) = 0;
	// プレイヤーからのコマンドを処理する。
	virtual void ExecuteCommand(Player& player, const PlayerCommand& command) = 0;
	// デバッグ用のステート名を返す
	virtual const char* GetDebugName() const = 0;
	/// <summary>
	/// このステートでジャンプの入力を受け付けるか。
	/// ExecuteCommand のジャンプの分岐と、アクションガイドの表示がこれを共有する
	/// </summary>
	virtual bool CanJump() const { return false; }
};

