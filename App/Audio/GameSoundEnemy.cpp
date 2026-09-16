#include "GameSoundBuilders.h"

// 雑魚敵（骸骨）とボス（ドラゴン）の SE。Sound.txt の「3.」「4.」に対応する。
//
// ドラゴンは体が大きいので、プレイヤーの音より1オクターブ近く低いところに芯を置き、
// 立ち上がりを鈍くしてある。小さい音をそのまま低くしただけでは「重い」とは聞こえない。

using namespace GameSoundBuild;

namespace GameSoundDefs {

	// ───────────────────────────── 雑魚敵（骸骨） ─────────────────────────────

	/// <summary>出現。黒い粒子が集まって形になる演出に合わせた、吸い込まれるような音</summary>
	SoundDefinition SkeletonSpawn()
	{
		SoundDefinition sound = Sound("SkeletonSpawn", 55);

		// 粒子が集まってくる間。低いところから上がってくる
		SELayer gather = Layer("Gather", Wave::PinkNoise, 0.85f, 0.7f);
		Pitch(gather, 300.0f, 2200.0f, Sweep::EaseIn);
		Env(gather, 0.45f, 0.0f, 0.30f, 0.0f, 0.09f, 1.5f);
		Filt(gather, Filter::BandPass, 400.0f, 1.6f, 2000.0f);
		sound.layers.push_back(gather);

		// 形になった瞬間の骨の音
		SELayer form = Layer("Form", Wave::WhiteNoise, 0.18f, 0.6f);
		form.startDelay = 0.62f;
		Pitch(form, 5000.0f, 900.0f);
		Env(form, 0.002f, 0.0f, 0.15f, 0.0f, 0.02f, 2.8f);
		Filt(form, Filter::BandPass, 2000.0f, 1.4f);
		sound.layers.push_back(form);

		SELayer thud = Layer("Thud", Wave::Sine, 0.26f, 0.55f);
		thud.startDelay = 0.62f;
		Pitch(thud, 180.0f, 55.0f);
		Env(thud, 0.002f, 0.0f, 0.23f, 0.0f, 0.025f, 2.5f);
		sound.layers.push_back(thud);

		Reverb(sound, 0.55f, 0.45f, 0.3f);
		return sound;
	}

	/// <summary>骸骨の足音。プレイヤーより軽く、骨が当たる音が混ざる</summary>
	SoundDefinition SkeletonFootstep()
	{
		SoundDefinition sound = Sound("SkeletonFootstep", 20);

		SELayer step = Layer("Step", Wave::WhiteNoise, 0.09f, 0.7f);
		Pitch(step, 3500.0f, 900.0f);
		Env(step, 0.001f, 0.0f, 0.075f, 0.0f, 0.012f, 3.0f);
		Filt(step, Filter::BandPass, 1800.0f, 1.3f, 800.0f);
		sound.layers.push_back(step);

		// 骨が鳴るカタッという成分
		SELayer bone = Layer("Bone", Wave::Sine, 0.07f, 0.4f);
		bone.frequency = 620.0f;
		FM(bone, 3.1f, 0.9f);
		Env(bone, 0.001f, 0.0f, 0.06f, 0.0f, 0.008f, 3.0f);
		sound.layers.push_back(bone);

		return sound;
	}

	/// <summary>攻撃の構え。チャージリングが縮むのに合わせて上がっていく</summary>
	SoundDefinition SkeletonCharge()
	{
		SoundDefinition sound = Sound("SkeletonCharge", 65);

		SELayer rise = Layer("Rise", Wave::Sine, 0.55f, 0.6f);
		Pitch(rise, 240.0f, 900.0f, Sweep::EaseIn);
		FM(rise, 2.3f, 0.5f);
		Env(rise, 0.18f, 0.0f, 0.0f, 1.0f, 0.12f, 1.5f);
		sound.layers.push_back(rise);

		SELayer air = Layer("Air", Wave::WhiteNoise, 0.55f, 0.3f);
		Pitch(air, 600.0f, 2800.0f, Sweep::EaseIn);
		Env(air, 0.24f, 0.0f, 0.0f, 1.0f, 0.14f, 1.5f);
		Filt(air, Filter::BandPass, 900.0f, 2.0f, 2400.0f);
		sound.layers.push_back(air);

		return sound;
	}

