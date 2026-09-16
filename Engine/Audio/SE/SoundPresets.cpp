#include "SoundPresets.h"

#include <algorithm>
#include <filesystem>

using Wave = SoundDSP::WaveType;
using Filter = SoundDSP::FilterType;
using Sweep = SoundDSP::SweepCurve;

namespace {

	/// <summary>同じ名前の WAV 素材が Resource/sound にあるか（SoundManager が見るのと同じ場所）</summary>
	bool WavAssetExists(const std::string& name)
	{
		std::error_code ec;
		return std::filesystem::exists(std::filesystem::path("Resource/sound") / (name + ".wav"), ec);
	}

	// レイヤーを組み立てるときの下ごしらえ。
	// 既定値からの差分だけ書きたいので、共通の初期化をここでまとめる
	SELayer MakeLayer(const char* name, Wave wave, float duration, float volume)
	{
		SELayer layer;
		layer.name = name;
		layer.wave = wave;
		layer.duration = duration;
		layer.volume = volume;
		return layer;
	}

	void SetPitchSweep(SELayer& layer, float startHz, float endHz, Sweep curve = Sweep::Exponential, float time = 0.0f)
	{
		layer.pitchSweepEnabled = true;
		layer.pitchStart = startHz;
		layer.pitchEnd = endHz;
		layer.pitchCurve = curve;
		layer.pitchSweepTime = time;
	}

	void SetEnvelope(SELayer& layer, float attack, float hold, float decay, float sustain, float release, float curve = 2.0f)
	{
		layer.envelope.attack = attack;
		layer.envelope.hold = hold;
		layer.envelope.decay = decay;
		layer.envelope.sustain = sustain;
		layer.envelope.release = release;
		layer.envelope.curve = curve;
	}

	void SetFilter(SELayer& layer, Filter type, float cutoff, float resonance = 0.707f, float cutoffEnd = -1.0f)
	{
		layer.filterType = type;
		layer.filterCutoff = cutoff;
		layer.filterResonance = resonance;
		if (cutoffEnd > 0.0f) {
			layer.filterSweepEnabled = true;
			layer.filterCutoffEnd = cutoffEnd;
		}
	}

	// ───────────────────────────── 個々のプリセット ─────────────────────────────

	/// <summary>回避。Noise → Pitch Down → Volume Envelope → High Pass（設計書 §5-①）</summary>
	SoundDefinition MakeDodge()
	{
		SoundDefinition sound;
		sound.name = "Dodge";
		sound.priority = 70;

		SELayer air = MakeLayer("Air", Wave::WhiteNoise, 0.26f, 0.9f);
		SetPitchSweep(air, 6000.0f, 800.0f);
		SetEnvelope(air, 0.015f, 0.0f, 0.20f, 0.0f, 0.045f, 2.2f);
		SetFilter(air, Filter::HighPass, 900.0f, 0.9f, 300.0f);
		sound.layers.push_back(air);

		// 身体が動いた重みを一瞬だけ足す
		SELayer body = MakeLayer("Body", Wave::Sine, 0.12f, 0.25f);
		SetPitchSweep(body, 180.0f, 70.0f);
		SetEnvelope(body, 0.004f, 0.0f, 0.10f, 0.0f, 0.02f);
		sound.layers.push_back(body);

		return sound;
	}

	/// <summary>ダッシュ。回避より長く、上がってから抜ける風</summary>
	SoundDefinition MakeDash()
	{
		SoundDefinition sound;
		sound.name = "Dash";
		sound.priority = 60;

		SELayer wind = MakeLayer("Wind", Wave::WhiteNoise, 0.40f, 0.85f);
		SetPitchSweep(wind, 1600.0f, 6000.0f, Sweep::EaseOut);
		SetEnvelope(wind, 0.09f, 0.0f, 0.24f, 0.0f, 0.07f, 1.6f);
		SetFilter(wind, Filter::BandPass, 900.0f, 1.4f, 2600.0f);
		sound.layers.push_back(wind);

		SELayer thrust = MakeLayer("Thrust", Wave::Saw, 0.22f, 0.35f);
		SetPitchSweep(thrust, 260.0f, 80.0f);
		SetEnvelope(thrust, 0.01f, 0.0f, 0.18f, 0.0f, 0.03f);
		SetFilter(thrust, Filter::LowPass, 900.0f, 0.8f);
		sound.layers.push_back(thrust);

		return sound;
	}

