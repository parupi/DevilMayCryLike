#pragma once
#include "ActionGuideTypes.h"

#include <array>
#include <string>

/// <summary>
/// アクションガイドの調整値。GlobalVariables の "ActionGuide" グループ
/// （Resource/GlobalVariables/UI/ActionGuide.json）に置き、エディタの Action Guide ウィンドウから触る。
///
/// 毎フレーム Apply() で読み直すので、エディタで動かした値はその場で画面に効く。
/// AddItem の既定値と json の値は一致させておくこと（他の調整値と同じ約束）
/// </summary>
struct ActionGuideSettings {
	static constexpr const char* kDirectoryName = "UI";
	static constexpr const char* kGroupName = "ActionGuide";

	// ── 中身 ──
	int32_t maxEntries = 5;            ///< 最大表示件数（1〜kMaxRows）
	bool showPreview = true;           ///< Preview（少し待つと出る派生）を出すか
	bool showLockOn = true;            ///< ロックオンを基本操作として出すか
	bool showJump = true;              ///< ジャンプを基本操作として出すか
	bool showDodge = true;             ///< 回避を出すか
	/// <summary>カテゴリごとの優先度。大きいほど上に出る</summary>
	std::array<int32_t, kActionGuideCategoryCount> priorities{};

	// ── 表示名（攻撃以外。攻撃の名前は攻撃エディタの Guide Name） ──
	std::string nameJump = "ジャンプ";
	std::string nameDodge = "回避";
	std::string nameLockOn = "ロックオン";
	// 押し方の言葉
	std::string textHold = "長押し";
	std::string textRelease = "離す";
	std::string textWait = "少し待って";

	// ── 配置（1280x720 の画面座標） ──
	float iconX = 1060.0f;           ///< アイコンの中心の x。名前はこの右、組み合わせはこの左に出る
	float bottomY = 680.0f;          ///< 一番下の行の中心の y。行は上へ積む
	float tutorialRoomRaise = 70.0f; ///< チュートリアルの部屋ではスキップゲージの分だけ持ち上げる
	// 5行ぶん積んでも、スタイルランクの数字（y≈425）に届かない間隔
	float rowSpacing = 34.0f;
	float iconSize = 28.0f;
	float fontSize = 19.0f;
	float gestureFontSize = 15.0f;
	float textGap = 10.0f;           ///< アイコンと文字の間
	float backdropAlpha = 0.55f;     ///< 行の後ろの帯の濃さ（明るい床の上でも文字とキーが読める濃さ）
	float backdropWidth = 300.0f;

	// ── 見え方 ──
	float normalAlpha = 1.0f;
	float previewAlpha = 0.4f;
	float fadeTime = 0.12f;          ///< 行が入れ替わったときのフェード[秒]
	float fadeStartAlpha = 0.35f;    ///< 入れ替わった行のフェードの始まりの濃さ（0 からだと短い受付窓で情報が隠れる）
	float slideOffset = 10.0f;       ///< 入れ替わった行の横すべりの量[px]
	float queuedMarkSize = 8.0f;     ///< 先行入力済みの印（アイコン左の点）の大きさ

	/// <summary>保存済みの値を読み、無い項目は既定値で登録する。起動後1回</summary>
	void RegisterAndLoad();
	/// <summary>GlobalVariables の今の値を読み直す。毎フレーム呼んでよい</summary>
	void Apply();
	/// <summary>ファイルへ書き出す（エディタの Save ボタン）</summary>
	static void Save();

	/// <summary>優先度の項目名（json のキー）</summary>
	static const char* PriorityKey(ActionGuideCategory category);
	/// <summary>カテゴリのエディタ表示名</summary>
	static const char* CategoryLabel(ActionGuideCategory category);
	/// <summary>カテゴリごとの優先度の既定値</summary>
	static int32_t DefaultPriority(ActionGuideCategory category);

	static constexpr int32_t kMaxRows = 8;
};