	/// <summary>骸骨の剣の振り。プレイヤーより細く軽い</summary>
	SoundDefinition SkeletonSwing()
	{
		SoundDefinition sound = Sound("SkeletonSwing", 70);

		SELayer swing = Layer("Swing", Wave::WhiteNoise, 0.20f, 0.85f);
		Pitch(swing, 6000.0f, 1400.0f);
		Env(swing, 0.018f, 0.0f, 0.15f, 0.0f, 0.03f, 2.4f);
		Filt(swing, Filter::BandPass, 2400.0f, 2.2f, 1000.0f);
		sound.layers.push_back(swing);

		SELayer rattle = Layer("Rattle", Wave::Sine, 0.10f, 0.28f);
		rattle.frequency = 760.0f;
		FM(rattle, 4.1f, 1.1f);
		Env(rattle, 0.002f, 0.0f, 0.085f, 0.0f, 0.012f, 3.0f);
		sound.layers.push_back(rattle);

		return sound;
	}

	/// <summary>突進のダッシュ。踏み込みと風</summary>
	SoundDefinition SkeletonRush()
	{
		SoundDefinition sound = Sound("SkeletonRush", 72);

		SELayer dash = Layer("Dash", Wave::WhiteNoise, 0.34f, 0.85f);
		Pitch(dash, 1400.0f, 5000.0f, Sweep::EaseOut);
		Env(dash, 0.06f, 0.0f, 0.24f, 0.0f, 0.04f, 1.7f);
		Filt(dash, Filter::BandPass, 800.0f, 1.5f, 2400.0f);
		sound.layers.push_back(dash);

		SELayer step = Layer("Step", Wave::Sine, 0.14f, 0.5f);
		Pitch(step, 200.0f, 65.0f);
		Env(step, 0.002f, 0.0f, 0.12f, 0.0f, 0.018f, 2.6f);
		sound.layers.push_back(step);

		return sound;
	}

	/// <summary>被弾・のけぞり。骨が軋む</summary>
	SoundDefinition SkeletonHit()
	{
		SoundDefinition sound = Sound("SkeletonHit", 50);

		SELayer crack = Layer("Crack", Wave::WhiteNoise, 0.12f, 0.85f);
		Pitch(crack, 6500.0f, 1300.0f);
		Env(crack, 0.001f, 0.0f, 0.10f, 0.0f, 0.015f, 3.0f);
		Filt(crack, Filter::BandPass, 2200.0f, 1.6f, 1100.0f);
		sound.layers.push_back(crack);

		// 骨がぶつかって軋む。FM の比率を大きく外すほどカラカラした響きになる
		SELayer rattle = Layer("Rattle", Wave::Sine, 0.18f, 0.45f);
		rattle.frequency = 540.0f;
		FM(rattle, 5.7f, 1.4f);
		Env(rattle, 0.001f, 0.0f, 0.16f, 0.0f, 0.018f, 3.0f);
		sound.layers.push_back(rattle);

		SELayer body = Layer("Body", Wave::Sine, 0.13f, 0.4f);
		Pitch(body, 220.0f, 80.0f);
		Env(body, 0.001f, 0.0f, 0.11f, 0.0f, 0.016f, 2.6f);
		sound.layers.push_back(body);

		return sound;
	}

	/// <summary>死亡。骨が崩れ落ちる</summary>
	SoundDefinition SkeletonDeath()
	{
		SoundDefinition sound = Sound("SkeletonDeath", 60);

		// 崩れる骨。長めのノイズをバンドパスで「カラカラ」に寄せる
		SELayer collapse = Layer("Collapse", Wave::WhiteNoise, 0.75f, 0.85f);
		Pitch(collapse, 4200.0f, 700.0f);
		Env(collapse, 0.004f, 0.02f, 0.66f, 0.0f, 0.06f, 1.9f);
		Filt(collapse, Filter::BandPass, 2200.0f, 1.3f, 700.0f);
		sound.layers.push_back(collapse);

		SELayer fall = Layer("Fall", Wave::Sine, 0.45f, 0.5f);
		Pitch(fall, 260.0f, 55.0f);
		Env(fall, 0.003f, 0.0f, 0.40f, 0.0f, 0.04f, 2.2f);
		sound.layers.push_back(fall);

		// 最後に床へ落ちる音
		SELayer settle = Layer("Settle", Wave::WhiteNoise, 0.22f, 0.4f);
		settle.startDelay = 0.42f;
		Pitch(settle, 2600.0f, 600.0f);
		settle.noiseSeed = 7717;
		Env(settle, 0.002f, 0.0f, 0.19f, 0.0f, 0.02f, 2.6f);
		Filt(settle, Filter::BandPass, 1500.0f, 1.2f);
		sound.layers.push_back(settle);

		Reverb(sound, 0.5f, 0.5f, 0.28f);
		return sound;
	}

