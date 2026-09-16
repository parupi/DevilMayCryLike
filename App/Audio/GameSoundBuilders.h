#pragma once
#include "Audio/SE/SoundDefinition.h"

/// <summary>
/// SE を組み立てるときの下ごしらえ（GameSoundLibrary の内部用）。
///
/// `SELayer` は既定値がしっかりあるので、差分だけ書けるようにしておくと
/// 1つの音が10行前後で済み、並べたときに違いが読み取れる。
/// </summary>
namespace GameSoundBuild {

	using Wave = SoundDSP::WaveType;
	using Filter = SoundDSP::FilterType;
	using Sweep = SoundDSP::SweepCurve;

	inline SELayer Layer(const char* name, Wave wave, float duration, float volume)
	{
		SELayer layer;
		layer.name = name;
		layer.wave = wave;
		layer.duration = duration;
		layer.volume = volume;
		return layer;
	}

	/// <summary>周波数を startHz から endHz へ動かす。time が 0 ならレイヤー全体を使う</summary>
	inline void Pitch(SELayer& layer, float startHz, float endHz,
		Sweep curve = Sweep::Exponential, float time = 0.0f)
	{
		layer.pitchSweepEnabled = true;
		layer.pitchStart = startHz;
		layer.pitchEnd = endHz;
		layer.pitchCurve = curve;
		layer.pitchSweepTime = time;
	}

	inline void Env(SELayer& layer, float attack, float hold, float decay,
		float sustain, float release, float curve = 2.0f)
	{
		layer.envelope.attack = attack;
		layer.envelope.hold = hold;
		layer.envelope.decay = decay;
		layer.envelope.sustain = sustain;
		layer.envelope.release = release;
		layer.envelope.curve = curve;
	}

	/// <summary>cutoffEnd に正の値を渡すとカットオフも掃引する</summary>
	inline void Filt(SELayer& layer, Filter type, float cutoff,
		float resonance = 0.707f, float cutoffEnd = -1.0f)
	{
		layer.filterType = type;
		layer.filterCutoff = cutoff;
		layer.filterResonance = resonance;
		if (cutoffEnd > 0.0f) {
			layer.filterSweepEnabled = true;
			layer.filterCutoffEnd = cutoffEnd;
		}
	}

	/// <summary>整数倍から外した比率ほど金属的な倍音になる（2.76 や 4.3 など）</summary>
	inline void FM(SELayer& layer, float ratio, float amount)
	{
		layer.fmRatio = ratio;
		layer.fmAmount = amount;
	}

	inline SoundDefinition Sound(const char* name, int priority)
	{
		SoundDefinition sound;
		sound.name = name;
		sound.priority = priority;
		return sound;
	}

	inline void Distortion(SoundDefinition& sound, float drive, float mix)
	{
		sound.distortion.enabled = true;
		sound.distortion.drive = drive;
		sound.distortion.mix = mix;
	}

	inline void Delay(SoundDefinition& sound, float time, float feedback, float mix)
	{
		sound.delay.enabled = true;
		sound.delay.time = time;
		sound.delay.feedback = feedback;
		sound.delay.mix = mix;
	}

	inline void Reverb(SoundDefinition& sound, float roomSize, float damping, float mix)
	{
		sound.reverb.enabled = true;
		sound.reverb.roomSize = roomSize;
		sound.reverb.damping = damping;
		sound.reverb.mix = mix;
	}

} // namespace GameSoundBuild

// ── 各グループの定義（cpp が分かれているだけで、役割は同じ）──
namespace GameSoundDefs {

	// プレイヤー（GameSoundPlayer.cpp）
	SoundDefinition PlayerFootstep();
	SoundDefinition PlayerJump();
	SoundDefinition PlayerLand();
	SoundDefinition PlayerDamage();
	SoundDefinition PlayerDamageHeavy();
	SoundDefinition PlayerLowHealth();
	SoundDefinition PlayerDeath();
	SoundDefinition PlayerSheathe();
	SoundDefinition PlayerClap();

	SoundDefinition SwordSlashHeavy();
	SoundDefinition SwordStinger();
	SoundDefinition SwordLaunch();
	SoundDefinition SwordSlam();
	SoundDefinition SwordCharge();
	SoundDefinition SwordChargeReady();
	SoundDefinition SwordCounter();

	SoundDefinition HitFlesh();
	SoundDefinition HitBone();
	SoundDefinition HitWood();
	SoundDefinition HitArmor();
	SoundDefinition HitHeavy();
	SoundDefinition HitBlocked();

	SoundDefinition LockOnRelease();
	SoundDefinition LockOnSwitch();

	// 敵（GameSoundEnemy.cpp）
	SoundDefinition SkeletonSpawn();
	SoundDefinition SkeletonFootstep();
	SoundDefinition SkeletonCharge();
	SoundDefinition SkeletonSwing();
	SoundDefinition SkeletonRush();
	SoundDefinition SkeletonHit();
	SoundDefinition SkeletonDeath();

	SoundDefinition DragonAppear();
	SoundDefinition DragonRoar();
	SoundDefinition DragonWingbeat();
	SoundDefinition DragonLand();
	SoundDefinition DragonBite();
	SoundDefinition DragonSlamCharge();
	SoundDefinition DragonSlamImpact();
	SoundDefinition DragonRushStep();
	SoundDefinition DragonRushLoop();
	SoundDefinition DragonRushStop();
	SoundDefinition DragonBreathCharge();
	SoundDefinition DragonBreathIgnite();
	SoundDefinition DragonBreathLoop();
	SoundDefinition DragonBreathEnd();
	SoundDefinition DragonPhaseRoar();
	SoundDefinition DragonBreak();
	SoundDefinition DragonHit();
	SoundDefinition DragonArmorSpark();
	SoundDefinition DragonDeath();

	// 仕組み・UI・環境音（GameSoundSystem.cpp）
	SoundDefinition AttackWarning();
	SoundDefinition RankUp();
	SoundDefinition RankDown();
	SoundDefinition BossBarAppear();
	SoundDefinition BossBarPhase();
	SoundDefinition BossBarDeplete();
	SoundDefinition BattleWallOn();
	SoundDefinition BattleWallOff();
	SoundDefinition EnemySpawnEvent();

	SoundDefinition UICursor();
	SoundDefinition UIPauseOpen();
	SoundDefinition UIPauseClose();
	SoundDefinition UISlider();
	SoundDefinition UIDialogOpen();
	SoundDefinition UIDialogYes();
	SoundDefinition UIDialogNo();
	SoundDefinition TitlePress();
	SoundDefinition TutorialShow();
	SoundDefinition TutorialClear();
	SoundDefinition StageStart();
	SoundDefinition GameOver();
	SoundDefinition ScoreCount();
	SoundDefinition RankReveal();
	SoundDefinition SceneTransition();

	SoundDefinition AmbienceDungeon();
	SoundDefinition TorchCrackle();

} // namespace GameSoundDefs
