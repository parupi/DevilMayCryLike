#include "GameSoundBuilders.h"

// プレイヤーまわりの SE。Sound.txt の「2. プレイヤー」に対応する。
//
// 優先度は設計書 SEEditor.md §Phase 8 の表に合わせてある:
//   ボス攻撃100 / ジャスト回避90 / プレイヤー攻撃80 / 敵ヒット50 / 足音20 / 環境10

using namespace GameSoundBuild;

namespace GameSoundDefs {

	// ───────────────────────────── 移動 ─────────────────────────────

	/// <summary>走りの足音。石畳を踏む短い音</summary>
	SoundDefinition PlayerFootstep()
	{
		SoundDefinition sound = Sound("PlayerFootstep", 20);

		SELayer step = Layer("Step", Wave::PinkNoise, 0.11f, 0.8f);
		Pitch(step, 2200.0f, 600.0f);
		Env(step, 0.002f, 0.0f, 0.09f, 0.0f, 0.015f, 3.0f);
		Filt(step, Filter::LowPass, 2500.0f, 0.9f, 700.0f);
		sound.layers.push_back(step);

		// 体重が乗った分の低音。これが無いと「サッ」で終わって歩いている感じが出ない
		SELayer thud = Layer("Thud", Wave::Sine, 0.09f, 0.45f);
		Pitch(thud, 140.0f, 60.0f);
		Env(thud, 0.002f, 0.0f, 0.07f, 0.0f, 0.015f, 2.6f);
		sound.layers.push_back(thud);

		return sound;
	}

	/// <summary>ジャンプの踏み切り。足音より強く、上へ抜ける</summary>
	SoundDefinition PlayerJump()
	{
		SoundDefinition sound = Sound("PlayerJump", 40);

		SELayer push = Layer("Push", Wave::PinkNoise, 0.14f, 0.7f);
		Pitch(push, 2600.0f, 800.0f);
		Env(push, 0.003f, 0.0f, 0.11f, 0.0f, 0.02f, 2.6f);
		Filt(push, Filter::LowPass, 3000.0f, 0.9f, 900.0f);
		sound.layers.push_back(push);

		// 上がっていく気配。ピッチを上げるだけで跳んだ方向が伝わる
		SELayer lift = Layer("Lift", Wave::Sine, 0.17f, 0.35f);
		Pitch(lift, 220.0f, 430.0f, Sweep::EaseOut);
		Env(lift, 0.01f, 0.0f, 0.14f, 0.0f, 0.02f);
		sound.layers.push_back(lift);

		return sound;
	}

	/// <summary>着地。低い衝撃＋砂利</summary>
	SoundDefinition PlayerLand()
	{
		SoundDefinition sound = Sound("PlayerLand", 45);

		SELayer impact = Layer("Impact", Wave::Sine, 0.22f, 0.9f);
		Pitch(impact, 200.0f, 55.0f);
		Env(impact, 0.002f, 0.0f, 0.19f, 0.0f, 0.025f, 2.6f);
		sound.layers.push_back(impact);

		SELayer gravel = Layer("Gravel", Wave::PinkNoise, 0.17f, 0.55f);
		Pitch(gravel, 3000.0f, 500.0f);
		Env(gravel, 0.002f, 0.0f, 0.14f, 0.0f, 0.02f, 2.8f);
		Filt(gravel, Filter::LowPass, 3000.0f, 0.9f, 500.0f);
		sound.layers.push_back(gravel);

		return sound;
	}

	// ───────────────────────────── 被弾 ─────────────────────────────