	// ───────────────────────────── ボス（ドラゴン） ─────────────────────────────

	/// <summary>登場の地響き。低いところだけで鳴らして「大きいものが来る」を出す</summary>
	SoundDefinition DragonAppear()
	{
		SoundDefinition sound = Sound("DragonAppear", 100);

		// 地鳴り。立ち上がりを鈍くするのが重要（速いと爆発に聞こえる）
		SELayer rumble = Layer("Rumble", Wave::PinkNoise, 2.0f, 1.0f);
		Pitch(rumble, 120.0f, 45.0f);
		Env(rumble, 0.55f, 0.15f, 1.0f, 0.0f, 0.25f, 1.5f);
		Filt(rumble, Filter::LowPass, 260.0f, 1.4f, 90.0f);
		sound.layers.push_back(rumble);

		SELayer sub = Layer("Sub", Wave::Sine, 1.8f, 0.8f);
		Pitch(sub, 55.0f, 28.0f);
		Env(sub, 0.4f, 0.1f, 1.0f, 0.0f, 0.25f, 1.6f);
		sound.layers.push_back(sub);

		// 揺れで転がる小石
		SELayer debris = Layer("Debris", Wave::WhiteNoise, 1.4f, 0.28f);
		debris.startDelay = 0.4f;
		Pitch(debris, 3200.0f, 900.0f);
		Env(debris, 0.25f, 0.0f, 0.95f, 0.0f, 0.18f, 1.6f);
		Filt(debris, Filter::BandPass, 1800.0f, 1.1f, 700.0f);
		sound.layers.push_back(debris);

		Distortion(sound, 2.5f, 0.35f);
		Reverb(sound, 0.8f, 0.45f, 0.35f);
		return sound;
	}

	/// <summary>
	/// 咆哮。生き物の声は合成では作れないので、
	/// 「低い唸りに濁った倍音を乗せて歪ませる」で代わりにしている
	/// </summary>
	SoundDefinition DragonRoar()
	{
		SoundDefinition sound = Sound("DragonRoar", 100);

		// 声帯の代わり。鋸歯状波を低いところで鳴らし、FM で濁らせる
		SELayer growl = Layer("Growl", Wave::Saw, 1.7f, 1.0f);
		Pitch(growl, 90.0f, 62.0f, Sweep::EaseOut);
		FM(growl, 1.37f, 0.9f);
		Env(growl, 0.12f, 0.25f, 1.0f, 0.0f, 0.3f, 1.4f);
		Filt(growl, Filter::LowPass, 1400.0f, 1.6f, 500.0f);
		sound.layers.push_back(growl);

		// 唸りの上に乗る倍音。うねらせると生き物らしくなる
		SELayer snarl = Layer("Snarl", Wave::Saw, 1.6f, 0.5f);
		Pitch(snarl, 190.0f, 130.0f, Sweep::EaseOut);
		FM(snarl, 2.61f, 1.3f);
		Env(snarl, 0.15f, 0.2f, 0.95f, 0.0f, 0.3f, 1.4f);
		Filt(snarl, Filter::BandPass, 900.0f, 1.2f, 420.0f);
		sound.layers.push_back(snarl);

		// 息の成分。これが無いと電子音に聞こえる
		SELayer breath = Layer("Breath", Wave::PinkNoise, 1.7f, 0.4f);
		Pitch(breath, 2600.0f, 800.0f);
		Env(breath, 0.1f, 0.2f, 1.1f, 0.0f, 0.3f, 1.4f);
		Filt(breath, Filter::BandPass, 1600.0f, 0.9f, 600.0f);
		sound.layers.push_back(breath);

		SELayer sub = Layer("Sub", Wave::Sine, 1.5f, 0.6f);
		Pitch(sub, 48.0f, 34.0f);
		Env(sub, 0.1f, 0.3f, 0.9f, 0.0f, 0.2f, 1.5f);
		sound.layers.push_back(sub);

		Distortion(sound, 3.5f, 0.5f);
		Reverb(sound, 0.75f, 0.4f, 0.32f);
		return sound;
	}