	/// <summary>ジャスト回避の「キィン！」。Sine + Noise → 高音 → 短いEnvelope → Reverb（設計書 §5-②）</summary>
	SoundDefinition MakeJustDodge()
	{
		SoundDefinition sound;
		sound.name = "JustDodge";
		sound.priority = 90;

		// FM を掛けた正弦波が金属的な倍音を作る。これが「キィン」の芯
		SELayer ring = MakeLayer("Ring", Wave::Sine, 0.55f, 0.9f);
		SetPitchSweep(ring, 2600.0f, 2350.0f);
		ring.fmRatio = 2.76f;   // 整数倍から外すほど金属寄りになる
		ring.fmAmount = 0.55f;
		SetEnvelope(ring, 0.002f, 0.0f, 0.50f, 0.0f, 0.05f, 3.0f);
		sound.layers.push_back(ring);

		SELayer shimmer = MakeLayer("Shimmer", Wave::Sine, 0.38f, 0.30f);
		shimmer.frequency = 3700.0f;
		shimmer.fmRatio = 1.41f;
		shimmer.fmAmount = 0.35f;
		SetEnvelope(shimmer, 0.001f, 0.0f, 0.34f, 0.0f, 0.04f, 3.0f);
		sound.layers.push_back(shimmer);

		// 当たった瞬間のノイズ。短くないと「キィン」が濁る
		SELayer spark = MakeLayer("Spark", Wave::WhiteNoise, 0.07f, 0.35f);
		SetPitchSweep(spark, 9000.0f, 3000.0f);
		SetEnvelope(spark, 0.001f, 0.0f, 0.06f, 0.0f, 0.01f, 3.0f);
		SetFilter(spark, Filter::HighPass, 2000.0f, 0.8f);
		sound.layers.push_back(spark);

		sound.reverb.enabled = true;
		sound.reverb.roomSize = 0.55f;
		sound.reverb.damping = 0.35f;
		sound.reverb.mix = 0.35f;

		return sound;
	}

	/// <summary>剣の振り。抜けの良い風切り</summary>
	SoundDefinition MakeSwordSlash()
	{
		SoundDefinition sound;
		sound.name = "SwordSlash";
		sound.priority = 80;

		SELayer swing = MakeLayer("Swing", Wave::WhiteNoise, 0.24f, 0.95f);
		SetPitchSweep(swing, 7000.0f, 1300.0f);
		SetEnvelope(swing, 0.02f, 0.0f, 0.17f, 0.0f, 0.04f, 2.4f);
		SetFilter(swing, Filter::BandPass, 2600.0f, 2.0f, 900.0f);
		sound.layers.push_back(swing);

		SELayer blade = MakeLayer("Blade", Wave::Saw, 0.14f, 0.22f);
		SetPitchSweep(blade, 520.0f, 140.0f);
		SetEnvelope(blade, 0.008f, 0.0f, 0.11f, 0.0f, 0.02f);
		SetFilter(blade, Filter::LowPass, 1800.0f, 0.9f);
		sound.layers.push_back(blade);

		return sound;
	}

	/// <summary>斬撃が当たった音。金属の「カキン」＋重み</summary>
	SoundDefinition MakeSwordHit()
	{
		SoundDefinition sound;
		sound.name = "SwordHit";
		sound.priority = 80;

		SELayer clang = MakeLayer("Clang", Wave::Sine, 0.28f, 0.8f);
		clang.frequency = 1900.0f;
		clang.fmRatio = 3.3f;
		clang.fmAmount = 1.2f;
		SetEnvelope(clang, 0.001f, 0.0f, 0.25f, 0.0f, 0.03f, 3.0f);
		sound.layers.push_back(clang);

		SELayer burst = MakeLayer("Burst", Wave::WhiteNoise, 0.09f, 0.5f);
		SetPitchSweep(burst, 10000.0f, 2200.0f);
		SetEnvelope(burst, 0.001f, 0.0f, 0.08f, 0.0f, 0.01f, 3.0f);
		SetFilter(burst, Filter::HighPass, 1800.0f, 0.8f);
		sound.layers.push_back(burst);

		SELayer weight = MakeLayer("Weight", Wave::Sine, 0.16f, 0.55f);
		SetPitchSweep(weight, 240.0f, 90.0f);
		SetEnvelope(weight, 0.002f, 0.0f, 0.14f, 0.0f, 0.02f, 2.5f);
		sound.layers.push_back(weight);

		sound.distortion.enabled = true;
		sound.distortion.drive = 2.5f;
		sound.distortion.mix = 0.4f;

		return sound;
	}