	/// <summary>軽い被弾。鈍い打撃に痛みの合図を少し足す</summary>
	SoundDefinition PlayerDamage()
	{
		SoundDefinition sound = Sound("PlayerDamage", 75);

		SELayer thud = Layer("Thud", Wave::Sine, 0.20f, 0.9f);
		Pitch(thud, 300.0f, 90.0f);
		Env(thud, 0.001f, 0.0f, 0.17f, 0.0f, 0.025f, 2.5f);
		sound.layers.push_back(thud);

		SELayer sting = Layer("Sting", Wave::WhiteNoise, 0.10f, 0.45f);
		Pitch(sting, 5000.0f, 1200.0f);
		Env(sting, 0.001f, 0.0f, 0.085f, 0.0f, 0.012f, 3.0f);
		Filt(sting, Filter::HighPass, 1100.0f, 0.8f);
		sound.layers.push_back(sting);

		// 濁った高音。被弾したことが分かりやすくなる
		SELayer pain = Layer("Pain", Wave::Sine, 0.15f, 0.3f);
		Pitch(pain, 560.0f, 420.0f);
		FM(pain, 2.4f, 0.9f);
		Env(pain, 0.002f, 0.0f, 0.13f, 0.0f, 0.015f, 2.8f);
		sound.layers.push_back(pain);

		Distortion(sound, 3.0f, 0.35f);
		return sound;
	}

	/// <summary>吹き飛ばされた被弾。軽被弾より重く長い</summary>
	SoundDefinition PlayerDamageHeavy()
	{
		SoundDefinition sound = Sound("PlayerDamageHeavy", 88);

		SELayer blast = Layer("Blast", Wave::Sine, 0.42f, 1.0f);
		Pitch(blast, 240.0f, 48.0f);
		Env(blast, 0.001f, 0.01f, 0.37f, 0.0f, 0.04f, 2.4f);
		sound.layers.push_back(blast);

		SELayer rip = Layer("Rip", Wave::PinkNoise, 0.30f, 0.6f);
		Pitch(rip, 4000.0f, 400.0f);
		Env(rip, 0.002f, 0.0f, 0.26f, 0.0f, 0.03f, 2.3f);
		Filt(rip, Filter::LowPass, 4000.0f, 0.9f, 400.0f);
		sound.layers.push_back(rip);

		SELayer sting = Layer("Sting", Wave::Sine, 0.20f, 0.35f);
		Pitch(sting, 620.0f, 300.0f);
		FM(sting, 3.1f, 1.2f);
		Env(sting, 0.001f, 0.0f, 0.18f, 0.0f, 0.02f, 3.0f);
		sound.layers.push_back(sting);

		Distortion(sound, 5.0f, 0.55f);
		Reverb(sound, 0.35f, 0.55f, 0.2f);
		return sound;
	}

	/// <summary>
	/// HP が少ないときの心音。ループさせる前提で、2拍のあと無音の間を置く。
	///
	/// 間は「長さだけ確保して、エンベロープを短く切る」で作っている。
	/// レイヤーの長さが SE 全体の長さになるので、最後のレイヤーを伸ばせば後ろに無音が付く
	/// </summary>
	SoundDefinition PlayerLowHealth()
	{
		SoundDefinition sound = Sound("PlayerLowHealth", 35);

		SELayer first = Layer("Beat1", Wave::Sine, 0.24f, 1.0f);
		Pitch(first, 72.0f, 44.0f);
		Env(first, 0.008f, 0.0f, 0.21f, 0.0f, 0.02f, 2.2f);
		Filt(first, Filter::LowPass, 240.0f, 1.2f);
		sound.layers.push_back(first);

		// 2拍目は少し低く弱く。長さ 0.8 秒のうち鳴るのは頭の 0.26 秒だけ
		SELayer second = Layer("Beat2", Wave::Sine, 0.80f, 0.75f);
		second.startDelay = 0.30f;
		Pitch(second, 64.0f, 40.0f, Sweep::Exponential, 0.26f);
		Env(second, 0.008f, 0.0f, 0.23f, 0.0f, 0.02f, 2.2f);
		Filt(second, Filter::LowPass, 220.0f, 1.2f);
		sound.layers.push_back(second);

		return sound;
	}