	/// <summary>羽ばたき。1回ぶん。待機中に繰り返し鳴らす</summary>
	SoundDefinition DragonWingbeat()
	{
		SoundDefinition sound = Sound("DragonWingbeat", 45);

		// 膜が空気を打つ音。ピッチを下げながら太くする
		SELayer flap = Layer("Flap", Wave::PinkNoise, 0.42f, 0.9f);
		Pitch(flap, 900.0f, 160.0f);
		Env(flap, 0.05f, 0.0f, 0.32f, 0.0f, 0.05f, 1.8f);
		Filt(flap, Filter::LowPass, 1200.0f, 1.5f, 200.0f);
		sound.layers.push_back(flap);

		SELayer gust = Layer("Gust", Wave::Sine, 0.34f, 0.45f);
		Pitch(gust, 110.0f, 45.0f);
		Env(gust, 0.04f, 0.0f, 0.27f, 0.0f, 0.03f, 1.8f);
		sound.layers.push_back(gust);

		return sound;
	}

	/// <summary>着地。巨体が地面を踏む</summary>
	SoundDefinition DragonLand()
	{
		SoundDefinition sound = Sound("DragonLand", 90);

		SELayer impact = Layer("Impact", Wave::Sine, 0.85f, 1.0f);
		Pitch(impact, 130.0f, 26.0f);
		Env(impact, 0.002f, 0.02f, 0.78f, 0.0f, 0.06f, 2.2f);
		sound.layers.push_back(impact);

		SELayer crush = Layer("Crush", Wave::PinkNoise, 0.5f, 0.65f);
		Pitch(crush, 2600.0f, 200.0f);
		Env(crush, 0.002f, 0.0f, 0.44f, 0.0f, 0.05f, 2.2f);
		Filt(crush, Filter::LowPass, 2600.0f, 1.0f, 240.0f);
		sound.layers.push_back(crush);

		Distortion(sound, 3.5f, 0.45f);
		Reverb(sound, 0.6f, 0.5f, 0.28f);
		return sound;
	}

	/// <summary>噛みつき。顎が閉じてガチンと鳴る</summary>
	SoundDefinition DragonBite()
	{
		SoundDefinition sound = Sound("DragonBite", 95);

		// 顎を振る風
		SELayer swing = Layer("Swing", Wave::PinkNoise, 0.22f, 0.6f);
		Pitch(swing, 2600.0f, 600.0f);
		Env(swing, 0.03f, 0.0f, 0.16f, 0.0f, 0.03f, 2.0f);
		Filt(swing, Filter::BandPass, 1400.0f, 1.4f, 500.0f);
		sound.layers.push_back(swing);

		// 牙がぶつかる瞬間
		SELayer snap = Layer("Snap", Wave::Sine, 0.22f, 0.9f);
		snap.startDelay = 0.16f;
		snap.frequency = 420.0f;
		FM(snap, 3.7f, 1.6f);
		Env(snap, 0.001f, 0.0f, 0.19f, 0.0f, 0.025f, 3.0f);
		sound.layers.push_back(snap);

		SELayer weight = Layer("Weight", Wave::Sine, 0.28f, 0.7f);
		weight.startDelay = 0.16f;
		Pitch(weight, 150.0f, 45.0f);
		Env(weight, 0.002f, 0.0f, 0.25f, 0.0f, 0.03f, 2.4f);
		sound.layers.push_back(weight);

		Distortion(sound, 3.0f, 0.4f);
		return sound;
	}

	/// <summary>叩きつけの溜め。振り上げている間の地鳴り</summary>
	SoundDefinition DragonSlamCharge()
	{
		SoundDefinition sound = Sound("DragonSlamCharge", 85);

		// 上がっていく地鳴り。ピッチを上げると「これから来る」感じになる
		SELayer rumble = Layer("Rumble", Wave::PinkNoise, 1.2f, 0.9f);
		Pitch(rumble, 70.0f, 220.0f, Sweep::EaseIn);
		Env(rumble, 0.3f, 0.0f, 0.0f, 1.0f, 0.2f, 1.4f);
		Filt(rumble, Filter::LowPass, 180.0f, 1.8f, 520.0f);
		sound.layers.push_back(rumble);

		SELayer strain = Layer("Strain", Wave::Saw, 1.2f, 0.4f);
		Pitch(strain, 60.0f, 110.0f, Sweep::EaseIn);
		FM(strain, 1.51f, 0.6f);
		Env(strain, 0.35f, 0.0f, 0.0f, 1.0f, 0.22f, 1.4f);
		Filt(strain, Filter::LowPass, 700.0f, 1.4f);
		sound.layers.push_back(strain);

		Distortion(sound, 2.0f, 0.3f);
		return sound;
	}

