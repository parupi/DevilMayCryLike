#define NOMINMAX
#include "ClearUI.h"

#include "Audio/GameSoundLibrary.h"
#include "Audio/SoundManager.h"
#include "GameObject/UI/Common/MenuNavigator.h"
#include "Graphics/Rendering/Sprite/Sprite.h"
#include "Graphics/Rendering/Sprite/SpriteManager.h"
#include "Graphics/Resource/TextureManager.h"
#include "Graphics/Text/FontManager.h"
#include "Graphics/Text/TextLabel.h"
#include "Scene/Clear/ClearSequence.h"

#include <algorithm>
#include <cmath>
#include <random>

namespace {

// STAGE CLEAR とランクの文字。64px で焼いた既定のフォントを大きく出すとぼやけるので、大きく焼いたものを別に持つ
constexpr const char* kDisplayFont = "ClearDisplay";
constexpr const char* kDisplayFontPath = "Resource/Font/Noto_Sans_JP/static/NotoSansJP-Bold.ttf";
constexpr float kDisplayFontBakeSize = 160.0f;
constexpr uint32_t kDisplayFontAtlasSize = 2048;

constexpr const char* kGlowTexture = "SoftCircle.png";
constexpr const char* kRingTexture = "ClearRing.png";
constexpr const char* kLineTexture = "white.png";

// 画面左の列（1280x720 の UI 座標）。プレイヤーは画面の右に立つ
constexpr float kLeft = 92.0f;
constexpr float kStageClearY = 150.0f;
constexpr float kStageClearSize = 70.0f;
constexpr float kLineY = 198.0f;
constexpr float kScoreCaptionY = 254.0f;
constexpr float kScoreValueY = 304.0f;
constexpr float kScoreValueSize = 58.0f;
constexpr float kRankCaptionY = 396.0f;
constexpr float kRankY = 486.0f;
constexpr float kRankSize = 150.0f;
constexpr float kCaptionSize = 22.0f;
constexpr float kMenuY = 640.0f;
constexpr float kMenuSize = 34.0f;
constexpr std::array<float, 2> kMenuX = { kLeft + 64.0f, kLeft + 250.0f };

const Vector4 kTextColor = { 0.97f, 0.95f, 0.90f, 1.0f };
const Vector4 kCaptionColor = { 0.70f, 0.73f, 0.78f, 1.0f };
const Vector4 kWarmColor = { 1.0f, 0.84f, 0.58f, 1.0f };
const Vector4 kMenuIdleColor = { 0.52f, 0.55f, 0.60f, 1.0f };
const Vector2 kShadowOffset = { 3.0f, 3.0f };

// スコアのカウント音の間隔[秒]。毎フレーム鳴らすと連続音になって耳障り
constexpr float kTickInterval = 0.07f;
// スコアが確定した瞬間に数字を軽く膨らませる時間[秒]と大きさ
constexpr float kScorePulseTime = 0.25f;
constexpr float kScorePulseScale = 0.12f;

float EaseOutCubic(float t) { const float u = 1.0f - t; return 1.0f - u * u * u; }
float EaseOutBack(float t)
{
	const float c1 = 1.70158f;
	const float c3 = c1 + 1.0f;
	const float u = t - 1.0f;
	return 1.0f + c3 * u * u * u + c1 * u * u;
}

std::mt19937& Rng()
{
	static std::mt19937 rng{ std::random_device{}() };
	return rng;
}
float Random(float min, float max) { return std::uniform_real_distribution<float>(min, max)(Rng()); }

TextLabel* CreateLabel(const std::string& name, const std::string& text, float size, TextAlignX alignX, const char* font = "")
{
	TextLabel* label = SpriteManager::GetInstance().CreateTextLabel(SpriteLayer::UI, name, font);
	label->SetText(text);
	label->SetFontSize(size);
	label->SetAlign(alignX, TextAlignY::Middle);
	label->SetColor({ 1.0f, 1.0f, 1.0f, 0.0f });
	return label;
}

Sprite* CreateAddSprite(const std::string& name, const std::string& texture)
{
	Sprite* sprite = SpriteManager::GetInstance().CreateSprite(SpriteLayer::UI, name, texture);
	sprite->SetAnchorPoint({ 0.5f, 0.5f });
	sprite->SetColor({ 1.0f, 1.0f, 1.0f, 0.0f });
	sprite->GetRenderState().blendMode = BlendMode::kAdd;
	return sprite;
}

Vector4 WithAlpha(const Vector4& color, float alpha) { return { color.x, color.y, color.z, color.w * alpha }; }

} // namespace