	/// <summary>死亡。とどめの衝撃から落ちていく余韻まで</summary>
	SoundDefinition PlayerDeath()
	{
		SoundDefinition sound = Sound("PlayerDeath", 100);

		SELayer impact = Layer("Impact", Wave::Sine, 0.70f, 1.0f);
		Pitch(impact, 200.0f, 30.0f);
		Env(impact, 0.001f, 0.01f, 0.63f, 0.0f, 0.06f, 2.2f);
		sound.layers.push_back(impact);

		SELayer rubble = Layer("Rubble", Wave::PinkNoise, 0.50f, 0.6f);
		Pitch(rubble, 3000.0f, 200.0f);
		Env(rubble, 0.002f, 0.0f, 0.44f, 0.0f, 0.05f, 2.2f);
		Filt(rubble, Filter::LowPass, 3000.0f, 0.9f, 200.0f);
		sound.layers.push_back(rubble);

		// 力が抜けて落ちていく音。直線で下げると生々しくなりすぎないので Linear
		SELayer fall = Layer("Fall", Wave::Sine, 1.20f, 0.4f);
		fall.startDelay = 0.15f;
		Pitch(fall, 520.0f, 80.0f, Sweep::Linear);
		FM(fall, 1.5f, 0.4f);
		Env(fall, 0.05f, 0.0f, 1.05f, 0.0f, 0.08f, 1.6f);
		sound.layers.push_back(fall);

		Distortion(sound, 4.0f, 0.5f);
		Reverb(sound, 0.65f, 0.45f, 0.35f);
		return sound;
	}

	// ───────────────────────────── 攻撃の振り ─────────────────────────────

	/// <summary>納刀。鞘に滑り込ませて最後に鳴らす</summary>
	SoundDefinition PlayerSheathe()
	{
		SoundDefinition sound = Sound("PlayerSheathe", 45);

		SELayer slide = Layer("Slide", Wave::WhiteNoise, 0.22f, 0.6f);
		Pitch(slide, 6000.0f, 2500.0f);
		Env(slide, 0.02f, 0.0f, 0.17f, 0.0f, 0.03f, 1.8f);
		Filt(slide, Filter::BandPass, 3000.0f, 2.6f, 1800.0f);
		sound.layers.push_back(slide);

		// 鍔が鳴る「カチッ」。これで納刀が終わったことが分かる
		SELayer click = Layer("Click", Wave::Sine, 0.12f, 0.5f);
		click.startDelay = 0.19f;
		click.frequency = 1500.0f;
		FM(click, 3.7f, 1.0f);
		Env(click, 0.001f, 0.0f, 0.10f, 0.0f, 0.015f, 3.0f);
		sound.layers.push_back(click);

		return sound;
	}

	/// <summary>クリアの拍手。短い破裂を3回</summary>
	SoundDefinition PlayerClap()
	{
		SoundDefinition sound = Sound("PlayerClap", 50);

		const float delays[3] = { 0.0f, 0.21f, 0.43f };
		const float volumes[3] = { 0.9f, 0.8f, 0.95f };
		const char* names[3] = { "Clap1", "Clap2", "Clap3" };

		for (int i = 0; i < 3; ++i) {
			SELayer clap = Layer(names[i], Wave::WhiteNoise, 0.09f, volumes[i]);
			clap.startDelay = delays[i];
			// 1回ごとに少しずらすと機械的な繰り返しに聞こえない
			Pitch(clap, 7000.0f - 400.0f * i, 1500.0f);
			clap.noiseSeed = 4400 + i * 137;
			Env(clap, 0.001f, 0.0f, 0.075f, 0.0f, 0.012f, 3.0f);
			Filt(clap, Filter::BandPass, 2200.0f, 1.2f);
			sound.layers.push_back(clap);
		}

		Reverb(sound, 0.5f, 0.4f, 0.3f);
		return sound;
	}

	/// <summary>強攻撃の重い風切り。通常斬りより遅く、低い成分が多い</summary>
	SoundDefinition SwordSlashHeavy()
	{
		SoundDefinition sound = Sound("SwordSlashHeavy", 85);

		SELayer swing = Layer("Swing", Wave::PinkNoise, 0.42f, 0.95f);
		Pitch(swing, 4000.0f, 700.0f);
		Env(swing, 0.11f, 0.0f, 0.25f, 0.0f, 0.06f, 1.9f);
		Filt(swing, Filter::LowPass, 4000.0f, 1.1f, 500.0f);
		sound.layers.push_back(swing);

		SELayer mass = Layer("Mass", Wave::Saw, 0.36f, 0.5f);
		Pitch(mass, 200.0f, 60.0f);
		Env(mass, 0.09f, 0.0f, 0.22f, 0.0f, 0.05f);
		Filt(mass, Filter::LowPass, 800.0f, 0.9f);
		sound.layers.push_back(mass);

		Distortion(sound, 2.2f, 0.35f);
		return sound;
	}