	/// <summary>叩きつけの衝撃。このゲームで一番重い音</summary>
	SoundDefinition DragonSlamImpact()
	{
		SoundDefinition sound = Sound("DragonSlamImpact", 100);

		SELayer boom = Layer("Boom", Wave::Sine, 1.1f, 1.0f);
		Pitch(boom, 150.0f, 22.0f);
		Env(boom, 0.001f, 0.02f, 1.0f, 0.0f, 0.07f, 2.3f);
		sound.layers.push_back(boom);

		SELayer rubble = Layer("Rubble", Wave::PinkNoise, 0.7f, 0.7f);
		Pitch(rubble, 3400.0f, 180.0f);
		Env(rubble, 0.002f, 0.0f, 0.62f, 0.0f, 0.07f, 2.1f);
		Filt(rubble, Filter::LowPass, 3400.0f, 1.0f, 220.0f);
		sound.layers.push_back(rubble);

		// 地面が割れる瞬間の芯
		SELayer crack = Layer("Crack", Wave::Sine, 0.12f, 0.45f);
		Pitch(crack, 900.0f, 240.0f);
		FM(crack, 2.3f, 1.1f);
		Env(crack, 0.001f, 0.0f, 0.10f, 0.0f, 0.015f, 3.0f);
		sound.layers.push_back(crack);

		Distortion(sound, 5.5f, 0.6f);
		Reverb(sound, 0.7f, 0.45f, 0.3f);
		return sound;
	}

	/// <summary>突進の踏み込み。地面を蹴った一発</summary>
	SoundDefinition DragonRushStep()
	{
		SoundDefinition sound = Sound("DragonRushStep", 90);

		SELayer stomp = Layer("Stomp", Wave::Sine, 0.45f, 1.0f);
		Pitch(stomp, 170.0f, 35.0f);
		Env(stomp, 0.002f, 0.01f, 0.40f, 0.0f, 0.04f, 2.3f);
		sound.layers.push_back(stomp);

		SELayer scrape = Layer("Scrape", Wave::PinkNoise, 0.3f, 0.55f);
		Pitch(scrape, 3000.0f, 500.0f);
		Env(scrape, 0.003f, 0.0f, 0.26f, 0.0f, 0.03f, 2.0f);
		Filt(scrape, Filter::LowPass, 3000.0f, 1.0f, 600.0f);
		sound.layers.push_back(scrape);

		Distortion(sound, 3.0f, 0.4f);
		return sound;
	}

	/// <summary>突進中のループ。走りながら風を切っている状態</summary>
	SoundDefinition DragonRushLoop()
	{
		SoundDefinition sound = Sound("DragonRushLoop", 80);

		// ループの継ぎ目で段差が出ないよう、頭と尻をどちらも 0 にする
		SELayer wind = Layer("Wind", Wave::PinkNoise, 0.8f, 0.85f);
		wind.frequency = 1400.0f;
		Env(wind, 0.2f, 0.0f, 0.0f, 1.0f, 0.2f, 1.4f);
		Filt(wind, Filter::BandPass, 700.0f, 1.1f);
		sound.layers.push_back(wind);

		SELayer body = Layer("Body", Wave::Sine, 0.8f, 0.5f);
		body.frequency = 52.0f;
		Env(body, 0.2f, 0.0f, 0.0f, 1.0f, 0.2f, 1.4f);
		sound.layers.push_back(body);

		return sound;
	}

	/// <summary>突進が止まったときの衝撃</summary>
	SoundDefinition DragonRushStop()
	{
		SoundDefinition sound = Sound("DragonRushStop", 92);

		SELayer skid = Layer("Skid", Wave::PinkNoise, 0.45f, 0.85f);
		Pitch(skid, 2400.0f, 350.0f);
		Env(skid, 0.01f, 0.0f, 0.38f, 0.0f, 0.05f, 1.8f);
		Filt(skid, Filter::BandPass, 1500.0f, 1.3f, 450.0f);
		sound.layers.push_back(skid);

		SELayer thud = Layer("Thud", Wave::Sine, 0.55f, 0.9f);
		Pitch(thud, 140.0f, 30.0f);
		Env(thud, 0.002f, 0.01f, 0.49f, 0.0f, 0.05f, 2.3f);
		sound.layers.push_back(thud);

		Distortion(sound, 3.5f, 0.45f);
		Reverb(sound, 0.55f, 0.5f, 0.24f);
		return sound;
	}

	// ── ブレス（溜め → 着火 → 炎のループ → 余韻）──

