#include "BossHealthBar.h"
#include "Graphics/Rendering/Sprite/SpriteManager.h"
#include "Graphics/Rendering/Sprite/Sprite.h"
#include "Graphics/Resource/TextureManager.h"
#include "GameObject/Character/Enemy/BossKnight/BossKnight.h"
#include "GameObject/Character/Enemy/BossKnight/State/BossStateCombatIdle.h"
#include <World3D/Object/Object3dManager.h>
#include <Utility/DeltaTime.h>
#include "Audio/GameSoundLibrary.h"
#include "Audio/SoundManager.h"
#include <Math/Vector3.h>
#include <algorithm>
#include <cmath>
#include <numbers>
#include <string>

namespace {

// ===== 色 =====
// HealthBar.md: 背景は黒寄りの暗い赤、HPは濃い赤、遅延ゲージは白〜薄いグレー、フレームは金属調。
// ゲーム内の他の飾り（タイトルの飾り罫など）に合わせて、金属は鈍い金にしている
const Vector4 kShadowColor{ 0.0f, 0.0f, 0.0f, 0.55f };
const Vector4 kFrameColor{ 0.20f, 0.18f, 0.17f, 1.0f };
const Vector4 kFrameLineTopColor{ 0.86f, 0.72f, 0.45f, 1.0f };
const Vector4 kFrameLineBottomColor{ 0.42f, 0.33f, 0.20f, 1.0f };
const Vector4 kBackgroundColor{ 0.10f, 0.015f, 0.02f, 1.0f };
const Vector4 kBackgroundGlossColor{ 0.40f, 0.08f, 0.09f, 0.30f };
const Vector4 kDelayColor{ 0.95f, 0.92f, 0.88f, 1.0f };
const Vector4 kHpColor{ 0.58f, 0.02f, 0.04f, 1.0f };
const Vector4 kHpFlashColor{ 1.0f, 0.36f, 0.30f, 1.0f };
const Vector4 kHpGlossColor{ 1.0f, 0.40f, 0.35f, 0.18f };
const Vector4 kHpEdgeColor{ 1.0f, 0.78f, 0.62f, 0.9f };
const Vector4 kTickColor{ 0.02f, 0.01f, 0.01f, 0.85f };
const Vector4 kGoldColor{ 0.80f, 0.66f, 0.38f, 1.0f };
const Vector4 kCapInnerColor{ 0.14f, 0.02f, 0.03f, 1.0f };
const Vector4 kNameColor{ 0.93f, 0.89f, 0.82f, 1.0f };
const Vector4 kNamePhaseColor{ 1.0f, 0.62f, 0.36f, 1.0f };
const Vector3 kHitFlashRgb{ 1.0f, 0.30f, 0.22f };
const Vector3 kSparkRgb{ 1.0f, 0.52f, 0.26f };
const Vector3 kRippleInnerRgb{ 1.0f, 1.0f, 1.0f };
const Vector3 kRippleOuterRgb{ 1.0f, 0.50f, 0.18f };

float Clamp01(float v) { return std::clamp(v, 0.0f, 1.0f); }
float EaseOutCubic(float t) { const float u = 1.0f - Clamp01(t); return 1.0f - u * u * u; }
float EaseInOut(float t) { t = Clamp01(t); return t * t * (3.0f - 2.0f * t); }
Vector4 WithAlpha(const Vector4& c, float alpha) { return { c.x, c.y, c.z, c.w * alpha }; }
Vector4 Mix(const Vector4& a, const Vector4& b, float t) {
	return { a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t, a.z + (b.z - a.z) * t, a.w + (b.w - a.w) * t };
}

/// <summary>位置・大きさ・色・表示をまとめて入れて、表示するなら行列も更新する</summary>
void Place(Sprite* sprite, bool show, const Vector2& position, const Vector2& size, const Vector4& color, float rotation = 0.0f) {
	if (!sprite) return;
	const bool visible = show && color.w > 0.001f && size.x > 0.01f && size.y > 0.01f;
	sprite->GetRenderState().isVisible = visible;
	if (!visible) return;
	sprite->SetPosition(position);
	sprite->SetSize(size);
	sprite->SetColor(color);
	sprite->SetRotation(rotation);
	sprite->Update();
}

// フェーズの境目（BossStateCombatIdle と同じ値）。目盛りを打つ位置
const float kPhaseRatios[2] = { BossPhase::kPhase2HpRatio, BossPhase::kPhase3HpRatio };

} // namespace