	/// <summary>スティンガー。前へ鋭く伸びるので、ピッチは上げる</summary>
	SoundDefinition SwordStinger()
	{
		SoundDefinition sound = Sound("SwordStinger", 85);

		SELayer thrust = Layer("Thrust", Wave::WhiteNoise, 0.26f, 0.9f);
		Pitch(thrust, 2000.0f, 7000.0f, Sweep::EaseOut);
		Env(thrust, 0.02f, 0.0f, 0.20f, 0.0f, 0.035f, 2.0f);
		Filt(thrust, Filter::BandPass, 1500.0f, 2.2f, 3500.0f);
		sound.layers.push_back(thrust);

		SELayer edge = Layer("Edge", Wave::Saw, 0.20f, 0.35f);
		Pitch(edge, 300.0f, 900.0f, Sweep::EaseOut);
		Env(edge, 0.01f, 0.0f, 0.17f, 0.0f, 0.02f);
		Filt(edge, Filter::LowPass, 3000.0f, 1.0f);
		sound.layers.push_back(edge);

		return sound;
	}

	/// <summary>打ち上げ。下から上へ持ち上がる</summary>
	SoundDefinition SwordLaunch()
	{
		SoundDefinition sound = Sound("SwordLaunch", 85);

		SELayer rise = Layer("Rise", Wave::WhiteNoise, 0.32f, 0.8f);
		Pitch(rise, 1200.0f, 6000.0f, Sweep::EaseIn);
		Env(rise, 0.04f, 0.0f, 0.25f, 0.0f, 0.03f, 1.8f);
		Filt(rise, Filter::HighPass, 600.0f, 0.9f);
		sound.layers.push_back(rise);

		SELayer lift = Layer("Lift", Wave::Sine, 0.34f, 0.45f);
		Pitch(lift, 300.0f, 1400.0f, Sweep::EaseIn);
		FM(lift, 1.5f, 0.4f);
		Env(lift, 0.03f, 0.0f, 0.28f, 0.0f, 0.03f, 1.8f);
		sound.layers.push_back(lift);

		return sound;
	}

	/// <summary>回転斬り・叩きつけ。振り回してから落とす</summary>
	SoundDefinition SwordSlam()
	{
		SoundDefinition sound = Sound("SwordSlam", 85);

		SELayer whirl = Layer("Whirl", Wave::WhiteNoise, 0.34f, 0.85f);
		Pitch(whirl, 5000.0f, 900.0f);
		Env(whirl, 0.06f, 0.0f, 0.24f, 0.0f, 0.04f, 2.0f);
		Filt(whirl, Filter::BandPass, 2000.0f, 1.8f, 700.0f);
		sound.layers.push_back(whirl);

		// 落ちきった衝撃。振りの後ろに置く
		SELayer drop = Layer("Drop", Wave::Sine, 0.30f, 0.7f);
		drop.startDelay = 0.16f;
		Pitch(drop, 260.0f, 60.0f);
		Env(drop, 0.002f, 0.0f, 0.26f, 0.0f, 0.03f, 2.5f);
		sound.layers.push_back(drop);

		Distortion(sound, 3.0f, 0.4f);
		return sound;
	}