	/// <summary>強攻撃の振り。重く長い風切り</summary>
	SoundDefinition MakeHeavyAttack()
	{
		SoundDefinition sound;
		sound.name = "HeavyAttack";
		sound.priority = 85;

		SELayer swing = MakeLayer("Swing", Wave::PinkNoise, 0.48f, 0.9f);
		SetPitchSweep(swing, 3200.0f, 600.0f);
		SetEnvelope(swing, 0.13f, 0.0f, 0.28f, 0.0f, 0.07f, 1.8f);
		SetFilter(swing, Filter::LowPass, 3200.0f, 1.1f, 450.0f);
		sound.layers.push_back(swing);

		SELayer mass = MakeLayer("Mass", Wave::Saw, 0.42f, 0.45f);
		SetPitchSweep(mass, 170.0f, 55.0f);
		SetEnvelope(mass, 0.10f, 0.0f, 0.26f, 0.0f, 0.06f);
		SetFilter(mass, Filter::LowPass, 700.0f, 0.9f);
		sound.layers.push_back(mass);

		sound.distortion.enabled = true;
		sound.distortion.drive = 2.0f;
		sound.distortion.mix = 0.3f;

		return sound;
	}

	/// <summary>軽い衝撃。Low Frequency → Pitch Down → 短いEnvelope（設計書 Phase 3 の Impact）</summary>
	SoundDefinition MakeImpact()
	{
		SoundDefinition sound;
		sound.name = "Impact";
		sound.priority = 55;

		SELayer body = MakeLayer("Body", Wave::Sine, 0.30f, 0.9f);
		SetPitchSweep(body, 320.0f, 70.0f);
		SetEnvelope(body, 0.002f, 0.0f, 0.26f, 0.0f, 0.03f, 2.5f);
		sound.layers.push_back(body);

		SELayer grit = MakeLayer("Grit", Wave::WhiteNoise, 0.14f, 0.45f);
		SetPitchSweep(grit, 5000.0f, 900.0f);
		SetEnvelope(grit, 0.001f, 0.0f, 0.12f, 0.0f, 0.02f, 2.8f);
		SetFilter(grit, Filter::LowPass, 3000.0f, 0.8f, 700.0f);
		sound.layers.push_back(grit);

		return sound;
	}

	/// <summary>強い衝撃の「ドゴン！」。Low Sine + Noise → Pitch Down → Distortion → Short Reverb（設計書 §5-③）</summary>
	SoundDefinition MakeHeavyImpact()
	{
		SoundDefinition sound;
		sound.name = "HeavyImpact";
		sound.priority = 95;

		SELayer boom = MakeLayer("Boom", Wave::Sine, 0.65f, 1.0f);
		SetPitchSweep(boom, 170.0f, 32.0f);
		SetEnvelope(boom, 0.002f, 0.01f, 0.58f, 0.0f, 0.05f, 2.4f);
		sound.layers.push_back(boom);

		SELayer rubble = MakeLayer("Rubble", Wave::PinkNoise, 0.40f, 0.6f);
		SetPitchSweep(rubble, 2600.0f, 200.0f);
		SetEnvelope(rubble, 0.003f, 0.0f, 0.34f, 0.0f, 0.05f, 2.2f);
		SetFilter(rubble, Filter::LowPass, 2400.0f, 1.0f, 300.0f);
		sound.layers.push_back(rubble);

		// 当たった瞬間の芯。これが無いと「ドゴン」ではなく「ドー」になる
		SELayer crack = MakeLayer("Crack", Wave::Sine, 0.10f, 0.35f);
		SetPitchSweep(crack, 1100.0f, 300.0f);
		crack.fmRatio = 2.1f;
		crack.fmAmount = 0.9f;
		SetEnvelope(crack, 0.001f, 0.0f, 0.09f, 0.0f, 0.01f, 3.0f);
		sound.layers.push_back(crack);

		sound.distortion.enabled = true;
		sound.distortion.drive = 5.0f;
		sound.distortion.mix = 0.6f;

		sound.reverb.enabled = true;
		sound.reverb.roomSize = 0.4f;
		sound.reverb.damping = 0.6f;
		sound.reverb.mix = 0.25f;

		return sound;
	}

