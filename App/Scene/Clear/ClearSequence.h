#pragma once
#include "Math/Vector3.h"

/// <summary>
/// クリア画面の演出の進行（クリアシーン演出仕様書 4章のタイムライン）。
///
/// シーン開始（暗転中）からの経過秒だけを持ち、各演出は「何秒から何秒か」をここで聞いて動く。
/// 時刻は GlobalVariables のグループ ClearSequence（Resource/GlobalVariables/Clear/ClearSequence.json）にあり、
/// 毎フレーム読み直すので、ImGui で書き換えるとその場で効く。
/// </summary>
class ClearSequence
{
public:
	static constexpr const char* kGroup = "ClearSequence";
	static constexpr const char* kDirectory = "Clear";

	struct Params {
		// 1.0〜1.5 秒: プレイヤーを照らす光を少し強める
		float lightBoostStart = 1.0f;
		float lightBoostEnd = 1.5f;
		float lightBoostScale = 1.3f;   // 光の明るさ・光の筋の倍率（強めたあとはそのまま保つ）
		// 1.5〜2.0 秒: STAGE CLEAR
		float stageClearStart = 1.5f;
		float stageClearDuration = 0.5f;
		// 2.0〜3.0 秒: スコアのカウントアップ
		float scoreStart = 2.0f;
		float scoreDuration = 1.0f;
		// 3.0〜3.5 秒: ランク
		float rankStart = 3.0f;
		float rankDuration = 0.5f;
		// 3.5 秒: ランクに応じた SE・発光・粒子
		float rankEffectStart = 3.5f;
		// 4.0 秒〜: メニュー
		float menuStart = 4.0f;
		float menuDuration = 0.4f;

		// カメラ（固定。プレイヤーを斜め前の低い所から見上げ、画面の左を UI に空ける）
		Vector3 cameraPosition{ -1.9f, 0.65f, -5.6f };
		Vector3 cameraTarget{ -1.05f, 1.1f, 0.0f };
		float cameraZoom = 0.05f;       // メニューが出るまでにゆっくり寄る量（画角の割合）
		float rankZoomPunch = 0.025f;   // ランクが出た瞬間に一瞬寄る量（画角の割合）
	};

	/// <summary>調整値を GlobalVariables に登録してファイルから読み、時刻を 0 に戻す</summary>
	void Initialize();
	/// <summary>時刻を進める。調整値もここで読み直す</summary>
	void Update(float deltaTime);

	/// <summary>最初からやり直す（デバッグの再生ボタン用）</summary>
	void Restart();
	/// <summary>演出を飛ばしてメニューが出た所まで進める（決定入力のスキップ）。飛ばした分の SE は鳴らさない</summary>
	void SkipToMenu();

	float GetTime() const { return time_; }
	const Params& GetParams() const { return params_; }
	bool IsMenuReady() const { return time_ >= params_.menuStart; }
	bool WasSkipped() const { return skipped_; }

	/// <summary>このフレームで時刻 at をまたいだか（一度だけ鳴らす SE などに使う）</summary>
	bool Crossed(float at) const { return prevTime_ < at && time_ >= at; }
	/// <summary>start から duration 秒かけて 0→1。始まる前は 0、終わった後は 1</summary>
	float Progress(float start, float duration) const;

#ifdef _DEBUG
	/// <summary>調整値をファイルへ保存する</summary>
	void Save() const;
#endif

private:
	void ReadParams();

	Params params_{};
	float time_ = 0.0f;
	float prevTime_ = -1.0f;
	bool skipped_ = false;
};
