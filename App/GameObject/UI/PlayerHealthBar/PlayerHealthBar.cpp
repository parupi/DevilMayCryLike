#include "PlayerHealthBar.h"
#include "Graphics/Rendering/Sprite/SpriteManager.h"
#include "Graphics/Rendering/Sprite/Sprite.h"
#include "Graphics/Resource/TextureManager.h"
#include <Utility/DeltaTime.h>
#include <Math/Vector3.h>
#include <algorithm>
#include <cmath>
#include <numbers>
#include <string>

namespace {

// ===== 色 =====
// フレームと飾りはボスのHPバーと同じ鈍い金。ゲージはボスの赤と取り違えないよう緑にしている
const Vector4 kShadowColor{ 0.0f, 0.0f, 0.0f, 0.55f };
const Vector4 kFrameColor{ 0.20f, 0.18f, 0.17f, 1.0f };
const Vector4 kFrameLineTopColor{ 0.86f, 0.72f, 0.45f, 1.0f };
const Vector4 kFrameLineBottomColor{ 0.42f, 0.33f, 0.20f, 1.0f };
const Vector4 kBackgroundColor{ 0.03f, 0.06f, 0.05f, 1.0f };
const Vector4 kDelayColor{ 0.95f, 0.92f, 0.88f, 1.0f };
const Vector4 kHpColor{ 0.22f, 0.66f, 0.38f, 1.0f };
const Vector4 kHpLowColor{ 0.72f, 0.10f, 0.08f, 1.0f };   // 瀕死の脈動で寄せる色
const Vector4 kHpFlashColor{ 1.0f, 0.42f, 0.34f, 1.0f };  // 被弾の一瞬
const Vector4 kHpGlossColor{ 1.0f, 1.0f, 1.0f, 0.16f };
const Vector4 kHpEdgeColor{ 0.85f, 1.0f, 0.88f, 0.9f };
const Vector4 kGoldColor{ 0.80f, 0.66f, 0.38f, 1.0f };
const Vector3 kHitFlashRgb{ 1.0f, 0.30f, 0.22f };
const Vector3 kSparkRgb{ 1.0f, 0.52f, 0.26f };

float Clamp01(float v) { return std::clamp(v, 0.0f, 1.0f); }
float EaseInOut(float t) { t = Clamp01(t); return t * t * (3.0f - 2.0f * t); }
Vector4 WithAlpha(const Vector4& c, float alpha) { return { c.x, c.y, c.z, c.w * alpha }; }
Vector4 Mix(const Vector4& a, const Vector4& b, float t) {
	return { a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t, a.z + (b.z - a.z) * t, a.w + (b.w - a.w) * t };
}

/// <summary>位置・大きさ・色・表示をまとめて入れて、表示するなら行列も更新する（BossHealthBar と同じ）</summary>
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

} // namespace

Sprite* PlayerHealthBar::CreateRect(const char* name, const char* texture) {
	Sprite* sprite = SpriteManager::GetInstance().CreateSprite(SpriteLayer::UI, name, texture);
	sprite->SetAnchorPoint({ 0.5f, 0.5f });
	return sprite;
}