Sprite* BossHealthBar::CreateRect(const char* name, const char* texture) {
	Sprite* sprite = SpriteManager::GetInstance().CreateSprite(SpriteLayer::UI, name, texture);
	sprite->SetAnchorPoint({ 0.5f, 0.5f });
	return sprite;
}

void BossHealthBar::Initialize() {
	TextureManager::GetInstance().LoadTexture("white.png");
	TextureManager::GetInstance().LoadTexture("SoftCircle.png");

	// 作った順に奥から描かれる（同じレイヤーは生成順）
	shadow_ = CreateRect("bossBarShadow");
	frame_ = CreateRect("bossBarFrame");
	frameLineTop_ = CreateRect("bossBarFrameTop");
	frameLineBottom_ = CreateRect("bossBarFrameBottom");

	// ゲージは左端を基準に伸び縮みさせる
	background_ = CreateRect("bossBarBack");
	background_->SetAnchorPoint({ 0.0f, 0.5f });
	backgroundGloss_ = CreateRect("bossBarBackGloss");
	backgroundGloss_->SetAnchorPoint({ 0.0f, 0.0f });
	delay_ = CreateRect("bossBarDelay");
	delay_->SetAnchorPoint({ 0.0f, 0.5f });
	hp_ = CreateRect("bossBarHp");
	hp_->SetAnchorPoint({ 0.0f, 0.5f });
	hpGloss_ = CreateRect("bossBarHpGloss");
	hpGloss_->SetAnchorPoint({ 0.0f, 0.0f });
	hpEdge_ = CreateRect("bossBarHpEdge");
	for (size_t i = 0; i < ticks_.size(); ++i) {
		ticks_[i] = CreateRect(("bossBarTick" + std::to_string(i)).c_str());
	}

	// 光りものは加算で重ねる
	flash_ = CreateRect("bossBarFlash");
	flash_->SetAnchorPoint({ 0.0f, 0.5f });
	flash_->GetRenderState().blendMode = BlendMode::kAdd;
	glow_ = CreateRect("bossBarGlow");
	glow_->GetRenderState().blendMode = BlendMode::kAdd;
	for (size_t i = 0; i < sweeps_.size(); ++i) {
		sweeps_[i] = CreateRect(("bossBarSweep" + std::to_string(i)).c_str(), "SoftCircle.png");
		sweeps_[i]->GetRenderState().blendMode = BlendMode::kAdd;
	}

	// 両端の飾り
	for (size_t i = 0; i < capOuter_.size(); ++i) {
		capLines_[i] = CreateRect(("bossBarCapLine" + std::to_string(i)).c_str());
		// 左は右端、右は左端を基準にして、フレームから外へ伸ばす
		capLines_[i]->SetAnchorPoint({ i == 0 ? 1.0f : 0.0f, 0.5f });
		capOuter_[i] = CreateRect(("bossBarCapOuter" + std::to_string(i)).c_str());
		capInner_[i] = CreateRect(("bossBarCapInner" + std::to_string(i)).c_str());
	}

	ripple_ = CreateRect("bossBarRipple", "SoftCircle.png");
	ripple_->GetRenderState().blendMode = BlendMode::kAdd;
	rippleLine_ = CreateRect("bossBarRippleLine");
	rippleLine_->GetRenderState().blendMode = BlendMode::kAdd;

	for (size_t i = 0; i < sparks_.size(); ++i) {
		sparks_[i].sprite = CreateRect(("bossBarSpark" + std::to_string(i)).c_str());
		sparks_[i].sprite->GetRenderState().blendMode = BlendMode::kAdd;
	}

	// ボスの名前。他のUIと同じ白文字＋黒い影
	name_ = SpriteManager::GetInstance().CreateTextLabel(SpriteLayer::UI, "bossBarName");
	name_->SetFontSize(kNameFontSize);
	name_->SetAlign(TextAlignX::Left, TextAlignY::Middle);
	name_->SetShadow(true, { 2.0f, 2.0f }, { 0.0f, 0.0f, 0.0f, 0.8f });

	state_ = State::Hidden;
	SetVisible(false);
}