	/// <summary>金属への打撃。倍音が濁るほど金属らしくなる</summary>
	SoundDefinition MakeMetalHit()
	{
		SoundDefinition sound;
		sound.name = "MetalHit";
		sound.priority = 75;

		SELayer strike = MakeLayer("Strike", Wave::Sine, 0.65f, 0.85f);
		strike.frequency = 1450.0f;
		strike.fmRatio = 5.1f;
		strike.fmAmount = 2.0f;
		SetEnvelope(strike, 0.001f, 0.0f, 0.60f, 0.0f, 0.05f, 3.0f);
		sound.layers.push_back(strike);

		SELayer overtone = MakeLayer("Overtone", Wave::Sine, 0.45f, 0.4f);
		overtone.frequency = 2900.0f;
		overtone.fmRatio = 1.41f;
		overtone.fmAmount = 1.0f;
		overtone.pan = 0.25f;
		SetEnvelope(overtone, 0.001f, 0.0f, 0.42f, 0.0f, 0.03f, 3.0f);
		sound.layers.push_back(overtone);

		SELayer hit = MakeLayer("Hit", Wave::WhiteNoise, 0.06f, 0.4f);
		SetPitchSweep(hit, 11000.0f, 3000.0f);
		SetEnvelope(hit, 0.001f, 0.0f, 0.05f, 0.0f, 0.01f, 3.0f);
		SetFilter(hit, Filter::HighPass, 2000.0f, 0.8f);
		sound.layers.push_back(hit);

		sound.reverb.enabled = true;
		sound.reverb.roomSize = 0.5f;
		sound.reverb.damping = 0.45f;
		sound.reverb.mix = 0.3f;

		return sound;
	}

	/// <summary>溜め。Sine → Pitch Rising + Noise → Low Pass（設計書 §5-④）</summary>
	SoundDefinition MakeCharge()
	{
		SoundDefinition sound;
		sound.name = "Charge";
		sound.priority = 65;

		SELayer core = MakeLayer("Core", Wave::Sine, 1.20f, 0.7f);
		SetPitchSweep(core, 180.0f, 1500.0f, Sweep::EaseIn);
		core.fmRatio = 2.0f;
		core.fmAmount = 0.25f;
		SetEnvelope(core, 0.30f, 0.0f, 0.0f, 1.0f, 0.22f, 1.5f);
		sound.layers.push_back(core);

		SELayer energy = MakeLayer("Energy", Wave::WhiteNoise, 1.20f, 0.3f);
		SetPitchSweep(energy, 700.0f, 4500.0f, Sweep::EaseIn);
		SetEnvelope(energy, 0.50f, 0.0f, 0.0f, 1.0f, 0.28f, 1.5f);
		SetFilter(energy, Filter::LowPass, 500.0f, 1.2f, 3200.0f);
		sound.layers.push_back(energy);

		return sound;
	}

	/// <summary>ロックオン。短い2音</summary>
	SoundDefinition MakeLockOn()
	{
		SoundDefinition sound;
		sound.name = "LockOn";
		sound.priority = 60;

		SELayer low = MakeLayer("Blip1", Wave::Sine, 0.07f, 0.6f);
		low.frequency = 1250.0f;
		SetEnvelope(low, 0.002f, 0.01f, 0.05f, 0.0f, 0.01f, 2.0f);
		sound.layers.push_back(low);

		SELayer high = MakeLayer("Blip2", Wave::Sine, 0.09f, 0.6f);
		high.frequency = 1875.0f;
		high.startDelay = 0.08f;
		SetEnvelope(high, 0.002f, 0.01f, 0.07f, 0.0f, 0.01f, 2.0f);
		sound.layers.push_back(high);

		return sound;
	}

	/// <summary>警告。矩形波の断続音</summary>
	SoundDefinition MakeWarning()
	{
		SoundDefinition sound;
		sound.name = "Warning";
		sound.priority = 85;

		for (int i = 0; i < 2; ++i) {
			SELayer beep = MakeLayer(i == 0 ? "Beep1" : "Beep2", Wave::Square, 0.16f, 0.5f);
			beep.frequency = 880.0f;
			beep.pulseWidth = 0.35f;
			beep.startDelay = 0.24f * static_cast<float>(i);
			SetEnvelope(beep, 0.006f, 0.10f, 0.04f, 0.0f, 0.02f, 1.6f);
			sound.layers.push_back(beep);
		}

		sound.masterFilterType = Filter::LowPass;
		sound.masterFilterCutoff = 4200.0f;

		return sound;
	}