void PlayerHealthBar::Initialize(int32_t maxHp) {
	TextureManager::GetInstance().LoadTexture("white.png");

	// 作った順に奥から描かれる（同じレイヤーは生成順）
	shadow_ = CreateRect("playerBarShadow");
	frame_ = CreateRect("playerBarFrame");
	frameLineTop_ = CreateRect("playerBarFrameTop");
	frameLineBottom_ = CreateRect("playerBarFrameBottom");

	// ゲージは左端を基準に伸び縮みさせる
	background_ = CreateRect("playerBarBack");
	background_->SetAnchorPoint({ 0.0f, 0.5f });
	delay_ = CreateRect("playerBarDelay");
	delay_->SetAnchorPoint({ 0.0f, 0.5f });
	hp_ = CreateRect("playerBarHp");
	hp_->SetAnchorPoint({ 0.0f, 0.5f });
	hpGloss_ = CreateRect("playerBarHpGloss");
	hpGloss_->SetAnchorPoint({ 0.0f, 0.0f });
	hpEdge_ = CreateRect("playerBarHpEdge");

	flash_ = CreateRect("playerBarFlash");
	flash_->SetAnchorPoint({ 0.0f, 0.5f });
	flash_->GetRenderState().blendMode = BlendMode::kAdd;

	capOuter_ = CreateRect("playerBarCapOuter");
	capInner_ = CreateRect("playerBarCapInner");

	for (size_t i = 0; i < sparks_.size(); ++i) {
		sparks_[i].sprite = CreateRect(("playerBarSpark" + std::to_string(i)).c_str());
		sparks_[i].sprite->GetRenderState().blendMode = BlendMode::kAdd;
	}

	lastHp_ = maxHp;
	hpRatio_ = 1.0f;
	delayRatio_ = 1.0f;
	delayFrom_ = 1.0f;
	initialized_ = true;
}

void PlayerHealthBar::OnDamaged(float previousRatio, float newRatio) {
	hpRatio_ = newRatio;

	// 白ゲージは今いる位置から追いかけ直す。続けて食らっている間は待ちが延び続ける
	delayFrom_ = delayRatio_;
	delayHoldTimer_ = kDelayHoldTime;
	delayCatchUpTimer_ = 0.0f;

	hitFlashTimer_ = kHitFlashTime;
	shakeTimer_ = kShakeTime;

	// 削れた位置（HPゲージの新しい先端）から火花。大きく削れたほど多い
	const float lost = (std::max)(0.0f, previousRatio - newRatio);
	const int32_t count = std::clamp(static_cast<int32_t>(3.0f + lost * 25.0f), 3, kSparkCount);
	EmitSparks(kLeft + kBarWidth * newRatio, kBarY, count);
}

