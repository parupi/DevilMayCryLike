#include "StyleHUD.h"
#include "Graphics/Rendering/Sprite/SpriteManager.h"
#include "Graphics/Rendering/Sprite/Sprite.h"
#include <Utility/DeltaTime.h>
#include <algorithm>
#include <string>

void StyleHUD::Initialize()
{
	// ランク画像（クリア画面と同じ Ranks.png。横5フレームのUVシート）
	rank_ = SpriteManager::GetInstance().CreateSprite(SpriteLayer::UI, "styleRank", "Ranks.png");
	rank_->SetUVSize({ 0.2f, 1.0f });
	rank_->SetAnchorPoint({ 0.5f, 0.5f });
	rank_->SetSize(rankSize_);
	rank_->SetPosition(rankPos_);

	// スタイルポイントの数字（ランク画像の下に中央揃え）。
	// ランク画像と同じ「黒い文字に白い縁」に寄せ、縁の代わりに白い影をずらして重ねる
	points_ = SpriteManager::GetInstance().CreateTextLabel(SpriteLayer::UI, "stylePoint");
	points_->SetFontSize(pointFontSize_);
	points_->SetAlign(TextAlignX::Center, TextAlignY::Middle);
	points_->SetPosition({ rankPos_.x, pointBaseY_ });
	points_->SetColor({ 0.05f, 0.05f, 0.05f, 1.0f });
	points_->SetShadow(true, { 2.0f, 2.0f }, { 1.0f, 1.0f, 1.0f, 0.9f });

	// 戦闘に入るまでは非表示にしておく
	Hide();
}

void StyleHUD::Update(const std::string& rankCode, int32_t stylePoint)
{
	const float dt = DeltaTime::GetDeltaTime();

	// ===== ランク画像 =====
	// ランクが変わった瞬間に拡大演出を開始する
	if (rankCode != lastRankCode_) {
		rankPopTimer_ = kRankPopDuration;
		lastRankCode_ = rankCode;
	}
	if (rankPopTimer_ > 0.0f) {
		rankPopTimer_ = (std::max)(0.0f, rankPopTimer_ - dt);
	}
	// 演出中は 1.4倍 →等倍 へ縮小しながら戻る
	float popT = (kRankPopDuration > 0.0f) ? (rankPopTimer_ / kRankPopDuration) : 0.0f;
	float scale = 1.0f + 0.4f * popT;

	rank_->GetRenderState().isVisible = true;
	rank_->SetUVPosition({ RankToUvX(rankCode), 0.0f });
	rank_->SetSize({ rankSize_.x * scale, rankSize_.y * scale });
	rank_->SetPosition(rankPos_);
	rank_->Update();

	// ===== スタイルポイントの数字（ランク画像の下に中央揃えで表示） =====
	// 値が変わらないフレームは文字を組み直さない
	points_->GetRenderState().isVisible = true;
	points_->SetText(std::to_string((std::max)(0, stylePoint)));
	points_->Update();
}

void StyleHUD::Hide()
{
	// 戦闘中以外は全部を非表示にする（DrawUILayersは表示中だけ描く）
	if (rank_) {
		rank_->GetRenderState().isVisible = false;
	}
	if (points_) {
		points_->GetRenderState().isVisible = false;
	}
}

float StyleHUD::RankToUvX(const std::string& rankCode)
{
	// Ranks.png は D / C / B / A / S+ の5フレーム構成。
	// S・SS・SSS は同じ "S" フレームを共有する（画像側の制約）。
	if (rankCode == "C") return 0.2f;
	if (rankCode == "B") return 0.4f;
	if (rankCode == "A") return 0.6f;
	if (rankCode == "S" || rankCode == "SS" || rankCode == "SSS") return 0.8f;
	return 0.0f; // D
}