	/// <summary>
	/// 溜め中のループ。ゆっくり膨らんで戻る唸り。
	///
	/// 頭と尻がどちらも 0 になる形にしてあるので、ループの継ぎ目で段差が出ない
	/// </summary>
	SoundDefinition SwordCharge()
	{
		SoundDefinition sound = Sound("SwordCharge", 60);

		SELayer hum = Layer("Hum", Wave::Saw, 1.0f, 0.7f);
		hum.frequency = 92.0f;
		FM(hum, 2.0f, 0.3f);
		Env(hum, 0.28f, 0.0f, 0.0f, 1.0f, 0.28f, 1.4f);
		Filt(hum, Filter::LowPass, 900.0f, 2.0f);
		sound.layers.push_back(hum);

		SELayer energy = Layer("Energy", Wave::WhiteNoise, 1.0f, 0.3f);
		energy.frequency = 900.0f;
		Env(energy, 0.32f, 0.0f, 0.0f, 1.0f, 0.32f, 1.4f);
		Filt(energy, Filter::LowPass, 700.0f, 3.0f);
		sound.layers.push_back(energy);

		return sound;
	}

	/// <summary>溜めきった合図。短く澄んだ「キン」</summary>
	SoundDefinition SwordChargeReady()
	{
		SoundDefinition sound = Sound("SwordChargeReady", 80);

		SELayer ping = Layer("Ping", Wave::Sine, 0.40f, 0.8f);
		ping.frequency = 1850.0f;
		FM(ping, 2.4f, 0.7f);
		Env(ping, 0.002f, 0.0f, 0.36f, 0.0f, 0.03f, 3.0f);
		sound.layers.push_back(ping);

		SELayer up = Layer("Up", Wave::Sine, 0.16f, 0.4f);
		Pitch(up, 920.0f, 1850.0f, Sweep::EaseOut);
		Env(up, 0.004f, 0.0f, 0.14f, 0.0f, 0.015f, 2.2f);
		sound.layers.push_back(up);

		Reverb(sound, 0.4f, 0.4f, 0.25f);
		return sound;
	}

	/// <summary>カウンター成立。ジャスト回避より重い「ギィン」</summary>
	SoundDefinition SwordCounter()
	{
		SoundDefinition sound = Sound("SwordCounter", 92);

		SELayer clash = Layer("Clash", Wave::Sine, 0.55f, 0.9f);
		clash.frequency = 1500.0f;
		FM(clash, 3.9f, 1.6f);
		Env(clash, 0.001f, 0.0f, 0.50f, 0.0f, 0.045f, 3.0f);
		sound.layers.push_back(clash);

		SELayer spark = Layer("Spark", Wave::WhiteNoise, 0.08f, 0.5f);
		Pitch(spark, 10000.0f, 2500.0f);
		Env(spark, 0.001f, 0.0f, 0.07f, 0.0f, 0.01f, 3.0f);
		Filt(spark, Filter::HighPass, 1900.0f, 0.8f);
		sound.layers.push_back(spark);

		SELayer weight = Layer("Weight", Wave::Sine, 0.20f, 0.5f);
		Pitch(weight, 260.0f, 80.0f);
		Env(weight, 0.002f, 0.0f, 0.18f, 0.0f, 0.02f, 2.5f);
		sound.layers.push_back(weight);

		Distortion(sound, 3.0f, 0.4f);
		Reverb(sound, 0.5f, 0.4f, 0.3f);
		return sound;
	}

	// ───────────────────────────── 手応え（材質別） ─────────────────────────────

	/// <summary>生身。鈍く湿った手応え</summary>
	SoundDefinition HitFlesh()
	{
		SoundDefinition sound = Sound("HitFlesh", 78);

		SELayer thud = Layer("Thud", Wave::Sine, 0.18f, 0.9f);
		Pitch(thud, 260.0f, 70.0f);
		Env(thud, 0.001f, 0.0f, 0.16f, 0.0f, 0.02f, 2.6f);
		sound.layers.push_back(thud);

		SELayer splat = Layer("Splat", Wave::PinkNoise, 0.12f, 0.55f);
		Pitch(splat, 1800.0f, 350.0f);
		Env(splat, 0.001f, 0.0f, 0.10f, 0.0f, 0.015f, 2.8f);
		Filt(splat, Filter::LowPass, 1800.0f, 0.9f, 350.0f);
		sound.layers.push_back(splat);

		return sound;
	}