void ClearUI::LoadResources()
{
	TextureManager::GetInstance().LoadTexture(kGlowTexture);
	TextureManager::GetInstance().LoadTexture(kRingTexture);
	TextureManager::GetInstance().LoadTexture(kLineTexture);
	// 2回目以降は読み込み済みのものが返る
	FontManager::GetInstance().LoadFont(kDisplayFont, kDisplayFontPath, kDisplayFontBakeSize, kDisplayFontAtlasSize);
}

void ClearUI::Initialize()
{
	// 光は文字の後ろに置くので先に作る（同じレイヤーの描画順は生成順）
	stageClearGlow_ = CreateAddSprite("clearStageGlow", kGlowTexture);
	rankGlow_ = CreateAddSprite("clearRankGlow", kGlowTexture);
	for (size_t i = 0; i < rings_.size(); ++i) {
		rings_[i] = CreateAddSprite("clearRankRing" + std::to_string(i), kRingTexture);
	}

	stageClear_ = CreateLabel("clearStage", "STAGE CLEAR", kStageClearSize, TextAlignX::Center, kDisplayFont);
	stageClear_->Update();
	stageClearWidth_ = stageClear_->GetSize().x;

	stageClearLine_ = SpriteManager::GetInstance().CreateSprite(SpriteLayer::UI, "clearStageLine", kLineTexture);
	stageClearLine_->SetAnchorPoint({ 0.0f, 0.5f });
	stageClearLine_->SetColor({ 1.0f, 1.0f, 1.0f, 0.0f });

	scoreCaption_ = CreateLabel("clearScoreCaption", "TOTAL SCORE", kCaptionSize, TextAlignX::Left);
	scoreValue_ = CreateLabel("clearScoreValue", "0", kScoreValueSize, TextAlignX::Left);
	rankCaption_ = CreateLabel("clearRankCaption", "RANK", kCaptionSize, TextAlignX::Left);
	rankLetter_ = CreateLabel("clearRankLetter", "D", kRankSize, TextAlignX::Center, kDisplayFont);

	// 光の粒は文字の上に重ねる
	for (size_t i = 0; i < sparks_.size(); ++i) {
		sparks_[i].sprite = CreateAddSprite("clearSpark" + std::to_string(i), kGlowTexture);
	}

	const std::array<const char*, 2> menuTexts = { "RETRY", "TITLE" };
	for (size_t i = 0; i < menuItems_.size(); ++i) {
		menuItems_[i] = CreateLabel("clearMenu" + std::to_string(i), menuTexts[i], kMenuSize, TextAlignX::Center);
		menuItems_[i]->SetPosition({ kMenuX[i], kMenuY });
	}
	menuCursor_ = SpriteManager::GetInstance().CreateSprite(SpriteLayer::UI, "clearMenuCursor", kLineTexture);
	menuCursor_->SetAnchorPoint({ 0.5f, 0.5f });
	menuCursor_->SetColor({ 1.0f, 1.0f, 1.0f, 0.0f });
	cursorX_ = kMenuX[0];
}

void ClearUI::SetResult(int32_t score, const std::string& rank)
{
	targetScore_ = (std::max)(score, 0);
	shownScore_ = -1;
	scoreFinished_ = false;
	rank_ = rank.empty() ? "D" : rank;
	style_ = StyleOf(rank_);

	// 大きさを最後の大きさにして幅を測り、左端をそろえた位置を中心にする（拡大・縮小は中心から）
	rankLetter_->SetText(rank_);
	rankLetter_->SetFontSize(kRankSize);
	rankLetter_->Update();
	rankCenter_ = { kLeft + rankLetter_->GetSize().x * 0.5f, kRankY };
	ResetEffects();
}

void ClearUI::ResetEffects()
{
	rankEffectTime_ = -1.0f;
	for (Spark& spark : sparks_) {
		spark.life = 0.0f;
		spark.maxLife = 0.0f;
	}
}