	/// <summary>決定音。Sine → 短いEnvelope → Pitch Rise（設計書 §5-⑤）</summary>
	SoundDefinition MakeUIConfirm()
	{
		SoundDefinition sound;
		sound.name = "UIConfirm";
		sound.priority = 40;

		SELayer tone = MakeLayer("Tone", Wave::Sine, 0.13f, 0.6f);
		SetPitchSweep(tone, 820.0f, 1640.0f, Sweep::EaseOut);
		SetEnvelope(tone, 0.004f, 0.0f, 0.11f, 0.0f, 0.02f, 2.0f);
		sound.layers.push_back(tone);

		SELayer tail = MakeLayer("Tail", Wave::Sine, 0.11f, 0.3f);
		tail.frequency = 1640.0f;
		tail.startDelay = 0.06f;
		SetEnvelope(tail, 0.003f, 0.0f, 0.09f, 0.0f, 0.02f, 2.4f);
		sound.layers.push_back(tail);

		return sound;
	}

	/// <summary>キャンセル音。決定と逆に下がる</summary>
	SoundDefinition MakeUICancel()
	{
		SoundDefinition sound;
		sound.name = "UICancel";
		sound.priority = 40;

		SELayer tone = MakeLayer("Tone", Wave::Sine, 0.16f, 0.55f);
		SetPitchSweep(tone, 700.0f, 330.0f, Sweep::EaseOut);
		SetEnvelope(tone, 0.004f, 0.0f, 0.14f, 0.0f, 0.02f, 2.0f);
		sound.layers.push_back(tone);

		return sound;
	}

	/// <summary>魔法。上がる高音にディレイとリバーブ</summary>
	SoundDefinition MakeMagic()
	{
		SoundDefinition sound;
		sound.name = "Magic";
		sound.priority = 70;

		SELayer sparkle = MakeLayer("Sparkle", Wave::Sine, 0.42f, 0.7f);
		SetPitchSweep(sparkle, 900.0f, 2500.0f, Sweep::EaseOut);
		sparkle.fmRatio = 1.5f;
		sparkle.fmAmount = 0.8f;
		SetEnvelope(sparkle, 0.02f, 0.0f, 0.36f, 0.0f, 0.04f, 2.2f);
		sound.layers.push_back(sparkle);

		SELayer air = MakeLayer("Air", Wave::Triangle, 0.32f, 0.3f);
		air.frequency = 1850.0f;
		air.startDelay = 0.05f;
		air.pan = -0.3f;
		SetEnvelope(air, 0.02f, 0.0f, 0.27f, 0.0f, 0.03f, 2.2f);
		sound.layers.push_back(air);

		sound.delay.enabled = true;
		sound.delay.time = 0.09f;
		sound.delay.feedback = 0.45f;
		sound.delay.mix = 0.4f;

		sound.reverb.enabled = true;
		sound.reverb.roomSize = 0.6f;
		sound.reverb.damping = 0.4f;
		sound.reverb.mix = 0.35f;

		return sound;
	}

	/// <summary>爆発。低音の轟き＋広い帯域のノイズ</summary>
	SoundDefinition MakeExplosion()
	{
		SoundDefinition sound;
		sound.name = "Explosion";
		sound.priority = 95;

		SELayer blast = MakeLayer("Blast", Wave::PinkNoise, 1.00f, 0.95f);
		SetPitchSweep(blast, 4500.0f, 150.0f);
		SetEnvelope(blast, 0.005f, 0.0f, 0.90f, 0.0f, 0.09f, 2.0f);
		SetFilter(blast, Filter::LowPass, 4500.0f, 0.9f, 220.0f);
		sound.layers.push_back(blast);

		SELayer boom = MakeLayer("Boom", Wave::Sine, 0.90f, 0.9f);
		SetPitchSweep(boom, 130.0f, 26.0f);
		SetEnvelope(boom, 0.004f, 0.02f, 0.80f, 0.0f, 0.08f, 2.2f);
		sound.layers.push_back(boom);

		SELayer crack = MakeLayer("Crack", Wave::WhiteNoise, 0.13f, 0.5f);
		SetPitchSweep(crack, 13000.0f, 2500.0f);
		SetEnvelope(crack, 0.001f, 0.0f, 0.11f, 0.0f, 0.02f, 3.0f);
		SetFilter(crack, Filter::HighPass, 2200.0f, 0.8f);
		sound.layers.push_back(crack);

		sound.distortion.enabled = true;
		sound.distortion.drive = 4.0f;
		sound.distortion.mix = 0.5f;

		sound.reverb.enabled = true;
		sound.reverb.roomSize = 0.7f;
		sound.reverb.damping = 0.5f;
		sound.reverb.mix = 0.35f;

		return sound;
	}