	/// <summary>骨。乾いた破砕音</summary>
	SoundDefinition HitBone()
	{
		SoundDefinition sound = Sound("HitBone", 78);

		SELayer crack = Layer("Crack", Wave::WhiteNoise, 0.10f, 0.9f);
		Pitch(crack, 8000.0f, 1500.0f);
		Env(crack, 0.001f, 0.0f, 0.085f, 0.0f, 0.012f, 3.0f);
		Filt(crack, Filter::BandPass, 2500.0f, 1.5f);
		sound.layers.push_back(crack);

		// 折れる瞬間の「パキッ」。矩形波を極端に短く切ると木や骨が折れた感じになる
		SELayer snap = Layer("Snap", Wave::Square, 0.05f, 0.5f);
		snap.frequency = 720.0f;
		snap.pulseWidth = 0.2f;
		Env(snap, 0.001f, 0.0f, 0.04f, 0.0f, 0.008f, 3.0f);
		sound.layers.push_back(snap);

		SELayer body = Layer("Body", Wave::Sine, 0.14f, 0.45f);
		Pitch(body, 200.0f, 70.0f);
		Env(body, 0.001f, 0.0f, 0.12f, 0.0f, 0.018f, 2.6f);
		sound.layers.push_back(body);

		return sound;
	}

	/// <summary>木。中空の「コン」</summary>
	SoundDefinition HitWood()
	{
		SoundDefinition sound = Sound("HitWood", 78);

		SELayer knock = Layer("Knock", Wave::Sine, 0.16f, 0.85f);
		knock.frequency = 380.0f;
		FM(knock, 2.1f, 0.5f);
		Env(knock, 0.001f, 0.0f, 0.14f, 0.0f, 0.018f, 3.0f);
		sound.layers.push_back(knock);

		SELayer tap = Layer("Tap", Wave::WhiteNoise, 0.05f, 0.45f);
		Pitch(tap, 4000.0f, 900.0f);
		Env(tap, 0.001f, 0.0f, 0.04f, 0.0f, 0.008f, 3.0f);
		Filt(tap, Filter::BandPass, 1500.0f, 1.2f);
		sound.layers.push_back(tap);

		SELayer low = Layer("Low", Wave::Sine, 0.12f, 0.4f);
		Pitch(low, 160.0f, 90.0f);
		Env(low, 0.002f, 0.0f, 0.10f, 0.0f, 0.015f, 2.4f);
		sound.layers.push_back(low);

		return sound;
	}

	/// <summary>硬い鱗・鎧。弾かれた金属音</summary>
	SoundDefinition HitArmor()
	{
		SoundDefinition sound = Sound("HitArmor", 78);

		SELayer clang = Layer("Clang", Wave::Sine, 0.42f, 0.85f);
		clang.frequency = 2100.0f;
		FM(clang, 4.3f, 1.5f);
		Env(clang, 0.001f, 0.0f, 0.38f, 0.0f, 0.035f, 3.0f);
		sound.layers.push_back(clang);

		SELayer spark = Layer("Spark", Wave::WhiteNoise, 0.08f, 0.5f);
		Pitch(spark, 11000.0f, 3000.0f);
		Env(spark, 0.001f, 0.0f, 0.07f, 0.0f, 0.01f, 3.0f);
		Filt(spark, Filter::HighPass, 2100.0f, 0.8f);
		sound.layers.push_back(spark);

		SELayer ring = Layer("Ring", Wave::Sine, 0.30f, 0.3f);
		ring.frequency = 3300.0f;
		ring.pan = 0.2f;
		FM(ring, 1.41f, 0.8f);
		Env(ring, 0.001f, 0.0f, 0.27f, 0.0f, 0.025f, 3.0f);
		sound.layers.push_back(ring);

		Reverb(sound, 0.45f, 0.4f, 0.28f);
		return sound;
	}

