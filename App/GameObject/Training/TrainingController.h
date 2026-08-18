#pragma once
#include <Math/Quaternion.h>
#include <Math/Vector3.h>
#include <cstdint>
#include <string>

class Enemy;
class LockOnSystem;
class Player;

/// <summary>敵にどこまで行動を許すか</summary>
enum class TrainingBehavior {
	Hold,     ///< 棒立ち。追いかけも攻撃もしない（被弾リアクションだけ見たいとき）
	MoveOnly, ///< 移動はするが攻撃しない
	Full,     ///< 通常どおり

	Count,
};

/// <summary>
/// トレーニングルームの操作をまとめたクラス。
///
/// 敵の生成・出し直し・種類の切り替えと、無敵／行動制限のスイッチを持つ。
/// GameScene がトレーニングのときだけ生成し、毎フレーム Update() を呼ぶ。
///
/// **敵はポインタではなく名前で持つ。** 死亡後は RemoveDeadObject でオブジェクトごと
/// 消えるのでポインタはダングリングする（ForceBattleEvent が同じ理由で名前保持になっている）。
/// さらに出し直しのたびに一意な名前を振る。名前はレンダラー・コライダー・パーティクルの
/// キーにもなっているので、前の相手が消えきる前に同じ名前で作ると衝突する
/// </summary>
class TrainingController
{
public:
	/// <summary>操作キーの一覧（HUD の案内と実装で同じものを使う）</summary>
	struct KeyHint {
		const char* key;
		const char* action;
	};
	static const KeyHint kKeyHints[];
	static const int32_t kKeyHintCount;

	~TrainingController();

	/// <summary>
	/// いま動いているトレーニング。本編では nullptr。
	///
	/// エディタのウィンドウ（App/Editor/Windows/TrainingWindow.cpp）が引くための入口。
	/// エディタは GameScene を知らない作りなので、Player や GameCamera を
	/// マネージャから引き直しているのと同じ理屈でここから取れるようにしている。
	/// 実体の所有は GameScene のままで、シーンを抜ければ自動で nullptr に戻る
	/// </summary>
	static TrainingController* GetCurrent() { return current_; }

	/// <summary>プレイヤーの初期位置をここで控えるので、ステージ生成後に呼ぶこと</summary>
	void Initialize(Player* player, LockOnSystem* lockOn);

	/// <summary>
	/// 毎フレーム呼ぶ。
	/// </summary>
	/// <param name="acceptInput">
	/// キー操作を受け付けるか。ポーズ中・ゲームオーバー中は false にする
	/// （メニュー操作中にトレーニングのキーが同時に効いてしまうのを防ぐ）
	/// </param>
	void Update(bool acceptInput);

	/// <summary>
	/// 設定メニューを開く要求が出ていたら true を返し、要求を下ろす。
	///
	/// ここで状態を切り替えないのは、シーンのステートを持っているのが GameScene だから。
	/// GameSceneStatePlay が毎フレーム拾って "TrainingMenu" へ遷移する
	/// </summary>
	bool ConsumeMenuRequest();

	/// <summary>設定メニューを開かせる。キーのほか、エディタのボタンからも呼ぶ</summary>
	void RequestMenu() { menuRequested_ = true; }

	// ======================
	// 操作
	// ======================

	/// <summary>今の相手を消して、同じ種類を出し直す</summary>
	void RequestRespawn();
	/// <summary>戦う敵を切り替える（EnemyCatalog の並び順のインデックス）</summary>
	void SelectEnemy(int32_t index);
	/// <summary>次の敵へ。一番後ろまで行ったら先頭へ戻る</summary>
	void SelectNextEnemy();
	/// <summary>プレイヤーを初期位置・全快に戻し、敵も出し直す</summary>
	void ResetAll();

	// ======================
	// 設定
	// ======================

	void SetPlayerInvincible(bool invincible);
	bool IsPlayerInvincible() const { return playerInvincible_; }

	/// <summary>敵を倒せなくする。HP も毎フレーム満タンへ戻すのでゲージは減らない</summary>
	void SetEnemyInvincible(bool invincible);
	bool IsEnemyInvincible() const { return enemyInvincible_; }

	void SetBehavior(TrainingBehavior behavior);
	TrainingBehavior GetBehavior() const { return behavior_; }
	void CycleBehavior();

	/// <summary>倒したあと自動で出し直すか</summary>
	void SetAutoRespawn(bool enable) { autoRespawn_ = enable; }
	bool IsAutoRespawn() const { return autoRespawn_; }

	void SetHudVisible(bool visible) { hudVisible_ = visible; }
	bool IsHudVisible() const { return hudVisible_; }

	// ======================
	// 参照
	// ======================

	/// <summary>今の相手。出し直しの合間や撃破直後は nullptr になる</summary>
	Enemy* GetEnemy() const;
	int32_t GetEnemyIndex() const { return enemyIndex_; }
	/// <summary>今選んでいる敵の表示名（EnemyCatalog のもの）</summary>
	const std::string& GetEnemyDisplayName() const;

	static const char* BehaviorLabel(TrainingBehavior behavior);
	const char* GetBehaviorLabel() const { return BehaviorLabel(behavior_); }

private:
	// 相手を1体作って出現させる
	void SpawnEnemy();
	// 今いる相手を死亡演出に乗せて片付ける（武器・判定の後始末は各クラスが持っている）
	void RemoveCurrentEnemy();
	// スイッチの内容を相手へ反映する。出し直すたびに新しい個体になるので毎フレーム掛ける
	void ApplySettings(Enemy* enemy);
	void HandleInput();

	Player* player_ = nullptr;
	LockOnSystem* lockOn_ = nullptr;

	// リセットで戻す位置。ステージデータに書かれた Player の初期値
	Vector3 playerStartPos_{};
	Quaternion playerStartRot_{};

	int32_t enemyIndex_ = 0;
	std::string enemyName_;     ///< 生成した相手の名前（空なら未生成）
	int32_t spawnSerial_ = 0;   ///< 名前を一意にするための連番

	/// <summary>
	/// 最初の生成を遅らせるフレーム数。
	/// Enemy::Spawn() の接地処理は Ground のコライダーが更新済みである前提だが、
	/// シーンの初期化時点ではまだ一度も CollisionManager::Update() が走っていない
	/// </summary>
	int32_t spawnDelayFrames_ = 0;
	float autoRespawnTimer_ = 0.0f;

	/// <summary>設定メニューを開く要求。ConsumeMenuRequest() で下ろす</summary>
	bool menuRequested_ = false;

	bool playerInvincible_ = false;
	bool enemyInvincible_ = false;
	bool autoRespawn_ = true;
	bool hudVisible_ = true;
	TrainingBehavior behavior_ = TrainingBehavior::Full;

	static TrainingController* current_;

	/// <summary>倒れてから次の相手が出るまでの間（秒）</summary>
	static constexpr float kAutoRespawnDelay = 0.8f;
	/// <summary>相手の名前の接頭辞。連番を付けて一意にする</summary>
	static constexpr const char* kEnemyNamePrefix = "TrainingEnemy";
};