	// ───────────────────────────── 表 ─────────────────────────────

	struct PresetEntry {
		const char* name;
		const char* description;
		SoundDefinition(*create)();
	};

	const PresetEntry kPresets[] = {
		{ "Dodge",       "回避。ノイズのピッチを落として抜けさせた風切り",         MakeDodge },
		{ "Dash",        "ダッシュ。回避より長く、上がってから抜ける風",           MakeDash },
		{ "JustDodge",   "ジャスト回避の「キィン！」。FM の金属音＋短い残響",       MakeJustDodge },
		{ "SwordSlash",  "剣の振り。帯域を絞った鋭い風切り",                       MakeSwordSlash },
		{ "SwordHit",    "斬撃ヒット。金属の「カキン」＋低音の重み",               MakeSwordHit },
		{ "HeavyAttack", "強攻撃の振り。重く長い風切り",                           MakeHeavyAttack },
		{ "Impact",      "軽い衝撃。低音のピッチダウン",                           MakeImpact },
		{ "HeavyImpact", "強い衝撃の「ドゴン！」。歪み＋短い残響",                 MakeHeavyImpact },
		{ "MetalHit",    "金属への打撃。倍音を濁らせた響き",                       MakeMetalHit },
		{ "Charge",      "溜め。上がっていく正弦波＋ノイズ",                       MakeCharge },
		{ "LockOn",      "ロックオン。短い2音",                                    MakeLockOn },
		{ "Warning",     "警告。矩形波の断続音",                                   MakeWarning },
		{ "UIConfirm",   "決定音。短く上がる電子音",                               MakeUIConfirm },
		{ "UICancel",    "キャンセル音。決定と逆に下がる",                         MakeUICancel },
		{ "Magic",       "魔法。上がる高音にディレイとリバーブ",                   MakeMagic },
		{ "Explosion",   "爆発。低音の轟き＋広い帯域のノイズ",                     MakeExplosion },
	};

} // namespace

namespace SoundPresets {

	int Count() { return static_cast<int>(std::size(kPresets)); }

	const char* GetName(int index)
	{
		if (index < 0 || index >= Count()) { return ""; }
		return kPresets[index].name;
	}

	const char* GetDescription(int index)
	{
		if (index < 0 || index >= Count()) { return ""; }
		return kPresets[index].description;
	}

	SoundDefinition Create(int index)
	{
		if (index < 0 || index >= Count()) { return CreateEmpty("NewSound"); }
		return kPresets[index].create();
	}

	bool CreateByName(const std::string& name, SoundDefinition& outDefinition)
	{
		for (const PresetEntry& preset : kPresets) {
			if (name == preset.name) {
				outDefinition = preset.create();
				return true;
			}
		}
		return false;
	}

	SoundDefinition CreateEmpty(const std::string& name)
	{
		SoundDefinition sound;
		sound.name = name;

		SELayer layer = MakeLayer("Layer 1", Wave::Sine, 0.25f, 0.8f);
		layer.frequency = 880.0f;
		SetEnvelope(layer, 0.005f, 0.0f, 0.20f, 0.0f, 0.04f, 2.0f);
		sound.layers.push_back(layer);

		return sound;
	}

	int ExportAll(bool overwrite)
	{
		int written = 0;
		for (const PresetEntry& preset : kPresets) {
			// 触った後のファイルを黙って潰さない。上書きは明示的に選んでもらう
			if (!overwrite && SoundFile::Exists(preset.name)) { continue; }

			// 同じ名前の WAV 素材があるものは書き出さない。
			// SoundManager は .sound を優先するので、ここで作ると
			// SwordSlash のように既にゲームで鳴っている音が黙って差し替わってしまう。
			// 差し替えたいときは「プリセットから作る」で開いて、自分で保存すればよい
			if (WavAssetExists(preset.name)) { continue; }

			if (SoundFile::Save(preset.create())) { ++written; }
		}
		return written;
	}

} // namespace SoundPresets