	/// <summary>強攻撃が当たった重い衝撃</summary>
	SoundDefinition HitHeavy()
	{
		SoundDefinition sound = Sound("HitHeavy", 90);

		SELayer boom = Layer("Boom", Wave::Sine, 0.50f, 1.0f);
		Pitch(boom, 190.0f, 40.0f);
		Env(boom, 0.002f, 0.01f, 0.44f, 0.0f, 0.045f, 2.4f);
		sound.layers.push_back(boom);

		SELayer debris = Layer("Debris", Wave::PinkNoise, 0.30f, 0.6f);
		Pitch(debris, 3000.0f, 250.0f);
		Env(debris, 0.002f, 0.0f, 0.26f, 0.0f, 0.03f, 2.2f);
		Filt(debris, Filter::LowPass, 3000.0f, 1.0f, 300.0f);
		sound.layers.push_back(debris);

		// 当たった瞬間の芯。これが無いと「ドー」としか鳴らない
		SELayer crack = Layer("Crack", Wave::Sine, 0.09f, 0.4f);
		Pitch(crack, 1000.0f, 300.0f);
		FM(crack, 2.1f, 0.9f);
		Env(crack, 0.001f, 0.0f, 0.08f, 0.0f, 0.01f, 3.0f);
		sound.layers.push_back(crack);

		Distortion(sound, 4.5f, 0.55f);
		Reverb(sound, 0.35f, 0.6f, 0.22f);
		return sound;
	}

	/// <summary>スーパーアーマーに弾かれた。手応えが無く詰まった音</summary>
	SoundDefinition HitBlocked()
	{
		SoundDefinition sound = Sound("HitBlocked", 70);

		SELayer dull = Layer("Dull", Wave::Sine, 0.14f, 0.7f);
		Pitch(dull, 420.0f, 160.0f);
		Env(dull, 0.001f, 0.0f, 0.12f, 0.0f, 0.016f, 3.0f);
		sound.layers.push_back(dull);

		// 金属質だが、ローパスで抜けを削って「通っていない」感じにする
		SELayer clank = Layer("Clank", Wave::Sine, 0.12f, 0.45f);
		clank.frequency = 900.0f;
		FM(clank, 5.2f, 1.2f);
		Env(clank, 0.001f, 0.0f, 0.10f, 0.0f, 0.015f, 3.0f);
		Filt(clank, Filter::LowPass, 2200.0f, 1.0f);
		sound.layers.push_back(clank);

		SELayer muted = Layer("Muted", Wave::PinkNoise, 0.10f, 0.4f);
		Pitch(muted, 1500.0f, 400.0f);
		Env(muted, 0.001f, 0.0f, 0.085f, 0.0f, 0.012f, 2.8f);
		Filt(muted, Filter::LowPass, 1200.0f, 0.9f);
		sound.layers.push_back(muted);

		return sound;
	}

	// ───────────────────────────── ロックオン ─────────────────────────────

	/// <summary>ロックオン解除。成立時と逆に下がる2音</summary>
	SoundDefinition LockOnRelease()
	{
		SoundDefinition sound = Sound("LockOnRelease", 55);

		SELayer high = Layer("Blip1", Wave::Sine, 0.06f, 0.5f);
		high.frequency = 1875.0f;
		Env(high, 0.002f, 0.01f, 0.04f, 0.0f, 0.008f, 2.0f);
		sound.layers.push_back(high);

		SELayer low = Layer("Blip2", Wave::Sine, 0.09f, 0.5f);
		low.startDelay = 0.07f;
		low.frequency = 1250.0f;
		Env(low, 0.002f, 0.01f, 0.07f, 0.0f, 0.01f, 2.0f);
		sound.layers.push_back(low);

		return sound;
	}

	/// <summary>ターゲット切り替え。1音だけの短い合図</summary>
	SoundDefinition LockOnSwitch()
	{
		SoundDefinition sound = Sound("LockOnSwitch", 55);

		SELayer blip = Layer("Blip", Wave::Sine, 0.07f, 0.55f);
		blip.frequency = 1560.0f;
		FM(blip, 1.5f, 0.3f);
		Env(blip, 0.002f, 0.01f, 0.05f, 0.0f, 0.008f, 2.2f);
		sound.layers.push_back(blip);

		return sound;
	}

} // namespace GameSoundDefs
