#include "TrainingController.h"

#include "GameData/EnemyCatalog.h"
#include "GameData/GameSession.h"
#include "GameObject/Character/Enemy/Enemy.h"
#include "GameObject/Character/Enemy/EnemySpawner.h"
#include "GameObject/Character/Enemy/EnemyStateNames.h"
#include "GameObject/Character/Player/Player.h"

#include <Input/Input.h>
#include <Utility/DeltaTime.h>
#include <World3D/Object/Object3dManager.h>
#include <World3D/WorldTransform.h>

namespace {
	const std::string kEmptyName;
}

// F5 / F9 / F10 はエディタ（一時停止・デバッグカメラ・コマ送り）、F11 は全画面が使っている。
// こちらは DirectInput で直接キーを読んでいてエディタとは別経路なので、
// 同じキーを使うと Debug ビルドで両方が同時に動いてしまう。空いているものだけを選ぶこと
const TrainingController::KeyHint TrainingController::kKeyHints[] = {
	{ "F1", "RESPAWN" },
	{ "F2", "NEXT ENEMY" },
	{ "F3", "PLAYER INVINCIBLE" },
	{ "F4", "ENEMY INVINCIBLE" },
	{ "F6", "ENEMY BEHAVIOR" },
	{ "F7", "RESET" },
	{ "F8", "HIDE HUD" },
};
const int32_t TrainingController::kKeyHintCount =
	static_cast<int32_t>(sizeof(kKeyHints) / sizeof(kKeyHints[0]));

TrainingController* TrainingController::current_ = nullptr;

TrainingController::~TrainingController()
{
	if (current_ == this) {
		current_ = nullptr;
	}
}

void TrainingController::Initialize(Player* player, LockOnSystem* lockOn)
{
	current_ = this;

	player_ = player;
	lockOn_ = lockOn;

	if (player_) {
		// リセットで戻す先。ステージデータに書かれた位置をそのまま覚えておく
		playerStartPos_ = player_->GetWorldTransform()->GetTranslation();
		playerStartRot_ = player_->GetWorldTransform()->GetRotation();
	}

	// タイトルで選ばれた敵を初期選択にする
	const std::vector<EnemyCatalogEntry>& entries = EnemyCatalog::GetEntries();
	const std::string& selected = GameSession::GetTrainingEnemyClass();
	for (int32_t i = 0; i < static_cast<int32_t>(entries.size()); ++i) {
		if (entries[i].className == selected) {
			enemyIndex_ = i;
			break;
		}
	}

	// 最初の1体はコライダーが動き出してから出す
	spawnDelayFrames_ = 2;
}

void TrainingController::Update(bool acceptInput)
{
	if (acceptInput) {
		HandleInput();
	}

	if (player_) {
		player_->SetInvincible(playerInvincible_);
	}

	if (spawnDelayFrames_ > 0 && --spawnDelayFrames_ == 0) {
		SpawnEnemy();
		return;
	}

	Enemy* enemy = GetEnemy();
	if (enemy && enemy->IsAlive()) {
		autoRespawnTimer_ = 0.0f;
		ApplySettings(enemy);
		return;
	}

	// 相手が居ない（撃破された・演出が終わって消えた）。
	// ヒットストップの影響を受けないよう、待ち時間は素のデルタタイムで測る
	if (!autoRespawn_) {
		return;
	}
	autoRespawnTimer_ += DeltaTime::GetUnscaledDeltaTime();
	if (autoRespawnTimer_ >= kAutoRespawnDelay) {
		autoRespawnTimer_ = 0.0f;
		SpawnEnemy();
	}
}

void TrainingController::HandleInput()
{
	Input& input = Input::GetInstance();

	// パッドを持ったまま片手で押せるよう、出し直しだけは BACK ボタンにも割り当てる
	const bool respawnPressed = input.TriggerKey(DIK_F1)
		|| (input.IsConnected() && input.TriggerButton(PadNumber::ButtonBack));

	if (respawnPressed) RequestRespawn();
	if (input.TriggerKey(DIK_F2)) SelectNextEnemy();
	if (input.TriggerKey(DIK_F3)) SetPlayerInvincible(!playerInvincible_);
	if (input.TriggerKey(DIK_F4)) SetEnemyInvincible(!enemyInvincible_);
	if (input.TriggerKey(DIK_F6)) CycleBehavior();
	if (input.TriggerKey(DIK_F7)) ResetAll();
	if (input.TriggerKey(DIK_F8)) hudVisible_ = !hudVisible_;
}

void TrainingController::RequestRespawn()
{
	RemoveCurrentEnemy();
	autoRespawnTimer_ = 0.0f;
	// 最初の1体と違い、このときは地面のコライダーが更新済みなので待たなくてよい
	SpawnEnemy();
}

void TrainingController::SelectEnemy(int32_t index)
{
	const int32_t count = static_cast<int32_t>(EnemyCatalog::GetEntries().size());
	if (count <= 0) return;
	if (index < 0 || index >= count) return;
	if (index == enemyIndex_ && GetEnemy()) return;

	enemyIndex_ = index;
	RequestRespawn();
}

void TrainingController::SelectNextEnemy()
{
	const int32_t count = static_cast<int32_t>(EnemyCatalog::GetEntries().size());
	if (count <= 0) return;
	SelectEnemy((enemyIndex_ + 1) % count);
}