	/// <summary>ブレスの溜め。息を吸い込んで喉が熱くなっていく</summary>
	SoundDefinition DragonBreathCharge()
	{
		SoundDefinition sound = Sound("DragonBreathCharge", 95);

		// 吸い込み。ピッチを上げていくと「溜まっている」ことが伝わる
		SELayer inhale = Layer("Inhale", Wave::PinkNoise, 1.6f, 0.8f);
		Pitch(inhale, 400.0f, 2400.0f, Sweep::EaseIn);
		Env(inhale, 0.5f, 0.0f, 0.0f, 1.0f, 0.3f, 1.4f);
		Filt(inhale, Filter::BandPass, 600.0f, 1.6f, 2200.0f);
		sound.layers.push_back(inhale);

		// 喉の奥で唸っている低音
		SELayer throat = Layer("Throat", Wave::Saw, 1.6f, 0.5f);
		Pitch(throat, 70.0f, 140.0f, Sweep::EaseIn);
		FM(throat, 1.43f, 0.8f);
		Env(throat, 0.45f, 0.0f, 0.0f, 1.0f, 0.3f, 1.4f);
		Filt(throat, Filter::LowPass, 800.0f, 1.5f);
		sound.layers.push_back(throat);

		Distortion(sound, 2.5f, 0.35f);
		return sound;
	}

	/// <summary>着火。ボッと火が点く一瞬</summary>
	SoundDefinition DragonBreathIgnite()
	{
		SoundDefinition sound = Sound("DragonBreathIgnite", 98);

		// 点火の破裂。低いところへ一気に落とす
		SELayer burst = Layer("Burst", Wave::PinkNoise, 0.45f, 1.0f);
		Pitch(burst, 5000.0f, 300.0f);
		Env(burst, 0.004f, 0.0f, 0.39f, 0.0f, 0.05f, 2.0f);
		Filt(burst, Filter::LowPass, 5000.0f, 1.0f, 500.0f);
		sound.layers.push_back(burst);

		SELayer thump = Layer("Thump", Wave::Sine, 0.4f, 0.75f);
		Pitch(thump, 180.0f, 40.0f);
		Env(thump, 0.002f, 0.0f, 0.35f, 0.0f, 0.04f, 2.3f);
		sound.layers.push_back(thump);

		Distortion(sound, 4.0f, 0.5f);
		return sound;
	}

	/// <summary>吐き続けている間の炎。ループ</summary>
	SoundDefinition DragonBreathLoop()
	{
		SoundDefinition sound = Sound("DragonBreathLoop", 95);

		// 炎は「低いノイズ＋ざらついた中域」。帯域を分けて2枚重ねる
		SELayer roarBody = Layer("Body", Wave::PinkNoise, 0.7f, 0.9f);
		roarBody.frequency = 900.0f;
		Env(roarBody, 0.18f, 0.0f, 0.0f, 1.0f, 0.18f, 1.4f);
		Filt(roarBody, Filter::LowPass, 1100.0f, 1.2f);
		sound.layers.push_back(roarBody);

		SELayer hiss = Layer("Hiss", Wave::WhiteNoise, 0.7f, 0.45f);
		hiss.frequency = 6000.0f;
		hiss.noiseSeed = 2291;
		Env(hiss, 0.18f, 0.0f, 0.0f, 1.0f, 0.18f, 1.4f);
		Filt(hiss, Filter::BandPass, 2600.0f, 0.9f);
		sound.layers.push_back(hiss);

		SELayer sub = Layer("Sub", Wave::Sine, 0.7f, 0.5f);
		sub.frequency = 58.0f;
		Env(sub, 0.18f, 0.0f, 0.0f, 1.0f, 0.18f, 1.4f);
		sound.layers.push_back(sub);

		Distortion(sound, 2.5f, 0.35f);
		return sound;
	}

	/// <summary>ブレスが消える余韻。火が細くなって残り火がパチパチ言う</summary>
	SoundDefinition DragonBreathEnd()
	{
		SoundDefinition sound = Sound("DragonBreathEnd", 85);

		SELayer fade = Layer("Fade", Wave::PinkNoise, 0.9f, 0.8f);
		Pitch(fade, 1600.0f, 300.0f);
		Env(fade, 0.02f, 0.0f, 0.78f, 0.0f, 0.09f, 1.7f);
		Filt(fade, Filter::LowPass, 1800.0f, 1.0f, 350.0f);
		sound.layers.push_back(fade);

		// 残り火。短いノイズを散らす
		SELayer ember = Layer("Ember", Wave::WhiteNoise, 0.8f, 0.3f);
		ember.startDelay = 0.2f;
		Pitch(ember, 5000.0f, 2200.0f);
		ember.noiseSeed = 3312;
		Env(ember, 0.1f, 0.0f, 0.62f, 0.0f, 0.08f, 1.8f);
		Filt(ember, Filter::HighPass, 2200.0f, 0.9f);
		sound.layers.push_back(ember);

		Reverb(sound, 0.6f, 0.5f, 0.26f);
		return sound;
	}