BossKnight* BossHealthBar::FindBoss() const {
	Object3dManager& objects = Object3dManager::GetInstance();

	// 追いかけている相手がいるなら、その名前で引き直す（倒されて削除されていれば見つからない）
	if (!bossName_.empty()) {
		return dynamic_cast<BossKnight*>(objects.FindObject(bossName_));
	}

	// まだ誰も追っていない。出現演出（ディゾルブ）まで終わって、戦える状態のボスを探す
	for (auto* object : objects.GetAllObject()) {
		auto* boss = dynamic_cast<BossKnight*>(object);
		if (boss && boss->IsActive() && boss->IsAlive() && !boss->IsDying() && !boss->IsAppearanceEffectPlaying()) {
			return boss;
		}
	}
	return nullptr;
}

void BossHealthBar::StartIntro(BossKnight& boss) {
	state_ = State::Intro;
	stateTimer_ = 0.0f;
	// バーがせり上がってくる音
	SoundManager::GetInstance().PlaySE(GameSound::kBossBarAppear, 0.7f);
	bossName_ = boss.name_;
	name_->SetText(BossKnight::kDisplayName);

	lastHp_ = boss.GetHp();
	lastPhase_ = boss.GetShownPhase();
	hpRatio_ = boss.GetHpRatio();
	delayRatio_ = hpRatio_;
	delayFrom_ = hpRatio_;
	delayHoldTimer_ = 0.0f;
	delayCatchUpTimer_ = 0.0f;

	hitFlashTimer_ = 0.0f;
	shakeTimer_ = 0.0f;
	phaseFreezeTimer_ = 0.0f;
	phaseEffectPending_ = false;
	phaseGlowTimer_ = 0.0f;
	rippleTimer_ = 0.0f;
}

void BossHealthBar::StartDefeat() {
	state_ = State::Defeat;
	stateTimer_ = 0.0f;
	// 削り切った音。ボスの断末魔と重なるので控えめにする
	SoundManager::GetInstance().PlaySE(GameSound::kBossBarDeplete, 0.6f);
	// 赤ゲージはその場で消し、白ゲージを今の位置から0まで追いつかせる
	hpRatio_ = 0.0f;
	delayFrom_ = delayRatio_;
	phaseFreezeTimer_ = 0.0f;
	phaseEffectPending_ = false;
}

void BossHealthBar::OnDamaged(float previousRatio, float newRatio) {
	hpRatio_ = newRatio;

	// 白ゲージは今いる位置から追いかけ直す。連続で当てている間は待ちが延び続けるので、
	// コンボ中は白い削れ跡が残り、手を止めたところでまとめて縮む
	delayFrom_ = delayRatio_;
	delayHoldTimer_ = kDelayHoldTime;
	delayCatchUpTimer_ = 0.0f;

	hitFlashTimer_ = kHitFlashTime;
	shakeTimer_ = kShakeTime;

	// 削れた位置（赤ゲージの新しい先端）から火花。大きく削れたほど多い
	const float lost = (std::max)(0.0f, previousRatio - newRatio);
	const int32_t count = std::clamp(static_cast<int32_t>(4.0f + lost * 80.0f), 4, 10);
	EmitSparks(kCenterX - kBarWidth * 0.5f + kBarWidth * newRatio, kBarY, count);
}

void BossHealthBar::OnPhaseChanged() {
	// まず一瞬止めてから、白い発光と波紋を出す（UpdateTimers が止め終わりで始める）
	phaseFreezeTimer_ = kPhaseFreezeTime;
	// 咆哮に重ねる。咆哮そのものより短く軽い音にして、UI の合図だと分かるようにする
	SoundManager::GetInstance().PlaySE(GameSound::kBossBarPhase, 0.55f);
	phaseEffectPending_ = true;
	shakeTimer_ = 0.0f;
}