ClearUI::RankStyle ClearUI::StyleOf(const std::string& rank)
{
	// S 以上は金、A は青白、B は緑、C 以下は灰（StylishScoreManager の D〜SSS に合わせる）
	if (rank == "SSS") return { { 1.0f, 0.80f, 0.32f, 1.0f }, 1.0f, 40, true, true };
	if (rank == "SS")  return { { 1.0f, 0.81f, 0.34f, 1.0f }, 0.95f, 34, true, true };
	if (rank == "S")   return { { 1.0f, 0.82f, 0.36f, 1.0f }, 0.9f, 28, true, true };
	if (rank == "A")   return { { 0.72f, 0.86f, 1.0f, 1.0f }, 0.6f, 10, true, false };
	if (rank == "B")   return { { 0.56f, 0.90f, 0.62f, 1.0f }, 0.35f, 0, false, false };
	if (rank == "C")   return { { 0.72f, 0.72f, 0.75f, 1.0f }, 0.15f, 0, false, false };
	return { { 0.56f, 0.57f, 0.60f, 1.0f }, 0.08f, 0, false, false };
}

void ClearUI::SetLabelAlpha(TextLabel* label, const Vector4& color, float alpha)
{
	label->SetColor(WithAlpha(color, alpha));
	// 暗い洞窟の上でも、明るい光の筋に重なっても読めるよう、影を濃いめに付ける
	label->SetShadow(alpha > 0.0f, kShadowOffset, { 0.0f, 0.0f, 0.0f, 0.75f * alpha });
	label->Update();
}

std::string ClearUI::FormatScore(int32_t score)
{
	std::string digits = std::to_string(score);
	std::string result;
	const int length = static_cast<int>(digits.size());
	for (int i = 0; i < length; ++i) {
		result += digits[i];
		const int remain = length - 1 - i;
		if (remain > 0 && remain % 3 == 0) {
			result += ',';
		}
	}
	return result;
}

void ClearUI::Update(const ClearSequence& sequence, float deltaTime)
{
	time_ += deltaTime;
	UpdateStageClear(sequence);
	UpdateScore(sequence, deltaTime);
	UpdateRank(sequence, deltaTime);
	UpdateMenuVisual(sequence);
}

void ClearUI::UpdateStageClear(const ClearSequence& sequence)
{
	const ClearSequence::Params& p = sequence.GetParams();
	const float t = sequence.Progress(p.stageClearStart, p.stageClearDuration);
	if (sequence.Crossed(p.stageClearStart)) {
		SoundManager::GetInstance().PlaySE(GameSound::kTutorialClear, 0.8f);
	}

	// 透明から現れつつ、わずかに大きい所から通常の大きさへ縮む
	const float alpha = EaseOutCubic(t);
	const float scale = 1.0f + 0.12f * (1.0f - EaseOutCubic(t));
	const Vector2 center = { kLeft + stageClearWidth_ * 0.5f, kStageClearY };
	stageClear_->SetPosition(center);
	stageClear_->SetFontSize(kStageClearSize * scale);
	SetLabelAlpha(stageClear_, kTextColor, alpha);

	// 文字の後ろの弱い光。出た瞬間だけ少し強い
	const float flash = t > 0.0f ? std::exp(-(sequence.GetTime() - p.stageClearStart) * 2.5f) : 0.0f;
	stageClearGlow_->SetPosition(center);
	stageClearGlow_->SetSize({ stageClearWidth_ * 1.6f, 170.0f });
	stageClearGlow_->SetColor(WithAlpha(kWarmColor, alpha * (0.10f + 0.20f * flash)));
	stageClearGlow_->Update();

	// 下線は左から右へ伸ばす
	const float line = EaseOutCubic(sequence.Progress(p.stageClearStart + 0.15f, 0.5f));
	stageClearLine_->SetPosition({ kLeft, kLineY });
	stageClearLine_->SetSize({ stageClearWidth_ * line, 2.0f });
	stageClearLine_->SetColor(WithAlpha(kWarmColor, 0.7f * alpha));
	stageClearLine_->Update();
}

