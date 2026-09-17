#pragma once

/// <summary>
/// このゲームで使う SE の一覧（Sound.txt の要望をそのまま形にしたもの）。
///
/// 音そのものは `Engine/Audio/SE` の合成器で作っていて、実体は
/// `Resource/Sounds/&lt;名前&gt;.sound`。ここにあるのは
///   ・ゲーム側から呼ぶときの名前（打ち間違いを防ぐための定数）
///   ・その .sound をコードから作り直すための定義（`Register`）
/// の2つ。
///
/// **音を作り直したいときは `GameSoundLibrary.cpp` を直すか、
/// エディタ（Window &gt; Engine &gt; Sound Editor）で開いて保存する。**
/// エディタで保存した .sound のほうが優先されるので、コードを触らずに調整できる。
///
/// BGM とボイスはここには入らない（合成器で作れる種類の音ではないため）。
/// </summary>
namespace GameSound {

	// ───────────────────────────── プレイヤー ─────────────────────────────

	inline constexpr const char* kDodge = "Dodge";
	inline constexpr const char* kDash = "Dash";
	inline constexpr const char* kJustDodge = "JustDodge";

	inline constexpr const char* kPlayerFootstep = "PlayerFootstep";
	inline constexpr const char* kPlayerJump = "PlayerJump";
	inline constexpr const char* kPlayerLand = "PlayerLand";
	inline constexpr const char* kPlayerDamage = "PlayerDamage";
	inline constexpr const char* kPlayerDamageHeavy = "PlayerDamageHeavy";
	/// <summary>HP が少ないときの心音。ループ</summary>
	inline constexpr const char* kPlayerLowHealth = "PlayerLowHealth";
	inline constexpr const char* kPlayerDeath = "PlayerDeath";
	inline constexpr const char* kPlayerSheathe = "PlayerSheathe";
	/// <summary>クリアの拍手モーション</summary>
	inline constexpr const char* kPlayerClap = "PlayerClap";

	// ── 攻撃の振り ──
	/// <summary>通常斬り。既存の WAV 素材をそのまま使う</summary>
	inline constexpr const char* kSwordSlash = "SwordSlash";
	inline constexpr const char* kSwordSlashHeavy = "SwordSlashHeavy";
	/// <summary>スティンガー（突進）</summary>
	inline constexpr const char* kSwordStinger = "SwordStinger";
	/// <summary>打ち上げ</summary>
	inline constexpr const char* kSwordLaunch = "SwordLaunch";
	/// <summary>回転斬り・叩きつけ</summary>
	inline constexpr const char* kSwordSlam = "SwordSlam";
	/// <summary>溜め中のループ。止めるのは呼び出し側の責任</summary>
	inline constexpr const char* kSwordCharge = "SwordCharge";
	/// <summary>溜めきった合図</summary>
	inline constexpr const char* kSwordChargeReady = "SwordChargeReady";
	inline constexpr const char* kSwordCounter = "SwordCounter";

	// ── 攻撃が当たった手応え ──
	/// <summary>既定のヒット。既存の WAV 素材</summary>
	inline constexpr const char* kSwordHit = "SwordHit";
	inline constexpr const char* kHitFlesh = "HitFlesh";
	inline constexpr const char* kHitBone = "HitBone";
	inline constexpr const char* kHitWood = "HitWood";
	/// <summary>硬い鱗・鎧に当たった金属音</summary>
	inline constexpr const char* kHitArmor = "HitArmor";
	/// <summary>強攻撃の重い衝撃</summary>
	inline constexpr const char* kHitHeavy = "HitHeavy";
	/// <summary>スーパーアーマーに弾かれた（攻撃が通っていない）</summary>
	inline constexpr const char* kHitBlocked = "HitBlocked";

	// ── ロックオン ──
	inline constexpr const char* kLockOn = "LockOn";
	inline constexpr const char* kLockOnRelease = "LockOnRelease";
	inline constexpr const char* kLockOnSwitch = "LockOnSwitch";

	// ───────────────────────────── 雑魚敵（骸骨） ─────────────────────────────

	inline constexpr const char* kSkeletonSpawn = "SkeletonSpawn";
	inline constexpr const char* kSkeletonFootstep = "SkeletonFootstep";
	/// <summary>攻撃の構え（チャージリングに合わせた溜め）</summary>
	inline constexpr const char* kSkeletonCharge = "SkeletonCharge";
	inline constexpr const char* kSkeletonSwing = "SkeletonSwing";
	inline constexpr const char* kSkeletonRush = "SkeletonRush";
	inline constexpr const char* kSkeletonHit = "SkeletonHit";
	inline constexpr const char* kSkeletonDeath = "SkeletonDeath";

	// ───────────────────────────── ボス（ドラゴン） ─────────────────────────────