void BossHealthBar::EmitSparks(float x, float y, int32_t count) {
	const float pi = std::numbers::pi_v<float>;
	std::uniform_real_distribution<float> angleDist(-pi * 0.95f, -pi * 0.05f); // 画面の上向き半分
	std::uniform_real_distribution<float> speedDist(90.0f, 240.0f);
	std::uniform_real_distribution<float> lifeDist(0.18f, 0.32f);
	std::uniform_real_distribution<float> jitterDist(-kBarHeight * 0.4f, kBarHeight * 0.4f);

	for (Spark& spark : sparks_) {
		if (count <= 0) break;
		if (spark.life > 0.0f) continue;
		const float angle = angleDist(rng_);
		const float speed = speedDist(rng_);
		spark.position = { x, y + jitterDist(rng_) };
		spark.velocity = { std::cos(angle) * speed, std::sin(angle) * speed };
		spark.maxLife = lifeDist(rng_);
		spark.life = spark.maxLife;
		--count;
	}
}

void BossHealthBar::UpdateTimers(float dt) {
	hitFlashTimer_ = (std::max)(0.0f, hitFlashTimer_ - dt);
	phaseGlowTimer_ = (std::max)(0.0f, phaseGlowTimer_ - dt);
	rippleTimer_ = (std::max)(0.0f, rippleTimer_ - dt);

	if (phaseFreezeTimer_ > 0.0f) {
		phaseFreezeTimer_ -= dt;
		if (phaseFreezeTimer_ <= 0.0f) {
			phaseFreezeTimer_ = 0.0f;
			if (phaseEffectPending_) {
				phaseEffectPending_ = false;
				phaseGlowTimer_ = kPhaseGlowTime;
				rippleTimer_ = kRippleTime;
			}
		}
		// 止まっている間は揺れも進めない
		return;
	}
	shakeTimer_ = (std::max)(0.0f, shakeTimer_ - dt);
}

void BossHealthBar::UpdateDelayGauge(float dt) {
	// フェーズ移行の「一瞬止まる」間は白ゲージも止める
	if (phaseFreezeTimer_ > 0.0f) return;

	if (delayRatio_ <= hpRatio_) {
		delayRatio_ = hpRatio_;
		return;
	}
	if (delayHoldTimer_ > 0.0f) {
		delayHoldTimer_ -= dt;
		return;
	}
	delayCatchUpTimer_ += dt;
	const float t = EaseInOut(delayCatchUpTimer_ / kDelayCatchUpTime);
	delayRatio_ = delayFrom_ + (hpRatio_ - delayFrom_) * t;
}

void BossHealthBar::UpdateSparks(float dt) {
	for (Spark& spark : sparks_) {
		if (spark.life <= 0.0f) continue;
		spark.life -= dt;
		spark.position.x += spark.velocity.x * dt;
		spark.position.y += spark.velocity.y * dt;
		spark.velocity.y += 520.0f * dt; // 少し落ちながら消える
	}
}