void ClearUI::UpdateScore(const ClearSequence& sequence, float deltaTime)
{
	const ClearSequence::Params& p = sequence.GetParams();
	const float captionAlpha = EaseOutCubic(sequence.Progress(p.scoreStart - 0.2f, 0.3f));
	scoreCaption_->SetPosition({ kLeft + 2.0f, kScoreCaptionY });
	SetLabelAlpha(scoreCaption_, kCaptionColor, captionAlpha);

	// 0 から最終値まで。終わり際はゆっくりにして、確定する瞬間が分かるようにする
	const float t = sequence.Progress(p.scoreStart, p.scoreDuration);
	const int32_t score = static_cast<int32_t>(std::lround(static_cast<double>(targetScore_) * EaseOutCubic(t)));
	tickTimer_ += deltaTime;
	if (score != shownScore_) {
		if (t > 0.0f && t < 1.0f && tickTimer_ >= kTickInterval) {
			SoundManager::GetInstance().PlaySE(GameSound::kScoreCount, 0.35f);
			tickTimer_ = 0.0f;
		}
		shownScore_ = score;
		scoreValue_->SetText(FormatScore(score));
	}

	const float finishAt = p.scoreStart + p.scoreDuration;
	if (sequence.Crossed(finishAt)) {
		SoundManager::GetInstance().PlaySE(GameSound::kUIConfirm, 0.6f);
	}
	// 確定した瞬間に小さく膨らませて戻す（飛ばしたときはしない）
	float pulse = 0.0f;
	const float sinceFinish = sequence.GetTime() - finishAt;
	if (!sequence.WasSkipped() && sinceFinish >= 0.0f && sinceFinish < kScorePulseTime) {
		pulse = std::sin(3.14159265f * sinceFinish / kScorePulseTime) * kScorePulseScale;
	}
	scoreValue_->SetFontSize(kScoreValueSize * (1.0f + pulse));
	scoreValue_->SetPosition({ kLeft, kScoreValueY });
	SetLabelAlpha(scoreValue_, kTextColor, EaseOutCubic(sequence.Progress(p.scoreStart - 0.1f, 0.2f)));
}

void ClearUI::UpdateRank(const ClearSequence& sequence, float deltaTime)
{
	const ClearSequence::Params& p = sequence.GetParams();
	const float captionAlpha = EaseOutCubic(sequence.Progress(p.rankStart - 0.2f, 0.3f));
	rankCaption_->SetPosition({ kLeft + 2.0f, kRankCaptionY });
	SetLabelAlpha(rankCaption_, kCaptionColor, captionAlpha);

	if (sequence.Crossed(p.rankStart)) {
		SoundManager::GetInstance().PlaySE(GameSound::kRankReveal, 0.8f);
	}

	// 大きい所から現れて、短い時間で通常の大きさへ縮む
	const float t = sequence.Progress(p.rankStart, p.rankDuration);
	const float alpha = (std::min)(t * 2.5f, 1.0f);
	const float scale = 1.0f + 1.2f * (1.0f - EaseOutBack(t));
	rankLetter_->SetPosition(rankCenter_);
	rankLetter_->SetFontSize(kRankSize * (std::max)(scale, 0.5f));
	SetLabelAlpha(rankLetter_, style_.color, alpha);

	// ランクに応じた演出の開始（SE・光の粒・衝撃波）
	if (sequence.Crossed(p.rankEffectStart)) {
		rankEffectTime_ = 0.0f;
		SpawnSparks();
		if (style_.high) {
			SoundManager::GetInstance().PlaySE(GameSound::kRankUp, 0.8f);
		}
	} else if (rankEffectTime_ >= 0.0f) {
		rankEffectTime_ += deltaTime;
	}

	// 後ろの光。演出の始まりで強く光ってから落ち着く。高ランクはゆっくり脈打つ
	const float flash = rankEffectTime_ >= 0.0f ? std::exp(-rankEffectTime_ * 3.0f) : 0.0f;
	const float breathe = style_.high ? 0.08f * std::sin(time_ * 2.4f) : 0.0f;
	const float glowAlpha = alpha * style_.glow * (0.32f + 0.45f * flash + breathe);
	const float glowSize = 240.0f * (0.75f + 0.45f * style_.glow) * (1.0f + 0.25f * flash);
	rankGlow_->SetPosition(rankCenter_);
	rankGlow_->SetSize({ glowSize * 1.25f, glowSize });
	rankGlow_->SetColor(WithAlpha(style_.color, glowAlpha));
	rankGlow_->Update();

	// 衝撃波のリング。2枚を少しずらして広げる
	for (size_t i = 0; i < rings_.size(); ++i) {
		const float local = style_.shockwave && rankEffectTime_ >= 0.0f
			? (rankEffectTime_ - 0.12f * static_cast<float>(i)) / 0.6f : -1.0f;
		const bool visible = local >= 0.0f && local < 1.0f;
		const float ease = EaseOutCubic(std::clamp(local, 0.0f, 1.0f));
		const float size = 120.0f + 420.0f * ease;
		const float fade = (1.0f - std::clamp(local, 0.0f, 1.0f));
		rings_[i]->SetPosition(rankCenter_);
		rings_[i]->SetSize({ size, size });
		rings_[i]->SetColor(WithAlpha(style_.color, visible ? 0.75f * fade * fade : 0.0f));
		rings_[i]->Update();
	}

	// 光の粒。外へ散ってから減速し、ゆっくり上へ漂って消える
	for (Spark& spark : sparks_) {
		if (spark.life < spark.maxLife) {
			spark.life += deltaTime;
			const float damping = std::exp(-3.0f * deltaTime);
			spark.velocity = { spark.velocity.x * damping, spark.velocity.y * damping - 40.0f * deltaTime };
			spark.position = { spark.position.x + spark.velocity.x * deltaTime, spark.position.y + spark.velocity.y * deltaTime };
		}
		const float life = spark.maxLife > 0.0f ? spark.life / spark.maxLife : 1.0f;
		const float sparkAlpha = life < 1.0f ? (1.0f - life) * (std::min)(spark.life * 10.0f, 1.0f) : 0.0f;
		spark.sprite->SetPosition(spark.position);
		spark.sprite->SetSize({ spark.size, spark.size });
		spark.sprite->SetColor(WithAlpha(style_.color, sparkAlpha));
		spark.sprite->Update();
	}
}