	/// <summary>登場の地響き</summary>
	inline constexpr const char* kDragonAppear = "DragonAppear";
	inline constexpr const char* kDragonRoar = "DragonRoar";
	inline constexpr const char* kDragonWingbeat = "DragonWingbeat";
	inline constexpr const char* kDragonLand = "DragonLand";
	/// <summary>噛みつき。顎を鳴らす</summary>
	inline constexpr const char* kDragonBite = "DragonBite";
	/// <summary>叩きつけの溜めの地鳴り</summary>
	inline constexpr const char* kDragonSlamCharge = "DragonSlamCharge";
	inline constexpr const char* kDragonSlamImpact = "DragonSlamImpact";
	inline constexpr const char* kDragonRushStep = "DragonRushStep";
	/// <summary>突進中。ループ</summary>
	inline constexpr const char* kDragonRushLoop = "DragonRushLoop";
	inline constexpr const char* kDragonRushStop = "DragonRushStop";
	inline constexpr const char* kDragonBreathCharge = "DragonBreathCharge";
	inline constexpr const char* kDragonBreathIgnite = "DragonBreathIgnite";
	/// <summary>炎。ループ</summary>
	inline constexpr const char* kDragonBreathLoop = "DragonBreathLoop";
	inline constexpr const char* kDragonBreathEnd = "DragonBreathEnd";
	inline constexpr const char* kDragonPhaseRoar = "DragonPhaseRoar";
	/// <summary>ブレイク（崩れ）の倒れ込み</summary>
	inline constexpr const char* kDragonBreak = "DragonBreak";
	inline constexpr const char* kDragonHit = "DragonHit";
	/// <summary>紫の火花で弾いた</summary>
	inline constexpr const char* kDragonArmorSpark = "DragonArmorSpark";
	inline constexpr const char* kDragonDeath = "DragonDeath";

	// ───────────────────────────── 戦闘まわりの仕組み ─────────────────────────────

	/// <summary>赤い予兆マーカーが出たときの警告</summary>
	inline constexpr const char* kAttackWarning = "AttackWarning";
	inline constexpr const char* kRankUp = "RankUp";
	inline constexpr const char* kRankDown = "RankDown";
	inline constexpr const char* kBossBarAppear = "BossBarAppear";
	inline constexpr const char* kBossBarPhase = "BossBarPhase";
	inline constexpr const char* kBossBarDeplete = "BossBarDeplete";
	inline constexpr const char* kBattleWallOn = "BattleWallOn";
	inline constexpr const char* kBattleWallOff = "BattleWallOff";
	inline constexpr const char* kEnemySpawnEvent = "EnemySpawnEvent";

	// ───────────────────────────── UI・メニュー ─────────────────────────────

	inline constexpr const char* kUICursor = "UICursor";
	inline constexpr const char* kUIConfirm = "UIConfirm";
	inline constexpr const char* kUICancel = "UICancel";
	inline constexpr const char* kUIPauseOpen = "UIPauseOpen";
	inline constexpr const char* kUIPauseClose = "UIPauseClose";
	/// <summary>音量スライダーを動かしたとき</summary>
	inline constexpr const char* kUISlider = "UISlider";
	inline constexpr const char* kUIDialogOpen = "UIDialogOpen";
	inline constexpr const char* kUIDialogYes = "UIDialogYes";
	inline constexpr const char* kUIDialogNo = "UIDialogNo";
	inline constexpr const char* kTitlePress = "TitlePress";
	inline constexpr const char* kTutorialShow = "TutorialShow";
	inline constexpr const char* kTutorialClear = "TutorialClear";
	inline constexpr const char* kStageStart = "StageStart";
	inline constexpr const char* kGameOver = "GameOver";
	/// <summary>スコアのカウントアップ。1桁進むごとに短く鳴らす</summary>
	inline constexpr const char* kScoreCount = "ScoreCount";
	inline constexpr const char* kRankReveal = "RankReveal";
	/// <summary>暗転</summary>
	inline constexpr const char* kSceneTransition = "SceneTransition";

	// ───────────────────────────── 環境音 ─────────────────────────────

	/// <summary>ダンジョンの空気。ループ</summary>
	inline constexpr const char* kAmbienceDungeon = "AmbienceDungeon";
	/// <summary>松明のパチパチ。ループ</summary>
	inline constexpr const char* kTorchCrackle = "TorchCrackle";

	// ───────────────────────────── 登録 ─────────────────────────────

	/// <summary>
	/// 上の SE を `SoundPresets` へ登録する。
	/// エディタの「プリセットから作る」に並び、一括書き出しの対象にもなる。
	/// `MyGameTitle::Initialize` から1回だけ呼ぶ
	/// </summary>
	void Register();

	/// <summary>
	/// まだ `Resource/Sounds` に無いものを書き出す。
	/// リポジトリには生成済みの .sound を置いてあるので通常は何もしないが、
	/// 消してしまった場合にここで作り直せる
	/// </summary>
	/// <returns>書き出した数</returns>
	int ExportMissing();

} // namespace GameSound