void BossHealthBar::Update(bool hudVisible) {
	const float dt = DeltaTime::GetDeltaTime();
	pulseTime_ += dt;

	switch (state_) {
	case State::Hidden:
		if (BossKnight* boss = FindBoss()) {
			StartIntro(*boss);
		}
		break;

	case State::Intro:
	case State::Active: {
		BossKnight* boss = FindBoss();
		// 倒した（死亡演出に入った・消えた）なら撃破演出へ
		if (!boss || !boss->IsAlive() || boss->IsDying()) {
			StartDefeat();
			break;
		}

		const float hp = boss->GetHp();
		const float ratio = boss->GetHpRatio();
		if (hp < lastHp_ - 1.0e-4f) {
			OnDamaged(hpRatio_, ratio);
		} else if (hp > lastHp_ + 1.0e-4f) {
			// 回復（トレーニングのリセットなど）は演出なしでそのまま合わせる
			hpRatio_ = ratio;
			delayRatio_ = (std::max)(delayRatio_, ratio);
		}
		lastHp_ = hp;

		// フェーズ移行はボスの咆哮に合わせる（shownPhase は咆哮に入る瞬間に進む）
		const int32_t phase = boss->GetShownPhase();
		if (phase > lastPhase_) {
			OnPhaseChanged();
		}
		lastPhase_ = phase;

		if (state_ == State::Intro) {
			stateTimer_ += dt;
			if (stateTimer_ >= kIntroNameTime + kIntroExpandTime + kIntroGlowTime + kIntroGlowFade) {
				state_ = State::Active;
			}
		}
		UpdateDelayGauge(dt);
		break;
	}

	case State::Defeat:
		stateTimer_ += dt;
		// 白ゲージが最後まで追いつく → 縮みながら消える
		delayRatio_ = delayFrom_ * (1.0f - EaseInOut(stateTimer_ / kDefeatDrainTime));
		if (stateTimer_ >= kDefeatDrainTime + kDefeatFadeTime) {
			state_ = State::Hidden;
			bossName_.clear();
		}
		break;
	}

	UpdateTimers(dt);
	UpdateSparks(dt);

	visible_ = hudVisible && state_ != State::Hidden;
	if (!visible_) {
		SetVisible(false);
		return;
	}
	ApplyLayout();
}

