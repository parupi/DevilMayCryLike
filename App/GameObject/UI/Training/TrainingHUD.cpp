#include "TrainingHUD.h"

#include "GameData/Score/StylishScoreManager.h"
#include "GameObject/Character/Enemy/Enemy.h"
#include "GameObject/Character/Player/Player.h"
#include "GameObject/Training/TrainingController.h"

#include <Graphics/Rendering/Sprite/Sprite.h>
#include <Graphics/Rendering/Sprite/SpriteManager.h>
#include <Graphics/Text/TextLabel.h>
#include <World3D/Object/Model/Animation/AnimationPlayer.h>

#include <cstdio>
#include <string>

namespace {
	// 下敷きの濃さ。文字が読めれば十分なので薄めにして、後ろの様子を隠さない
	constexpr float kBackdropAlpha = 0.45f;

	const char* OnOff(bool value) { return value ? "ON" : "OFF"; }
}

void TrainingHUD::Initialize()
{
	SpriteManager& sprites = SpriteManager::GetInstance();

	// 下敷きは文字より先に作る（同じレイヤーでは後から作ったものが手前に出る）
	backdrop_ = sprites.CreateSprite(SpriteLayer::UI, "trainHudBackdrop", "white.png");
	backdrop_->SetAnchorPoint({ 0.0f, 0.0f });
	backdrop_->SetPosition({ kLeftX - 10.0f, kTopY - 16.0f });
	backdrop_->SetSize({ kBackdropWidth, kRowSpacing * kRowCount + 20.0f });

	for (int32_t i = 0; i < kRowCount; ++i) {
		TextLabel* label = sprites.CreateTextLabel(SpriteLayer::UI, "trainHud" + std::to_string(i));
		label->SetFontSize(kFontSize);
		label->SetAlign(TextAlignX::Left, TextAlignY::Middle);
		label->SetPosition({ kLeftX, kTopY + kRowSpacing * i });
		label->SetShadow(true);
		rows_[i] = label;
	}

	Hide();
}

void TrainingHUD::Hide()
{
	visible_ = false;

	backdrop_->GetRenderState().isVisible = false;
	backdrop_->Update();
	for (TextLabel* label : rows_) {
		label->GetRenderState().isVisible = false;
		label->Update();
	}
}

void TrainingHUD::Update(const TrainingController& controller, Player* player)
{
	if (!controller.IsHudVisible()) {
		Hide();
		return;
	}
	visible_ = true;

	backdrop_->SetColor({ 0.0f, 0.0f, 0.0f, kBackdropAlpha });
	backdrop_->GetRenderState().isVisible = true;
	backdrop_->Update();

	char buffer[256];

	// ── 相手 ──
	Enemy* enemy = controller.GetEnemy();

	snprintf(buffer, sizeof(buffer), "TRAINING  -  %s", controller.GetEnemyDisplayName().c_str());
	SetRow(Row::Title, buffer);

	if (enemy) {
		snprintf(buffer, sizeof(buffer), "ENEMY HP   %.1f / %.1f", enemy->GetHp(), enemy->GetMaxHp());
		SetRow(Row::EnemyHp, buffer);

		// 再生中のクリップは静的モデルの敵では取れない
		const char* clip = "-";
		if (AnimationPlayer* animation = enemy->GetAnimationPlayer()) {
			clip = animation->GetCurrentClipName().c_str();
		}
		snprintf(buffer, sizeof(buffer), "STATE      %s   /   CLIP  %s",
			enemy->GetCurrentStateName().c_str(), clip);
		SetRow(Row::EnemyState, buffer);

		snprintf(buffer, sizeof(buffer), "DAMAGE     %.1f   HITS %u   LAST %.1f",
			enemy->GetTotalDamageTaken(), enemy->GetDamageHitCount(), enemy->GetLastDamageTaken());
		SetRow(Row::Damage, buffer);
	} else {
		// 撃破直後と出し直しの合間。行を消さずに状態を出しておく
		SetRow(Row::EnemyHp, "ENEMY HP   -", 0.5f);
		SetRow(Row::EnemyState, "STATE      (RESPAWNING)", 0.5f);
		SetRow(Row::Damage, "DAMAGE     -", 0.5f);
	}

	// ── プレイヤー ──
	if (player) {
		snprintf(buffer, sizeof(buffer), "PLAYER HP  %d / %d", player->GetHp(), player->GetMaxHp());
		SetRow(Row::PlayerHp, buffer);

		if (StylishScoreManager* score = player->GetScoreManager()) {
			snprintf(buffer, sizeof(buffer), "RANK %s   STYLE %d   COMBO %u",
				score->GetCurrentRank().c_str(), score->GetCurrentScore(), score->GetComboCount());
		} else {
			snprintf(buffer, sizeof(buffer), "RANK -");
		}
		SetRow(Row::Score, buffer);
	} else {
		SetRow(Row::PlayerHp, "PLAYER HP  -", 0.5f);
		SetRow(Row::Score, "RANK -", 0.5f);
	}

	// ── 今の設定 ──
	snprintf(buffer, sizeof(buffer), "P.INV %s   E.INV %s   BEHAVIOR %s   AUTO %s",
		OnOff(controller.IsPlayerInvincible()), OnOff(controller.IsEnemyInvincible()),
		controller.GetBehaviorLabel(), OnOff(controller.IsAutoRespawn()));
	SetRow(Row::Settings, buffer);

	// ── 操作キー ──
	// 実装と同じ表を使うので、割り当てを変えても案内がずれない。
	// 1行に収まらないので前半・後半で折り返す
	const int32_t half = (TrainingController::kKeyHintCount + 1) / 2;
	std::string first;
	std::string second;
	for (int32_t i = 0; i < TrainingController::kKeyHintCount; ++i) {
		std::string& target = (i < half) ? first : second;
		if (!target.empty()) target += "   ";
		target += TrainingController::kKeyHints[i].key;
		target += ":";
		target += TrainingController::kKeyHints[i].action;
	}
	SetRow(Row::KeysA, first.c_str(), 0.6f);
	SetRow(Row::KeysB, second.c_str(), 0.6f);
}

void TrainingHUD::SetRow(Row row, const char* text, float brightness)
{
	TextLabel* label = Label(row);
	label->SetText(text);
	label->SetColor({ brightness, brightness, brightness, 1.0f });
	label->GetRenderState().isVisible = visible_;
	label->Update();
}