void TrainingController::ResetAll()
{
	if (player_) {
		player_->GetWorldTransform()->GetTranslation() = playerStartPos_;
		player_->GetWorldTransform()->GetRotation() = playerStartRot_;
		player_->GetVelocity() = { 0.0f, 0.0f, 0.0f };
		player_->GetAcceleration() = { 0.0f, 0.0f, 0.0f };
		player_->SetHp(player_->GetMaxHp());
	}
	RequestRespawn();
}

void TrainingController::SetPlayerInvincible(bool invincible)
{
	playerInvincible_ = invincible;
	if (player_) {
		player_->SetInvincible(invincible);
	}
}

void TrainingController::SetEnemyInvincible(bool invincible)
{
	enemyInvincible_ = invincible;
	if (Enemy* enemy = GetEnemy()) {
		ApplySettings(enemy);
	}
}

void TrainingController::SetBehavior(TrainingBehavior behavior)
{
	behavior_ = behavior;

	Enemy* enemy = GetEnemy();
	if (!enemy) return;

	ApplySettings(enemy);

	// 棒立ちに切り替えた瞬間に攻撃モーションの途中だと、ステートが更新されないまま
	// 判定が出しっぱなしで固まる。Idle へ移して Exit() を通し、攻撃を畳ませる
	if (behavior_ == TrainingBehavior::Hold && enemy->IsAlive()) {
		enemy->ChangeState(EnemyStateName::Idle);
	}
}

void TrainingController::CycleBehavior()
{
	const int32_t next = (static_cast<int32_t>(behavior_) + 1)
		% static_cast<int32_t>(TrainingBehavior::Count);
	SetBehavior(static_cast<TrainingBehavior>(next));
}

Enemy* TrainingController::GetEnemy() const
{
	if (enemyName_.empty()) return nullptr;
	return dynamic_cast<Enemy*>(Object3dManager::GetInstance().FindObject(enemyName_));
}

const std::string& TrainingController::GetEnemyDisplayName() const
{
	const std::vector<EnemyCatalogEntry>& entries = EnemyCatalog::GetEntries();
	if (enemyIndex_ < 0 || enemyIndex_ >= static_cast<int32_t>(entries.size())) {
		return kEmptyName;
	}
	return entries[enemyIndex_].displayName;
}

const char* TrainingController::BehaviorLabel(TrainingBehavior behavior)
{
	switch (behavior) {
	case TrainingBehavior::Hold:     return "HOLD";
	case TrainingBehavior::MoveOnly: return "MOVE ONLY";
	case TrainingBehavior::Full:     return "FULL";
	default:                         return "?";
	}
}

void TrainingController::SpawnEnemy()
{
	const std::vector<EnemyCatalogEntry>& entries = EnemyCatalog::GetEntries();
	if (enemyIndex_ < 0 || enemyIndex_ >= static_cast<int32_t>(entries.size())) return;
	if (!player_) return;

	const EnemyCatalogEntry& entry = entries[enemyIndex_];

	// プレイヤーの正面へ置く。プレイヤーの前方向はローカル +Z（敵とは逆なので注意）。
	// 高さは Enemy::Spawn() の中の接地処理が合わせるので大まかでよい
	WorldTransform* playerTransform = player_->GetWorldTransform();
	const Vector3 forward = RotateVector({ 0.0f, 0.0f, 1.0f }, playerTransform->GetRotation());
	const Vector3 spawnPos = playerTransform->GetTranslation() + forward * entry.spawnDistance;

	// 前の相手がまだ消えきっていないことがあるので名前は毎回変える
	const std::string name = std::string(kEnemyNamePrefix) + std::to_string(spawnSerial_++);

	Enemy* enemy = EnemySpawner::Spawn(entry.className, name, spawnPos, entry.colliderHalfExtents);
	if (!enemy) return;

	enemyName_ = name;

	enemy->SetupLockOn(lockOn_);
	ApplySettings(enemy);
	// 出現演出（黒い粒子の収束 + ディゾルブイン）を再生しながら起動する
	enemy->Spawn();
}

void TrainingController::RemoveCurrentEnemy()
{
	Enemy* enemy = GetEnemy();
	// 名前は先に手放す。死亡演出の間も FindObject では見つかるので、
	// 残したままだと「もう居ない相手」を今の相手として扱ってしまう
	enemyName_.clear();

	if (!enemy || !enemy->IsAlive()) return;

	// 死亡演出に乗せて消す。武器・ヒットボックスの後始末は
	// 各クラスの OnDeathEffectFinished が持っているので、この経路を通すのが確実
	enemy->SetDeathSuppressed(false);
	enemy->OnDeath();
}

void TrainingController::ApplySettings(Enemy* enemy)
{
	if (!enemy) return;

	enemy->SetDeathSuppressed(enemyInvincible_);
	enemy->SetAttackSuppressed(behavior_ != TrainingBehavior::Full);
	enemy->SetActionSuppressed(behavior_ == TrainingBehavior::Hold);

	// 無敵のときは HP も戻す。CanDie() だけだと 1 に張り付いてゲージが空に見えてしまう
	if (enemyInvincible_) {
		enemy->SetHp(enemy->GetMaxHp());
	}
}