void PlayerHealthBar::EmitSparks(float x, float y, int32_t count) {
	const float pi = std::numbers::pi_v<float>;
	std::uniform_real_distribution<float> angleDist(-pi * 0.95f, -pi * 0.05f); // 画面の上向き半分
	std::uniform_real_distribution<float> speedDist(70.0f, 200.0f);
	std::uniform_real_distribution<float> lifeDist(0.16f, 0.28f);
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

void PlayerHealthBar::UpdateDelayGauge(float dt) {
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

void PlayerHealthBar::UpdateSparks(float dt) {
	for (Spark& spark : sparks_) {
		if (spark.life <= 0.0f) continue;
		spark.life -= dt;
		spark.position.x += spark.velocity.x * dt;
		spark.position.y += spark.velocity.y * dt;
		spark.velocity.y += 520.0f * dt; // 少し落ちながら消える
	}
}

void PlayerHealthBar::Update(int32_t hp, int32_t maxHp, float alpha) {
	if (!initialized_) return;

	// 演出は実時間で進める（被弾のヒットストップ中も白ゲージの待ちと揺れは進めたい）
	const float dt = DeltaTime::GetDeltaTime();
	pulseTime_ += dt;

	const float ratio = (maxHp > 0) ? Clamp01(static_cast<float>(hp) / static_cast<float>(maxHp)) : 0.0f;
	if (hp < lastHp_) {
		OnDamaged(hpRatio_, ratio);
	} else if (hp > lastHp_) {
		// 回復は演出なしでそのまま合わせる
		hpRatio_ = ratio;
		delayRatio_ = (std::max)(delayRatio_, ratio);
	}
	lastHp_ = hp;

	hitFlashTimer_ = (std::max)(0.0f, hitFlashTimer_ - dt);
	shakeTimer_ = (std::max)(0.0f, shakeTimer_ - dt);
	UpdateDelayGauge(dt);
	UpdateSparks(dt);

	ApplyLayout(alpha);
}

void PlayerHealthBar::ApplyLayout(float alpha) {
	const float pi = std::numbers::pi_v<float>;
	const float a = Clamp01(alpha);
	const bool show = a > 0.001f;

	// ===== 揺れ（左右へ小刻みに、減衰しながら） =====
	float shakeX = 0.0f;
	if (shakeTimer_ > 0.0f) {
		const float k = shakeTimer_ / kShakeTime;
		shakeX = kShakeAmplitude * k * std::sin(shakeTimer_ * 90.0f);
	}

	const float left = kLeft + shakeX;
	const float w = kBarWidth;
	const float h = kBarHeight;
	const float cx = left + w * 0.5f;
	const float cy = kBarY;
	const float top = cy - h * 0.5f;
	const float frameW = w + kFramePad * 2.0f;
	const float frameH = h + kFramePad * 2.0f;
	const float hitK = hitFlashTimer_ / kHitFlashTime;

	// ===== フレームと背景 =====
	Place(shadow_, show, { cx + 2.0f, cy + 3.0f }, { frameW + 8.0f, frameH + 8.0f }, WithAlpha(kShadowColor, a));
	Place(frame_, show, { cx, cy }, { frameW, frameH }, WithAlpha(kFrameColor, a));
	Place(frameLineTop_, show, { cx, cy - frameH * 0.5f + 0.75f }, { frameW, 1.5f }, WithAlpha(kFrameLineTopColor, a));
	Place(frameLineBottom_, show, { cx, cy + frameH * 0.5f - 0.75f }, { frameW, 1.5f }, WithAlpha(kFrameLineBottomColor, a));
	Place(background_, show, { left, cy }, { w, h }, WithAlpha(kBackgroundColor, a));

	// ===== ゲージ =====
	Place(delay_, show && delayRatio_ > 0.0f, { left, cy }, { w * delayRatio_, h }, WithAlpha(kDelayColor, a));

	// 瀕死は赤へ脈打たせる（心音と同じ境目）。被弾の一瞬はさらに明るい赤
	const bool lowHealth = hpRatio_ > 0.0f && hpRatio_ <= kLowHealthRatio;
	const float lowK = lowHealth ? 0.5f + 0.5f * std::sin(pulseTime_ * kLowHealthPulseSpeed) : 0.0f;
	const Vector4 baseColor = lowHealth ? Mix(kHpLowColor, kHpColor, 0.35f * (1.0f - lowK)) : kHpColor;
	const Vector4 hpColor = Mix(baseColor, kHpFlashColor, hitK);
	Place(hp_, show && hpRatio_ > 0.0f, { left, cy }, { w * hpRatio_, h }, WithAlpha(hpColor, a));
	Place(hpGloss_, show && hpRatio_ > 0.0f, { left, top }, { w * hpRatio_, h * 0.4f }, WithAlpha(kHpGlossColor, a));
	Place(hpEdge_, show && hpRatio_ > 0.001f && hpRatio_ < 0.999f, { left + w * hpRatio_, cy }, { 2.0f, h }, WithAlpha(kHpEdgeColor, a));

	// ===== 被弾の発光 =====
	Place(flash_, show && hitK > 0.0f, { left, cy }, { w, h }, { kHitFlashRgb.x, kHitFlashRgb.y, kHitFlashRgb.z, 0.55f * hitK * a });

	// ===== 左端の飾り。芯はゲージと同じ色にして、瀕死の脈動も一緒に見せる =====
	const float capX = left - kFramePad - 8.0f;
	Place(capOuter_, show, { capX, cy }, { 18.0f, 18.0f }, WithAlpha(kGoldColor, a), pi * 0.25f);
	Place(capInner_, show, { capX, cy }, { 9.0f, 9.0f }, WithAlpha(hpRatio_ > 0.0f ? hpColor : kBackgroundColor, a), pi * 0.25f);

	// ===== 火花 =====
	for (Spark& spark : sparks_) {
		const bool alive = spark.life > 0.0f;
		const float k = alive ? spark.life / spark.maxLife : 0.0f;
		const float rotation = std::atan2(spark.velocity.y, spark.velocity.x);
		Place(spark.sprite, show && alive, spark.position, { 7.0f, 2.0f }, { kSparkRgb.x, kSparkRgb.y, kSparkRgb.z, k * a }, rotation);
	}
}