	/// <summary>フェーズ移行の咆哮。通常の咆哮より高く長く、気配を変える</summary>
	SoundDefinition DragonPhaseRoar()
	{
		SoundDefinition sound = Sound("DragonPhaseRoar", 100);

		SELayer growl = Layer("Growl", Wave::Saw, 2.3f, 1.0f);
		Pitch(growl, 130.0f, 78.0f, Sweep::EaseOut);
		FM(growl, 1.73f, 1.2f);
		Env(growl, 0.1f, 0.4f, 1.4f, 0.0f, 0.35f, 1.4f);
		Filt(growl, Filter::LowPass, 2000.0f, 1.7f, 600.0f);
		sound.layers.push_back(growl);

		SELayer scream = Layer("Scream", Wave::Saw, 2.1f, 0.55f);
		Pitch(scream, 320.0f, 210.0f, Sweep::EaseOut);
		FM(scream, 2.91f, 1.6f);
		Env(scream, 0.12f, 0.35f, 1.3f, 0.0f, 0.3f, 1.4f);
		Filt(scream, Filter::BandPass, 1300.0f, 1.1f, 600.0f);
		sound.layers.push_back(scream);

		SELayer breath = Layer("Breath", Wave::PinkNoise, 2.2f, 0.4f);
		Pitch(breath, 3400.0f, 900.0f);
		Env(breath, 0.1f, 0.3f, 1.5f, 0.0f, 0.3f, 1.4f);
		Filt(breath, Filter::BandPass, 2000.0f, 0.9f, 700.0f);
		sound.layers.push_back(breath);

		SELayer sub = Layer("Sub", Wave::Sine, 2.0f, 0.7f);
		Pitch(sub, 52.0f, 32.0f);
		Env(sub, 0.08f, 0.5f, 1.2f, 0.0f, 0.22f, 1.5f);
		sound.layers.push_back(sub);

		Distortion(sound, 4.0f, 0.55f);
		Reverb(sound, 0.85f, 0.35f, 0.38f);
		return sound;
	}

	/// <summary>ブレイク（崩れ）。膝を突いて倒れ込む</summary>
	SoundDefinition DragonBreak()
	{
		SoundDefinition sound = Sound("DragonBreak", 95);

		// 力が抜けて落ちていく唸り
		SELayer groan = Layer("Groan", Wave::Saw, 1.3f, 0.8f);
		Pitch(groan, 120.0f, 45.0f, Sweep::Linear);
		FM(groan, 1.37f, 0.9f);
		Env(groan, 0.06f, 0.1f, 1.05f, 0.0f, 0.15f, 1.6f);
		Filt(groan, Filter::LowPass, 900.0f, 1.5f, 300.0f);
		sound.layers.push_back(groan);

		// 倒れ込んだ衝撃
		SELayer fall = Layer("Fall", Wave::Sine, 0.8f, 0.95f);
		fall.startDelay = 0.5f;
		Pitch(fall, 140.0f, 28.0f);
		Env(fall, 0.002f, 0.02f, 0.72f, 0.0f, 0.06f, 2.3f);
		sound.layers.push_back(fall);

		SELayer dust = Layer("Dust", Wave::PinkNoise, 0.6f, 0.5f);
		dust.startDelay = 0.5f;
		Pitch(dust, 2800.0f, 300.0f);
		Env(dust, 0.004f, 0.0f, 0.53f, 0.0f, 0.06f, 2.0f);
		Filt(dust, Filter::LowPass, 2800.0f, 1.0f, 350.0f);
		sound.layers.push_back(dust);

		Distortion(sound, 3.0f, 0.4f);
		Reverb(sound, 0.7f, 0.45f, 0.3f);
		return sound;
	}