void BossHealthBar::ApplyLayout() {
	const float pi = std::numbers::pi_v<float>;

	// ===== 登場・撃破の進み具合 =====
	float nameAlpha = 1.0f;
	float barAlpha = 1.0f;
	float expand = 1.0f;     // 中央から横へ伸びる割合
	float slideY = 0.0f;     // 上から少し滑り込む
	float nameSlideX = 0.0f;
	float introGlow = 0.0f;
	float sweepT = -1.0f;    // 0〜1 の間だけ光が外へ流れる
	float shrink = 1.0f;     // 撃破で少し縮む
	float defeatGlow = 0.0f;

	if (state_ == State::Intro) {
		const float t = stateTimer_;
		nameAlpha = Clamp01(t / kIntroNameTime);
		nameSlideX = (1.0f - EaseOutCubic(t / kIntroNameTime)) * 14.0f;
		barAlpha = Clamp01((t - kIntroNameTime * 0.5f) / (kIntroNameTime * 0.5f));
		expand = EaseOutCubic((t - kIntroNameTime) / kIntroExpandTime);
		slideY = -(1.0f - EaseOutCubic(t / (kIntroNameTime + kIntroExpandTime))) * 10.0f;

		const float glowStart = kIntroNameTime + kIntroExpandTime;
		if (t >= glowStart) {
			const float gt = t - glowStart;
			introGlow = (gt < kIntroGlowTime) ? 1.0f : 1.0f - Clamp01((gt - kIntroGlowTime) / kIntroGlowFade);
			sweepT = Clamp01(gt / (kIntroGlowTime + kIntroGlowFade));
		}
	} else if (state_ == State::Defeat) {
		const float t = stateTimer_;
		nameAlpha = 1.0f - Clamp01(t / kDefeatDrainTime);
		const float fadeT = Clamp01((t - kDefeatDrainTime) / kDefeatFadeTime);
		barAlpha = 1.0f - fadeT;
		shrink = 1.0f - 0.12f * EaseOutCubic(fadeT);
		// 光を残して消える
		defeatGlow = (t >= kDefeatDrainTime) ? 0.35f * (1.0f - fadeT) : 0.0f;
	}

	// ===== 揺れ（左右へ小刻みに、減衰しながら） =====
	float shakeX = 0.0f;
	if (shakeTimer_ > 0.0f && phaseFreezeTimer_ <= 0.0f) {
		const float k = shakeTimer_ / kShakeTime;
		shakeX = kShakeAmplitude * k * std::sin(shakeTimer_ * 90.0f);
	}

	const float cx = kCenterX + shakeX;
	const float cy = kBarY + slideY;
	const float fullWidth = kBarWidth * shrink;
	const float w = (std::max)(8.0f, fullWidth * expand);
	const float h = kBarHeight * shrink;
	const float left = cx - w * 0.5f;
	const float top = cy - h * 0.5f;
	const float frameW = w + kFramePad * 2.0f;
	const float frameH = h + kFramePad * 2.0f;

	const float hitK = hitFlashTimer_ / kHitFlashTime;
	const float phaseGlowK = phaseGlowTimer_ / kPhaseGlowTime;
	const float a = barAlpha;

	// ===== フレームと背景 =====
	Place(shadow_, true, { cx + 2.0f, cy + 3.0f }, { frameW + 8.0f, frameH + 8.0f }, WithAlpha(kShadowColor, a));
	Place(frame_, true, { cx, cy }, { frameW, frameH }, WithAlpha(kFrameColor, a));
	Place(frameLineTop_, true, { cx, cy - frameH * 0.5f + 0.75f }, { frameW, 1.5f }, WithAlpha(kFrameLineTopColor, a));
	Place(frameLineBottom_, true, { cx, cy + frameH * 0.5f - 0.75f }, { frameW, 1.5f }, WithAlpha(kFrameLineBottomColor, a));
	Place(background_, true, { left, cy }, { w, h }, WithAlpha(kBackgroundColor, a));
	Place(backgroundGloss_, true, { left, top }, { w, h * 0.45f }, WithAlpha(kBackgroundGlossColor, a));

	// ===== ゲージ =====
	Place(delay_, delayRatio_ > 0.0f, { left, cy }, { w * delayRatio_, h }, WithAlpha(kDelayColor, a));

	// 最終フェーズは赤ゲージの光沢をゆっくり脈打たせて、追い詰めたことを見せる
	const float pulse = (lastPhase_ >= 3 && state_ != State::Defeat)
		? 0.15f * (0.5f + 0.5f * std::sin(pulseTime_ * 5.0f)) : 0.0f;
	const Vector4 hpColor = Mix(kHpColor, kHpFlashColor, hitK);
	Place(hp_, hpRatio_ > 0.0f, { left, cy }, { w * hpRatio_, h }, WithAlpha(hpColor, a));
	Place(hpGloss_, hpRatio_ > 0.0f, { left, top }, { w * hpRatio_, h * 0.4f },
		WithAlpha({ kHpGlossColor.x, kHpGlossColor.y, kHpGlossColor.z, kHpGlossColor.w + pulse }, a));
	Place(hpEdge_, hpRatio_ > 0.001f && hpRatio_ < 0.999f, { left + w * hpRatio_, cy }, { 2.0f, h }, WithAlpha(kHpEdgeColor, a));

	for (size_t i = 0; i < ticks_.size(); ++i) {
		Place(ticks_[i], true, { left + w * kPhaseRatios[i], cy }, { 2.0f, h }, WithAlpha(kTickColor, a));
	}

	// ===== 発光 =====
	// 被弾は赤、フェーズ移行は白
	if (phaseGlowK > 0.0f) {
		Place(flash_, true, { left, cy }, { w, h }, { 1.0f, 1.0f, 1.0f, 0.8f * phaseGlowK * a });
	} else {
		Place(flash_, hitK > 0.0f, { left, cy }, { w, h }, { kHitFlashRgb.x, kHitFlashRgb.y, kHitFlashRgb.z, 0.55f * hitK * a });
	}

	const float glowAlpha = (std::max)({ introGlow * 0.9f, phaseGlowK * 0.6f, defeatGlow });
	Place(glow_, glowAlpha > 0.0f, { cx, cy }, { frameW + 8.0f, frameH + 8.0f },
		{ 1.0f, 1.0f, 1.0f, glowAlpha * (state_ == State::Defeat ? 1.0f : a) });

	// 登場時、フレームの上を中央から外へ光が流れる
	const bool sweeping = sweepT >= 0.0f && sweepT < 1.0f;
	for (size_t i = 0; i < sweeps_.size(); ++i) {
		const float dir = (i == 0) ? -1.0f : 1.0f;
		Place(sweeps_[i], sweeping, { cx + dir * w * 0.5f * EaseOutCubic(sweepT), cy },
			{ 48.0f, frameH + 14.0f }, { 1.0f, 1.0f, 1.0f, (1.0f - sweepT) * a });
	}

	// ===== 両端の飾り =====
	for (size_t i = 0; i < capOuter_.size(); ++i) {
		const float dir = (i == 0) ? -1.0f : 1.0f;
		const float edgeX = cx + dir * (frameW * 0.5f + 6.0f);
		Place(capLines_[i], true, { cx + dir * (frameW * 0.5f + 14.0f), cy }, { 26.0f * expand, 2.0f }, WithAlpha(kGoldColor, 0.85f * a));
		Place(capOuter_[i], true, { edgeX, cy }, { 16.0f, 16.0f }, WithAlpha(kGoldColor, a), pi * 0.25f);
		Place(capInner_[i], true, { edgeX, cy }, { 8.0f, 8.0f }, WithAlpha(kCapInnerColor, a), pi * 0.25f);
	}

	// ===== フェーズ移行の波紋（バー中央から外へ） =====
	if (rippleTimer_ > 0.0f) {
		const float rt = 1.0f - rippleTimer_ / kRippleTime;
		const float e = EaseOutCubic(rt);
		const Vector4 rippleColor{
			kRippleInnerRgb.x + (kRippleOuterRgb.x - kRippleInnerRgb.x) * rt,
			kRippleInnerRgb.y + (kRippleOuterRgb.y - kRippleInnerRgb.y) * rt,
			kRippleInnerRgb.z + (kRippleOuterRgb.z - kRippleInnerRgb.z) * rt,
			0.9f * (1.0f - rt) * (1.0f - rt) * a };
		Place(ripple_, true, { cx, cy }, { w * (0.15f + 1.15f * e), 24.0f + 70.0f * e }, rippleColor);
		Place(rippleLine_, true, { cx, cy }, { w * 1.1f * e, 3.0f * (1.0f - rt) + 1.0f }, { 1.0f, 0.9f, 0.75f, (1.0f - rt) * a });
	} else {
		Place(ripple_, false, {}, {}, {});
		Place(rippleLine_, false, {}, {}, {});
	}

	// ===== 火花 =====
	for (Spark& spark : sparks_) {
		const bool alive = spark.life > 0.0f;
		const float k = alive ? spark.life / spark.maxLife : 0.0f;
		const float rotation = std::atan2(spark.velocity.y, spark.velocity.x);
		Place(spark.sprite, alive, spark.position, { 8.0f, 2.0f }, { kSparkRgb.x, kSparkRgb.y, kSparkRgb.z, k }, rotation);
	}

	// ===== 名前（バーの左上。バーの伸びに関係なく固定の位置） =====
	name_->GetRenderState().isVisible = nameAlpha > 0.001f;
	if (nameAlpha > 0.001f) {
		const Vector4 nameColor = Mix(kNameColor, kNamePhaseColor, phaseGlowK);
		name_->SetPosition({ kCenterX - kBarWidth * 0.5f + shakeX + nameSlideX, kBarY + kNameOffsetY + slideY });
		name_->SetColor(WithAlpha(nameColor, nameAlpha));
		name_->Update();
	}
}

void BossHealthBar::SetVisible(bool visible) {
	for (Sprite* sprite : { shadow_, frame_, frameLineTop_, frameLineBottom_, background_, backgroundGloss_,
		delay_, hp_, hpGloss_, hpEdge_, flash_, glow_, ripple_, rippleLine_ }) {
		if (sprite) sprite->GetRenderState().isVisible = visible;
	}
	for (auto* sprites : { &ticks_, &sweeps_, &capLines_, &capOuter_, &capInner_ }) {
		for (Sprite* sprite : *sprites) {
			if (sprite) sprite->GetRenderState().isVisible = visible;
		}
	}
	for (Spark& spark : sparks_) {
		if (spark.sprite) spark.sprite->GetRenderState().isVisible = visible && spark.life > 0.0f;
	}
	if (name_) name_->GetRenderState().isVisible = visible;
}