void ClearUI::SpawnSparks()
{
	const int count = (std::min)(style_.sparkCount, static_cast<int>(sparks_.size()));
	for (int i = 0; i < count; ++i) {
		Spark& spark = sparks_[i];
		const float angle = Random(0.0f, 6.2831853f);
		const float speed = Random(160.0f, 420.0f);
		const float radius = Random(10.0f, 50.0f);
		spark.position = { rankCenter_.x + std::cos(angle) * radius, rankCenter_.y + std::sin(angle) * radius };
		spark.velocity = { std::cos(angle) * speed, std::sin(angle) * speed };
		spark.life = 0.0f;
		spark.maxLife = Random(0.6f, 1.3f);
		spark.size = Random(8.0f, 20.0f);
	}
}

void ClearUI::UpdateMenuVisual(const ClearSequence& sequence)
{
	const ClearSequence::Params& p = sequence.GetParams();
	menuAppear_ = EaseOutCubic(sequence.Progress(p.menuStart, p.menuDuration));
	const float rise = (1.0f - menuAppear_) * 20.0f;

	for (size_t i = 0; i < menuItems_.size(); ++i) {
		const bool isSelected = static_cast<int>(i) == selected_;
		menuItems_[i]->SetPosition({ kMenuX[i], kMenuY + rise });
		SetLabelAlpha(menuItems_[i], isSelected ? kTextColor : kMenuIdleColor, menuAppear_);
	}

	// 選んでいる項目の下線。項目の間を滑らかに移る
	cursorX_ += (kMenuX[selected_] - cursorX_) * 0.3f;
	const float width = menuItems_[selected_]->GetSize().x + 24.0f;
	menuCursor_->SetPosition({ cursorX_, kMenuY + 26.0f + rise });
	menuCursor_->SetSize({ width, 3.0f });
	menuCursor_->SetColor(WithAlpha(kWarmColor, 0.85f * menuAppear_));
	menuCursor_->Update();
}

ClearUI::MenuResult ClearUI::UpdateMenu(const MenuNavigator& navigator)
{
	// 2つしか無いので、左右でも上下でも行き来できるようにする
	if (navigator.IsLeft() || navigator.IsRight() || navigator.IsUp() || navigator.IsDown()) {
		selected_ = 1 - selected_;
		SoundManager::GetInstance().PlaySE(GameSound::kUICursor, 0.6f);
	}
	if (navigator.IsDecide()) {
		SoundManager::GetInstance().PlaySE(GameSound::kUIConfirm, 0.8f);
		return selected_ == 0 ? MenuResult::Retry : MenuResult::Title;
	}
	return MenuResult::None;
}