	/// <summary>被弾。鱗を斬られた鈍い手応え</summary>
	SoundDefinition DragonHit()
	{
		SoundDefinition sound = Sound("DragonHit", 55);

		SELayer thud = Layer("Thud", Wave::Sine, 0.26f, 0.9f);
		Pitch(thud, 200.0f, 55.0f);
		Env(thud, 0.001f, 0.0f, 0.23f, 0.0f, 0.025f, 2.5f);
		sound.layers.push_back(thud);

		SELayer scale = Layer("Scale", Wave::PinkNoise, 0.16f, 0.55f);
		Pitch(scale, 2600.0f, 500.0f);
		Env(scale, 0.001f, 0.0f, 0.14f, 0.0f, 0.018f, 2.8f);
		Filt(scale, Filter::BandPass, 1400.0f, 1.3f, 500.0f);
		sound.layers.push_back(scale);

		// 硬いものを叩いた芯
		SELayer knock = Layer("Knock", Wave::Sine, 0.14f, 0.35f);
		knock.frequency = 620.0f;
		FM(knock, 3.3f, 1.0f);
		Env(knock, 0.001f, 0.0f, 0.12f, 0.0f, 0.015f, 3.0f);
		sound.layers.push_back(knock);

		return sound;
	}

	/// <summary>紫の火花で弾いた。攻撃が通っていないことを伝える鋭い金属音</summary>
	SoundDefinition DragonArmorSpark()
	{
		SoundDefinition sound = Sound("DragonArmorSpark", 72);

		SELayer clang = Layer("Clang", Wave::Sine, 0.35f, 0.8f);
		clang.frequency = 2600.0f;
		FM(clang, 4.7f, 1.7f);
		Env(clang, 0.001f, 0.0f, 0.31f, 0.0f, 0.03f, 3.2f);
		sound.layers.push_back(clang);

		SELayer spark = Layer("Spark", Wave::WhiteNoise, 0.09f, 0.55f);
		Pitch(spark, 12000.0f, 3200.0f);
		Env(spark, 0.001f, 0.0f, 0.08f, 0.0f, 0.01f, 3.0f);
		Filt(spark, Filter::HighPass, 2400.0f, 0.8f);
		sound.layers.push_back(spark);

		// 弾かれて詰まった低音
		SELayer block = Layer("Block", Wave::Sine, 0.13f, 0.4f);
		Pitch(block, 380.0f, 150.0f);
		Env(block, 0.001f, 0.0f, 0.11f, 0.0f, 0.015f, 3.0f);
		sound.layers.push_back(block);

		Reverb(sound, 0.4f, 0.45f, 0.24f);
		return sound;
	}

	/// <summary>死亡。断末魔から崩れ落ちるまで</summary>
	SoundDefinition DragonDeath()
	{
		SoundDefinition sound = Sound("DragonDeath", 100);

		// 断末魔。咆哮より下がっていく形にする
		SELayer scream = Layer("Scream", Wave::Saw, 1.8f, 1.0f);
		Pitch(scream, 200.0f, 55.0f, Sweep::Exponential);
		FM(scream, 2.11f, 1.4f);
		Env(scream, 0.06f, 0.2f, 1.4f, 0.0f, 0.22f, 1.5f);
		Filt(scream, Filter::LowPass, 1800.0f, 1.6f, 400.0f);
		sound.layers.push_back(scream);

		SELayer breath = Layer("Breath", Wave::PinkNoise, 1.6f, 0.45f);
		Pitch(breath, 3000.0f, 500.0f);
		Env(breath, 0.05f, 0.15f, 1.2f, 0.0f, 0.2f, 1.5f);
		Filt(breath, Filter::BandPass, 1800.0f, 0.9f, 500.0f);
		sound.layers.push_back(breath);

		// 倒れた衝撃
		SELayer collapse = Layer("Collapse", Wave::Sine, 1.2f, 0.95f);
		collapse.startDelay = 1.1f;
		Pitch(collapse, 130.0f, 22.0f);
		Env(collapse, 0.002f, 0.03f, 1.1f, 0.0f, 0.08f, 2.2f);
		sound.layers.push_back(collapse);

		SELayer rubble = Layer("Rubble", Wave::PinkNoise, 0.9f, 0.55f);
		rubble.startDelay = 1.1f;
		Pitch(rubble, 3200.0f, 200.0f);
		Env(rubble, 0.005f, 0.0f, 0.82f, 0.0f, 0.08f, 2.0f);
		Filt(rubble, Filter::LowPass, 3200.0f, 1.0f, 250.0f);
		sound.layers.push_back(rubble);

		Distortion(sound, 4.5f, 0.55f);
		Reverb(sound, 0.85f, 0.4f, 0.4f);
		return sound;
	}

} // namespace GameSoundDefs
