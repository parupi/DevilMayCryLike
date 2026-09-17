#include "GameSoundLibrary.h"

#include "Audio/SE/SoundPresets.h"
#include "GameSoundBuilders.h"

#include <string>

namespace {

	// 名前・説明・作る関数の対応表。
	// 名前は GameSoundLibrary.h の定数と必ず一致させること（ここがズレると
	// ゲームからは鳴らないのにエディタには出てくる、という分かりにくい状態になる）
	struct Entry {
		const char* name;
		const char* description;
		SoundDefinition(*create)();
	};

	const Entry kEntries[] = {
		// ── プレイヤー：移動 ──
		{ GameSound::kPlayerFootstep,    "走りの足音。石畳を踏む短い音",                     GameSoundDefs::PlayerFootstep },
		{ GameSound::kPlayerJump,        "ジャンプの踏み切り",                               GameSoundDefs::PlayerJump },
		{ GameSound::kPlayerLand,        "着地。低い衝撃＋砂利",                             GameSoundDefs::PlayerLand },

		// ── プレイヤー：被弾 ──
		{ GameSound::kPlayerDamage,      "軽い被弾",                                         GameSoundDefs::PlayerDamage },
		{ GameSound::kPlayerDamageHeavy, "吹き飛ばされた被弾",                               GameSoundDefs::PlayerDamageHeavy },
		{ GameSound::kPlayerLowHealth,   "HPが少ないときの心音（ループ）",                   GameSoundDefs::PlayerLowHealth },
		{ GameSound::kPlayerDeath,       "死亡。とどめの衝撃から落ちる余韻まで",             GameSoundDefs::PlayerDeath },

		// ── プレイヤー：攻撃の振り ──
		{ GameSound::kPlayerSheathe,     "納刀",                                             GameSoundDefs::PlayerSheathe },
		{ GameSound::kPlayerClap,        "クリアの拍手",                                     GameSoundDefs::PlayerClap },
		{ GameSound::kSwordSlashHeavy,   "強攻撃の重い風切り",                               GameSoundDefs::SwordSlashHeavy },
		{ GameSound::kSwordStinger,      "スティンガー（突進）",                             GameSoundDefs::SwordStinger },
		{ GameSound::kSwordLaunch,       "打ち上げ",                                         GameSoundDefs::SwordLaunch },
		{ GameSound::kSwordSlam,         "回転斬り・叩きつけ",                               GameSoundDefs::SwordSlam },
		{ GameSound::kSwordCharge,       "溜め中のループ",                                   GameSoundDefs::SwordCharge },
		{ GameSound::kSwordChargeReady,  "溜めきった合図",                                   GameSoundDefs::SwordChargeReady },
		{ GameSound::kSwordCounter,      "カウンター成立",                                   GameSoundDefs::SwordCounter },

		// ── プレイヤー：手応え ──
		{ GameSound::kHitFlesh,          "生身に当たった鈍い手応え",                         GameSoundDefs::HitFlesh },
		{ GameSound::kHitBone,           "骨が砕ける",                                       GameSoundDefs::HitBone },
		{ GameSound::kHitWood,           "木を叩いた中空の音",                               GameSoundDefs::HitWood },
		{ GameSound::kHitArmor,          "硬い鱗に弾かれた金属音",                           GameSoundDefs::HitArmor },
		{ GameSound::kHitHeavy,          "強攻撃が当たった重い衝撃",                         GameSoundDefs::HitHeavy },
		{ GameSound::kHitBlocked,        "スーパーアーマーに弾かれた",                       GameSoundDefs::HitBlocked },

		// ── ロックオン ──
		{ GameSound::kLockOnRelease,     "ロックオン解除",                                   GameSoundDefs::LockOnRelease },
		{ GameSound::kLockOnSwitch,      "ターゲット切り替え",                               GameSoundDefs::LockOnSwitch },

		// ── 雑魚敵（骸骨）──
		{ GameSound::kSkeletonSpawn,     "出現。黒い粒子が集まる演出に合わせた音",           GameSoundDefs::SkeletonSpawn },
		{ GameSound::kSkeletonFootstep,  "骸骨の足音",                                       GameSoundDefs::SkeletonFootstep },
		{ GameSound::kSkeletonCharge,    "攻撃の構え（チャージリングに合わせた溜め）",       GameSoundDefs::SkeletonCharge },
		{ GameSound::kSkeletonSwing,     "剣の振り",                                         GameSoundDefs::SkeletonSwing },
		{ GameSound::kSkeletonRush,      "突進攻撃のダッシュ",                               GameSoundDefs::SkeletonRush },
		{ GameSound::kSkeletonHit,       "被弾・のけぞりの骨の音",                           GameSoundDefs::SkeletonHit },
		{ GameSound::kSkeletonDeath,     "死亡。崩れ落ちる",                                 GameSoundDefs::SkeletonDeath },

		// ── ボス（ドラゴン）──
		{ GameSound::kDragonAppear,      "登場の地響き",                                     GameSoundDefs::DragonAppear },
		{ GameSound::kDragonRoar,        "咆哮",                                             GameSoundDefs::DragonRoar },
		{ GameSound::kDragonWingbeat,    "羽ばたき1回ぶん",                                  GameSoundDefs::DragonWingbeat },
		{ GameSound::kDragonLand,        "着地",                                             GameSoundDefs::DragonLand },
		{ GameSound::kDragonBite,        "噛みつき。顎を鳴らす",                             GameSoundDefs::DragonBite },
		{ GameSound::kDragonSlamCharge,  "叩きつけの溜めの地鳴り",                           GameSoundDefs::DragonSlamCharge },
		{ GameSound::kDragonSlamImpact,  "叩きつけの衝撃",                                   GameSoundDefs::DragonSlamImpact },
		{ GameSound::kDragonRushStep,    "突進の踏み込み",                                   GameSoundDefs::DragonRushStep },
		{ GameSound::kDragonRushLoop,    "突進中（ループ）",                                 GameSoundDefs::DragonRushLoop },
		{ GameSound::kDragonRushStop,    "突進が止まったときの衝撃",                         GameSoundDefs::DragonRushStop },
		{ GameSound::kDragonBreathCharge,"ブレスの溜め",                                     GameSoundDefs::DragonBreathCharge },
		{ GameSound::kDragonBreathIgnite,"ブレスの着火",                                     GameSoundDefs::DragonBreathIgnite },
		{ GameSound::kDragonBreathLoop,  "炎（ループ）",                                     GameSoundDefs::DragonBreathLoop },
		{ GameSound::kDragonBreathEnd,   "ブレスが消える余韻",                               GameSoundDefs::DragonBreathEnd },
		{ GameSound::kDragonPhaseRoar,   "フェーズ移行の咆哮",                               GameSoundDefs::DragonPhaseRoar },
		{ GameSound::kDragonBreak,       "ブレイク（崩れ）の倒れ込み",                       GameSoundDefs::DragonBreak },
		{ GameSound::kDragonHit,         "被弾",                                             GameSoundDefs::DragonHit },
		{ GameSound::kDragonArmorSpark,  "紫の火花で弾いた",                                 GameSoundDefs::DragonArmorSpark },
		{ GameSound::kDragonDeath,       "死亡",                                             GameSoundDefs::DragonDeath },

		// ── 戦闘まわりの仕組み ──
		{ GameSound::kAttackWarning,     "赤い予兆マーカーが出たときの警告",                 GameSoundDefs::AttackWarning },
		{ GameSound::kRankUp,            "スタイルランクアップ",                             GameSoundDefs::RankUp },
		{ GameSound::kRankDown,          "スタイルランクダウン",                             GameSoundDefs::RankDown },
		{ GameSound::kBossBarAppear,     "ボスHPバーの登場",                                 GameSoundDefs::BossBarAppear },
		{ GameSound::kBossBarPhase,      "ボスHPバーのフェーズ移行",                         GameSoundDefs::BossBarPhase },
		{ GameSound::kBossBarDeplete,    "ボスHPバーを削り切った",                           GameSoundDefs::BossBarDeplete },
		{ GameSound::kBattleWallOn,      "強制戦闘エリアの光の壁が出る",                     GameSoundDefs::BattleWallOn },
		{ GameSound::kBattleWallOff,     "光の壁が消える",                                   GameSoundDefs::BattleWallOff },
		{ GameSound::kEnemySpawnEvent,   "敵の出現イベント",                                 GameSoundDefs::EnemySpawnEvent },

		// ── UI・メニュー ──
		{ GameSound::kUICursor,          "カーソル移動",                                     GameSoundDefs::UICursor },
		{ GameSound::kUIPauseOpen,       "ポーズを開く",                                     GameSoundDefs::UIPauseOpen },
		{ GameSound::kUIPauseClose,      "ポーズを閉じる",                                   GameSoundDefs::UIPauseClose },
		{ GameSound::kUISlider,          "音量スライダーを動かす",                           GameSoundDefs::UISlider },
		{ GameSound::kUIDialogOpen,      "確認ダイアログが開く",                             GameSoundDefs::UIDialogOpen },
		{ GameSound::kUIDialogYes,       "確認ダイアログの「はい」",                         GameSoundDefs::UIDialogYes },
		{ GameSound::kUIDialogNo,        "確認ダイアログの「いいえ」",                       GameSoundDefs::UIDialogNo },
		{ GameSound::kTitlePress,        "タイトルで PRESS A BUTTON を押した",               GameSoundDefs::TitlePress },
		{ GameSound::kTutorialShow,      "チュートリアルが表示された",                       GameSoundDefs::TutorialShow },
		{ GameSound::kTutorialClear,     "チュートリアルの1項目をクリアした",               GameSoundDefs::TutorialClear },
		{ GameSound::kStageStart,        "ステージ開始の演出",                               GameSoundDefs::StageStart },
		{ GameSound::kGameOver,          "YOU DIED の表示",                                  GameSoundDefs::GameOver },
		{ GameSound::kScoreCount,        "スコアのカウントアップ",                           GameSoundDefs::ScoreCount },
		{ GameSound::kRankReveal,        "クリア画面のランク表示",                           GameSoundDefs::RankReveal },
		{ GameSound::kSceneTransition,   "シーン遷移の暗転",                                 GameSoundDefs::SceneTransition },

		// ── 環境音 ──
		{ GameSound::kAmbienceDungeon,   "ダンジョンの空気（ループ）",                       GameSoundDefs::AmbienceDungeon },
		{ GameSound::kTorchCrackle,      "松明のパチパチ（ループ）",                         GameSoundDefs::TorchCrackle },
	};

} // namespace

namespace GameSound {

	void Register()
	{
		for (const Entry& entry : kEntries) {
			SoundPresets::RegisterExternal(entry.name, entry.description, entry.create);
		}
	}

	int ExportMissing()
	{
		int written = 0;
		for (const Entry& entry : kEntries) {
			if (SoundFile::Exists(entry.name)) { continue; }
			if (SoundFile::Save(entry.create())) { ++written; }
		}
		return written;
	}

} // namespace GameSound
