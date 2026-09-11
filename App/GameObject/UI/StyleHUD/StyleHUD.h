#pragma once
#include <string>
#include <vector>
#include <cstdint>
#include <Math/Vector2.h>

class Sprite;
class TextLabel;

/// <summary>
/// ゲーム中に表示するスタイリッシュランクのHUD。
/// 画面右・中央付近にランク画像（クリア画面と同じ Ranks.png）と
/// スタイルポイントの数字（フォントで描く文字）を表示する。
/// ランクが変わった瞬間は画像を一瞬拡大させて達成感を出す。
///
/// スプライトと文字は SpriteManager::DrawUILayers() が自動描画するため、
/// 生成後は毎フレーム Update() で見た目を更新するだけでよい（ハートUIと同じ流儀）。
/// </summary>
class StyleHUD
{
public:
	StyleHUD() = default;
	~StyleHUD() = default;

	void Initialize();

	/// <summary>
	/// 現在のランクとスタイルポイントで表示を更新する。
	/// </summary>
	/// <param name="rankCode">ランクの短縮コード（"D"〜"SSS"）</param>
	/// <param name="stylePoint">現在のスタイルポイント</param>
	void Update(const std::string& rankCode, int32_t stylePoint);

	/// <summary>HUDを非表示にする（戦闘中以外で呼ぶ）。</summary>
	void Hide();

private:
	// ランクコードから Ranks.png の UV X座標を求める（クリア画面のRankUIと同じ対応）
	static float RankToUvX(const std::string& rankCode);

private:
	// ランクアップ時の拡大演出の長さ（秒）
	static constexpr float kRankPopDuration = 0.3f;

	Sprite* rank_ = nullptr;          // ランク画像
	TextLabel* points_ = nullptr;     // スタイルポイントの数字（以前は Numbers.png を桁ぶん並べていた）

	std::string lastRankCode_;        // ランク変化検出用
	float rankPopTimer_ = 0.0f;       // ランクアップ拡大演出の残り時間

	// ===== レイアウト（画面 1280x720、右側・中央高さ） =====
	Vector2 rankPos_ = { 1160.0f, 320.0f };  // ランク画像の中心
	Vector2 rankSize_ = { 150.0f, 150.0f };  // ランク画像の基本サイズ
	float pointBaseY_ = 425.0f;              // 数字の中心Y（横はランク画像の中心に揃える）
	float pointFontSize_ = 46.0f;            // 数字の高さ(px)
};
