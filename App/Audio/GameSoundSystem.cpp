#include "GameSoundBuilders.h"

// 戦闘まわりの仕組み・UI・環境音。Sound.txt の「5.」「6.」「7. 環境音」に対応する。
//
// UI の音は「短く・低音を入れない・毎回同じに聞こえる」の3つを守る。
// カーソル移動のように連打される音に低音や残響を入れると、すぐ耳障りになる。

using namespace GameSoundBuild;

namespace GameSoundDefs {

	// ───────────────────────────── 戦闘まわりの仕組み ─────────────────────────────

	/// <summary>
	/// 赤い予兆マーカーが出たときの警告。
	/// 予兆を見て避けるゲームなので、視界の外で出ても気づけるように鋭くしてある
	/// </summary>
	SoundDefinition AttackWarning()
	{
		SoundDefinition sound = Sound("AttackWarning", 88);

		// 不協和な2音。濁らせると「危険」の合図として聞き分けやすい
		SELayer high = Layer("Alarm", Wave::Square, 0.22f, 0.7f);
		high.frequency = 1180.0f;
		high.pulseWidth = 0.3f;
		Env(high, 0.004f, 0.06f, 0.12f, 0.0f, 0.03f, 2.0f);
		Filt(high, Filter::LowPass, 3600.0f, 1.0f);
		sound.layers.push_back(high);

		SELayer beat = Layer("Beat", Wave::Square, 0.22f, 0.45f);
		beat.frequency = 1570.0f;   // 高音と不協和にして、うねりで目立たせる
		beat.pulseWidth = 0.35f;
		Env(beat, 0.004f, 0.05f, 0.12f, 0.0f, 0.03f, 2.0f);
		Filt(beat, Filter::LowPass, 3600.0f, 1.0f);
		sound.layers.push_back(beat);

		return sound;
	}

	/// <summary>ランクアップ。短く上がる3音</summary>
	SoundDefinition RankUp()
	{
		SoundDefinition sound = Sound("RankUp", 70);

		// 完全5度→オクターブ。上がっていく感じが分かりやすい並び
		const float notes[3] = { 880.0f, 1320.0f, 1760.0f };
		const char* names[3] = { "Note1", "Note2", "Note3" };
		for (int i = 0; i < 3; ++i) {
			SELayer note = Layer(names[i], Wave::Sine, (i == 2) ? 0.30f : 0.12f, 0.6f);
			note.startDelay = 0.065f * static_cast<float>(i);
			note.frequency = notes[i];
			FM(note, 2.0f, 0.25f);
			Env(note, 0.003f, 0.01f, (i == 2) ? 0.26f : 0.10f, 0.0f, 0.02f, 2.4f);
			sound.layers.push_back(note);
		}

		Reverb(sound, 0.45f, 0.4f, 0.24f);
		return sound;
	}

	/// <summary>ランクダウン。上がる並びの逆。最後を濁らせて落ちた感じにする</summary>
	SoundDefinition RankDown()
	{
		SoundDefinition sound = Sound("RankDown", 70);

		const float notes[3] = { 1320.0f, 990.0f, 660.0f };
		const char* names[3] = { "Note1", "Note2", "Note3" };
		for (int i = 0; i < 3; ++i) {
			SELayer note = Layer(names[i], Wave::Sine, (i == 2) ? 0.28f : 0.11f, 0.5f);
			note.startDelay = 0.06f * static_cast<float>(i);
			note.frequency = notes[i];
			FM(note, 1.7f, 0.35f + 0.25f * static_cast<float>(i));
			Env(note, 0.004f, 0.0f, (i == 2) ? 0.24f : 0.09f, 0.0f, 0.02f, 2.2f);
			sound.layers.push_back(note);
		}

		Filt(sound.layers.back(), Filter::LowPass, 1800.0f, 1.0f);
		return sound;
	}

	/// <summary>ボスHPバーの登場。低いところから立ち上がる</summary>
	SoundDefinition BossBarAppear()
	{
		SoundDefinition sound = Sound("BossBarAppear", 80);

		SELayer swell = Layer("Swell", Wave::Saw, 0.9f, 0.7f);
		Pitch(swell, 90.0f, 220.0f, Sweep::EaseIn);
		FM(swell, 1.5f, 0.4f);
		Env(swell, 0.4f, 0.0f, 0.42f, 0.0f, 0.07f, 1.6f);
		Filt(swell, Filter::LowPass, 700.0f, 1.6f, 1400.0f);
		sound.layers.push_back(swell);

		// バーが出きった合図
		SELayer hit = Layer("Set", Wave::Sine, 0.35f, 0.55f);
		hit.startDelay = 0.55f;
		hit.frequency = 520.0f;
		FM(hit, 2.6f, 0.9f);
		Env(hit, 0.002f, 0.0f, 0.31f, 0.0f, 0.03f, 3.0f);
		sound.layers.push_back(hit);

		Reverb(sound, 0.55f, 0.45f, 0.26f);
		return sound;
	}

	/// <summary>フェーズ移行。バーの色が変わる瞬間に重ねる</summary>
	SoundDefinition BossBarPhase()
	{
		SoundDefinition sound = Sound("BossBarPhase", 85);

		SELayer impact = Layer("Impact", Wave::Sine, 0.5f, 0.85f);
		Pitch(impact, 300.0f, 60.0f);
		Env(impact, 0.002f, 0.0f, 0.45f, 0.0f, 0.04f, 2.4f);
		sound.layers.push_back(impact);

		SELayer shine = Layer("Shine", Wave::Sine, 0.45f, 0.5f);
		Pitch(shine, 900.0f, 1700.0f, Sweep::EaseOut);
		FM(shine, 2.3f, 0.8f);
		Env(shine, 0.005f, 0.0f, 0.40f, 0.0f, 0.04f, 2.6f);
		sound.layers.push_back(shine);

		Distortion(sound, 2.5f, 0.35f);
		Reverb(sound, 0.5f, 0.4f, 0.28f);
		return sound;
	}

	/// <summary>削り切ったとき。バーが空になる</summary>
	SoundDefinition BossBarDeplete()
	{
		SoundDefinition sound = Sound("BossBarDeplete", 90);

		// ガラスが割れるように落とす
		SELayer shatter = Layer("Shatter", Wave::WhiteNoise, 0.5f, 0.85f);
		Pitch(shatter, 9000.0f, 1200.0f);
		Env(shatter, 0.002f, 0.0f, 0.44f, 0.0f, 0.05f, 2.4f);
		Filt(shatter, Filter::BandPass, 3000.0f, 1.2f, 900.0f);
		sound.layers.push_back(shatter);

		SELayer drop = Layer("Drop", Wave::Sine, 0.6f, 0.8f);
		Pitch(drop, 420.0f, 45.0f);
		Env(drop, 0.003f, 0.0f, 0.54f, 0.0f, 0.05f, 2.2f);
		sound.layers.push_back(drop);

		Reverb(sound, 0.6f, 0.45f, 0.3f);
		return sound;
	}

	/// <summary>強制戦闘エリアの光の壁が出る</summary>
	SoundDefinition BattleWallOn()
	{
		SoundDefinition sound = Sound("BattleWallOn", 80);

		// 立ち上がる結界。上がるピッチ＋うなり
		SELayer rise = Layer("Rise", Wave::Saw, 1.0f, 0.7f);
		Pitch(rise, 120.0f, 480.0f, Sweep::EaseIn);
		FM(rise, 1.41f, 0.6f);
		Env(rise, 0.35f, 0.0f, 0.5f, 0.0f, 0.08f, 1.6f);
		Filt(rise, Filter::BandPass, 400.0f, 2.2f, 1200.0f);
		sound.layers.push_back(rise);

		SELayer seal = Layer("Seal", Wave::Sine, 0.6f, 0.5f);
		seal.startDelay = 0.55f;
		seal.frequency = 660.0f;
		FM(seal, 2.76f, 1.0f);
		Env(seal, 0.004f, 0.0f, 0.54f, 0.0f, 0.05f, 2.8f);
		sound.layers.push_back(seal);

		Reverb(sound, 0.6f, 0.4f, 0.3f);
		return sound;
	}

	/// <summary>光の壁が消える。出るときの逆</summary>
	SoundDefinition BattleWallOff()
	{
		SoundDefinition sound = Sound("BattleWallOff", 75);

		SELayer fall = Layer("Fall", Wave::Saw, 0.8f, 0.6f);
		Pitch(fall, 480.0f, 110.0f);
		FM(fall, 1.41f, 0.5f);
		Env(fall, 0.02f, 0.0f, 0.70f, 0.0f, 0.07f, 1.8f);
		Filt(fall, Filter::BandPass, 1200.0f, 2.0f, 380.0f);
		sound.layers.push_back(fall);

		SELayer air = Layer("Air", Wave::WhiteNoise, 0.5f, 0.3f);
		Pitch(air, 3000.0f, 700.0f);
		Env(air, 0.01f, 0.0f, 0.44f, 0.0f, 0.045f, 2.0f);
		Filt(air, Filter::HighPass, 900.0f, 0.9f);
		sound.layers.push_back(air);

		Reverb(sound, 0.5f, 0.5f, 0.24f);
		return sound;
	}

	/// <summary>敵の出現イベント。これから湧く合図</summary>
	SoundDefinition EnemySpawnEvent()
	{
		SoundDefinition sound = Sound("EnemySpawnEvent", 78);

		SELayer drone = Layer("Drone", Wave::Saw, 0.8f, 0.65f);
		Pitch(drone, 180.0f, 90.0f);
		FM(drone, 1.37f, 0.8f);
		Env(drone, 0.12f, 0.0f, 0.60f, 0.0f, 0.07f, 1.7f);
		Filt(drone, Filter::LowPass, 900.0f, 1.8f, 400.0f);
		sound.layers.push_back(drone);

		SELayer sting = Layer("Sting", Wave::Sine, 0.4f, 0.45f);
		Pitch(sting, 1400.0f, 700.0f);
		FM(sting, 3.1f, 1.1f);
		Env(sting, 0.003f, 0.0f, 0.36f, 0.0f, 0.035f, 2.8f);
		sound.layers.push_back(sting);

		Reverb(sound, 0.55f, 0.45f, 0.28f);
		return sound;
	}

	// ───────────────────────────── UI・メニュー ─────────────────────────────

	/// <summary>カーソル移動。いちばん連打されるので短く軽く</summary>
	SoundDefinition UICursor()
	{
		SoundDefinition sound = Sound("UICursor", 35);

		SELayer tick = Layer("Tick", Wave::Sine, 0.05f, 0.5f);
		tick.frequency = 1480.0f;
		Env(tick, 0.001f, 0.005f, 0.035f, 0.0f, 0.008f, 2.6f);
		sound.layers.push_back(tick);

		// 芯を少し足して「コッ」と鳴らす
		SELayer body = Layer("Body", Wave::Triangle, 0.045f, 0.25f);
		body.frequency = 740.0f;
		Env(body, 0.001f, 0.0f, 0.035f, 0.0f, 0.008f, 2.6f);
		sound.layers.push_back(body);

		return sound;
	}

	/// <summary>ポーズを開く。時間が止まる感じに下げる</summary>
	SoundDefinition UIPauseOpen()
	{
		SoundDefinition sound = Sound("UIPauseOpen", 60);

		SELayer down = Layer("Down", Wave::Sine, 0.28f, 0.6f);
		Pitch(down, 900.0f, 320.0f, Sweep::EaseOut);
		FM(down, 1.5f, 0.3f);
		Env(down, 0.005f, 0.0f, 0.25f, 0.0f, 0.025f, 2.2f);
		sound.layers.push_back(down);

		SELayer air = Layer("Air", Wave::WhiteNoise, 0.22f, 0.25f);
		Pitch(air, 4000.0f, 1200.0f);
		Env(air, 0.004f, 0.0f, 0.19f, 0.0f, 0.02f, 2.2f);
		Filt(air, Filter::HighPass, 1200.0f, 0.9f);
		sound.layers.push_back(air);

		return sound;
	}

	/// <summary>ポーズを閉じる。開くときの逆</summary>
	SoundDefinition UIPauseClose()
	{
		SoundDefinition sound = Sound("UIPauseClose", 60);

		SELayer up = Layer("Up", Wave::Sine, 0.24f, 0.6f);
		Pitch(up, 320.0f, 900.0f, Sweep::EaseOut);
		FM(up, 1.5f, 0.3f);
		Env(up, 0.005f, 0.0f, 0.21f, 0.0f, 0.025f, 2.2f);
		sound.layers.push_back(up);

		SELayer air = Layer("Air", Wave::WhiteNoise, 0.18f, 0.25f);
		Pitch(air, 1200.0f, 4000.0f, Sweep::EaseOut);
		Env(air, 0.004f, 0.0f, 0.15f, 0.0f, 0.02f, 2.2f);
		Filt(air, Filter::HighPass, 1200.0f, 0.9f);
		sound.layers.push_back(air);

		return sound;
	}

	/// <summary>音量スライダー。1目盛りごとに鳴るので、カーソルより更に軽く</summary>
	SoundDefinition UISlider()
	{
		SoundDefinition sound = Sound("UISlider", 30);

		SELayer tick = Layer("Tick", Wave::Triangle, 0.035f, 0.4f);
		tick.frequency = 2100.0f;
		Env(tick, 0.001f, 0.004f, 0.024f, 0.0f, 0.005f, 2.8f);
		sound.layers.push_back(tick);

		return sound;
	}

	/// <summary>確認ダイアログが開く</summary>
	SoundDefinition UIDialogOpen()
	{
		SoundDefinition sound = Sound("UIDialogOpen", 55);

		SELayer open = Layer("Open", Wave::Sine, 0.22f, 0.55f);
		Pitch(open, 480.0f, 760.0f, Sweep::EaseOut);
		FM(open, 2.0f, 0.3f);
		Env(open, 0.006f, 0.0f, 0.19f, 0.0f, 0.02f, 2.2f);
		sound.layers.push_back(open);

		SELayer sub = Layer("Sub", Wave::Sine, 0.18f, 0.3f);
		Pitch(sub, 220.0f, 140.0f);
		Env(sub, 0.004f, 0.0f, 0.15f, 0.0f, 0.02f, 2.4f);
		sound.layers.push_back(sub);

		return sound;
	}

	/// <summary>「はい」。決定より少し重く、踏み切った感じにする</summary>
	SoundDefinition UIDialogYes()
	{
		SoundDefinition sound = Sound("UIDialogYes", 55);

		SELayer tone = Layer("Tone", Wave::Sine, 0.2f, 0.65f);
		Pitch(tone, 620.0f, 1240.0f, Sweep::EaseOut);
		FM(tone, 2.0f, 0.25f);
		Env(tone, 0.004f, 0.01f, 0.17f, 0.0f, 0.02f, 2.2f);
		sound.layers.push_back(tone);

		SELayer body = Layer("Body", Wave::Sine, 0.16f, 0.35f);
		Pitch(body, 310.0f, 200.0f);
		Env(body, 0.003f, 0.0f, 0.14f, 0.0f, 0.018f, 2.4f);
		sound.layers.push_back(body);

		return sound;
	}

	/// <summary>「いいえ」。下がって終わる</summary>
	SoundDefinition UIDialogNo()
	{
		SoundDefinition sound = Sound("UIDialogNo", 55);

		SELayer tone = Layer("Tone", Wave::Sine, 0.2f, 0.55f);
		Pitch(tone, 620.0f, 290.0f, Sweep::EaseOut);
		Env(tone, 0.004f, 0.0f, 0.18f, 0.0f, 0.02f, 2.2f);
		sound.layers.push_back(tone);

		return sound;
	}

	/// <summary>タイトルで PRESS A BUTTON を押した。始まる合図なので大きめに</summary>
	SoundDefinition TitlePress()
	{
		SoundDefinition sound = Sound("TitlePress", 80);

		SELayer strike = Layer("Strike", Wave::Sine, 0.7f, 0.85f);
		strike.frequency = 880.0f;
		FM(strike, 2.76f, 1.1f);
		Env(strike, 0.002f, 0.0f, 0.64f, 0.0f, 0.06f, 3.0f);
		sound.layers.push_back(strike);

		SELayer sub = Layer("Sub", Wave::Sine, 0.5f, 0.6f);
		Pitch(sub, 220.0f, 60.0f);
		Env(sub, 0.003f, 0.0f, 0.45f, 0.0f, 0.045f, 2.3f);
		sound.layers.push_back(sub);

		SELayer air = Layer("Air", Wave::WhiteNoise, 0.3f, 0.3f);
		Pitch(air, 8000.0f, 2000.0f);
		Env(air, 0.002f, 0.0f, 0.26f, 0.0f, 0.03f, 2.6f);
		Filt(air, Filter::HighPass, 1800.0f, 0.9f);
		sound.layers.push_back(air);

		Reverb(sound, 0.6f, 0.4f, 0.32f);
		return sound;
	}

	/// <summary>チュートリアルの説明が出た</summary>
	SoundDefinition TutorialShow()
	{
		SoundDefinition sound = Sound("TutorialShow", 50);

		SELayer note = Layer("Note", Wave::Sine, 0.22f, 0.5f);
		Pitch(note, 700.0f, 1050.0f, Sweep::EaseOut);
		FM(note, 2.0f, 0.2f);
		Env(note, 0.006f, 0.01f, 0.18f, 0.0f, 0.02f, 2.2f);
		sound.layers.push_back(note);

		return sound;
	}

	/// <summary>チュートリアルの1項目をクリアした。上がる2音</summary>
	SoundDefinition TutorialClear()
	{
		SoundDefinition sound = Sound("TutorialClear", 55);

		SELayer first = Layer("Note1", Wave::Sine, 0.1f, 0.55f);
		first.frequency = 1050.0f;
		Env(first, 0.003f, 0.01f, 0.08f, 0.0f, 0.015f, 2.4f);
		sound.layers.push_back(first);

		SELayer second = Layer("Note2", Wave::Sine, 0.3f, 0.55f);
		second.startDelay = 0.085f;
		second.frequency = 1575.0f;
		FM(second, 2.0f, 0.25f);
		Env(second, 0.003f, 0.01f, 0.26f, 0.0f, 0.025f, 2.6f);
		sound.layers.push_back(second);

		Reverb(sound, 0.4f, 0.45f, 0.2f);
		return sound;
	}

	/// <summary>ステージ開始の演出</summary>
	SoundDefinition StageStart()
	{
		SoundDefinition sound = Sound("StageStart", 85);

		// 低いところから立ち上げて、最後に決める
		SELayer swell = Layer("Swell", Wave::Saw, 0.85f, 0.6f);
		Pitch(swell, 70.0f, 200.0f, Sweep::EaseIn);
		FM(swell, 1.5f, 0.4f);
		Env(swell, 0.6f, 0.0f, 0.2f, 0.0f, 0.05f, 1.6f);
		Filt(swell, Filter::LowPass, 500.0f, 1.8f, 1100.0f);
		sound.layers.push_back(swell);

		SELayer hit = Layer("Hit", Wave::Sine, 0.8f, 0.9f);
		hit.startDelay = 0.7f;
		Pitch(hit, 260.0f, 45.0f);
		Env(hit, 0.002f, 0.01f, 0.72f, 0.0f, 0.06f, 2.3f);
		sound.layers.push_back(hit);

		SELayer shine = Layer("Shine", Wave::Sine, 0.6f, 0.45f);
		shine.startDelay = 0.7f;
		shine.frequency = 1320.0f;
		FM(shine, 2.76f, 0.9f);
		Env(shine, 0.002f, 0.0f, 0.55f, 0.0f, 0.05f, 3.0f);
		sound.layers.push_back(shine);

		Distortion(sound, 2.5f, 0.3f);
		Reverb(sound, 0.7f, 0.4f, 0.35f);
		return sound;
	}

	/// <summary>YOU DIED の表示。重く沈む</summary>
	SoundDefinition GameOver()
	{
		SoundDefinition sound = Sound("GameOver", 95);

		SELayer toll = Layer("Toll", Wave::Sine, 2.2f, 0.9f);
		toll.frequency = 110.0f;
		FM(toll, 2.76f, 1.3f);
		Env(toll, 0.004f, 0.0f, 2.0f, 0.0f, 0.18f, 2.2f);
		sound.layers.push_back(toll);

		SELayer sub = Layer("Sub", Wave::Sine, 1.8f, 0.7f);
		Pitch(sub, 60.0f, 32.0f);
		Env(sub, 0.006f, 0.05f, 1.6f, 0.0f, 0.15f, 2.0f);
		sound.layers.push_back(sub);

		// 沈んでいく倍音
		SELayer fall = Layer("Fall", Wave::Saw, 1.6f, 0.35f);
		fall.startDelay = 0.2f;
		Pitch(fall, 330.0f, 90.0f, Sweep::Linear);
		FM(fall, 1.41f, 0.7f);
		Env(fall, 0.1f, 0.0f, 1.4f, 0.0f, 0.12f, 1.6f);
		Filt(fall, Filter::LowPass, 1200.0f, 1.4f, 400.0f);
		sound.layers.push_back(fall);

		Reverb(sound, 0.85f, 0.35f, 0.4f);
		return sound;
	}

	/// <summary>スコアのカウントアップ。連続で鳴るので極端に短く</summary>
	SoundDefinition ScoreCount()
	{
		SoundDefinition sound = Sound("ScoreCount", 30);

		SELayer tick = Layer("Tick", Wave::Triangle, 0.03f, 0.4f);
		tick.frequency = 2640.0f;
		Env(tick, 0.001f, 0.003f, 0.02f, 0.0f, 0.005f, 3.0f);
		sound.layers.push_back(tick);

		return sound;
	}

	/// <summary>クリア画面でランクが出る</summary>
	SoundDefinition RankReveal()
	{
		SoundDefinition sound = Sound("RankReveal", 80);

		// 決まった感じの和音。同時に鳴らす
		const float notes[3] = { 660.0f, 990.0f, 1320.0f };
		const char* names[3] = { "Chord1", "Chord2", "Chord3" };
		for (int i = 0; i < 3; ++i) {
			SELayer note = Layer(names[i], Wave::Sine, 0.9f, 0.55f - 0.08f * static_cast<float>(i));
			note.frequency = notes[i];
			note.pan = -0.25f + 0.25f * static_cast<float>(i);
			FM(note, 2.0f, 0.3f);
			Env(note, 0.004f, 0.02f, 0.82f, 0.0f, 0.07f, 2.6f);
			sound.layers.push_back(note);
		}

		SELayer shine = Layer("Shine", Wave::WhiteNoise, 0.35f, 0.3f);
		Pitch(shine, 9000.0f, 3000.0f);
		Env(shine, 0.004f, 0.0f, 0.30f, 0.0f, 0.035f, 2.4f);
		Filt(shine, Filter::HighPass, 2600.0f, 0.9f);
		sound.layers.push_back(shine);

		Reverb(sound, 0.65f, 0.4f, 0.32f);
		return sound;
	}

	/// <summary>シーン遷移の暗転。すーっと落ちる</summary>
	SoundDefinition SceneTransition()
	{
		SoundDefinition sound = Sound("SceneTransition", 65);

		SELayer sweep = Layer("Sweep", Wave::WhiteNoise, 0.65f, 0.7f);
		Pitch(sweep, 5000.0f, 500.0f);
		Env(sweep, 0.05f, 0.0f, 0.53f, 0.0f, 0.06f, 1.8f);
		Filt(sweep, Filter::BandPass, 2600.0f, 1.0f, 500.0f);
		sound.layers.push_back(sweep);

		SELayer sub = Layer("Sub", Wave::Sine, 0.5f, 0.45f);
		Pitch(sub, 180.0f, 45.0f);
		Env(sub, 0.03f, 0.0f, 0.42f, 0.0f, 0.045f, 2.0f);
		sound.layers.push_back(sub);

		return sound;
	}

	// ───────────────────────────── 環境音 ─────────────────────────────

	/// <summary>
	/// ダンジョンの空気。風と反響。ループ。
	///
	/// ずっと鳴っているものなので、耳に残る成分（はっきりしたピッチ・高い倍音）を入れない。
	/// 低いノイズだけで「広い場所にいる」を出す。
	///
	/// **ループ用の作りにしてあるので、触るときは2つ気をつけること。**
	///   1. リバーブを掛けない。尾がレイヤーの長さより後ろへ伸びて、
	///      継ぎ目に無音が挟まる（`SoundDefinition::GetTotalDuration` が尾のぶん伸ばすため）
	///   2. 全体を長くして、立ち上がりと終わりを相対的に短くする。
	///      合成器は必ず頭と尻を 0 にするので、短いループだとそこが周期的な脈になって聞こえる
	/// </summary>
	SoundDefinition AmbienceDungeon()
	{
		SoundDefinition sound = Sound("AmbienceDungeon", 10);
		// 環境音は常に一定の音量で鳴らしたいので、ピークを揃えない
		sound.normalize = false;
		sound.masterVolume = 0.85f;

		constexpr float kLoop = 6.0f;

		// 低い風。6秒のうち 4.8 秒は一定なので、継ぎ目は風が凪ぐ程度にしか聞こえない
		SELayer wind = Layer("Wind", Wave::PinkNoise, kLoop, 0.55f);
		wind.frequency = 320.0f;
		Env(wind, 0.6f, 0.0f, 0.0f, 1.0f, 0.6f, 1.3f);
		Filt(wind, Filter::LowPass, 420.0f, 0.9f);
		sound.layers.push_back(wind);

		// 反響の芯。うっすら低音を足すと空間の広さが出る
		SELayer hollow = Layer("Hollow", Wave::Sine, kLoop, 0.22f);
		hollow.frequency = 58.0f;
		Env(hollow, 0.7f, 0.0f, 0.0f, 1.0f, 0.7f, 1.3f);
		sound.layers.push_back(hollow);

		// 遠くの空気の流れ。風と違うシードにして重なりを自然にする
		SELayer draft = Layer("Draft", Wave::PinkNoise, kLoop, 0.2f);
		draft.frequency = 900.0f;
		draft.noiseSeed = 8821;
		draft.pan = 0.35f;
		Env(draft, 0.8f, 0.0f, 0.0f, 1.0f, 0.8f, 1.3f);
		Filt(draft, Filter::BandPass, 700.0f, 0.8f);
		sound.layers.push_back(draft);

		return sound;
	}

	/// <summary>
	/// 松明のパチパチ。ループ。
	/// 炎の「ゴー」という低い成分に、弾ける音を不規則に散らす。
	/// ループの作法は <see cref="AmbienceDungeon"/> と同じ
	/// </summary>
	SoundDefinition TorchCrackle()
	{
		SoundDefinition sound = Sound("TorchCrackle", 10);
		sound.normalize = false;
		sound.masterVolume = 0.8f;

		constexpr float kLoop = 4.0f;

		SELayer flame = Layer("Flame", Wave::PinkNoise, kLoop, 0.45f);
		flame.frequency = 700.0f;
		Env(flame, 0.3f, 0.0f, 0.0f, 1.0f, 0.3f, 1.3f);
		Filt(flame, Filter::LowPass, 900.0f, 1.0f);
		sound.layers.push_back(flame);

		// 弾ける音。割り切れない間隔で置くと、繰り返しに聞こえにくい
		const float pops[8] = { 0.13f, 0.47f, 0.89f, 1.21f, 1.63f, 2.27f, 2.71f, 3.41f };
		const float volumes[8] = { 0.5f, 0.35f, 0.55f, 0.3f, 0.45f, 0.5f, 0.28f, 0.42f };
		const char* names[8] = { "Pop1", "Pop2", "Pop3", "Pop4", "Pop5", "Pop6", "Pop7", "Pop8" };
		for (int i = 0; i < 8; ++i) {
			SELayer pop = Layer(names[i], Wave::WhiteNoise, 0.05f, volumes[i]);
			pop.startDelay = pops[i];
			Pitch(pop, 6000.0f - 400.0f * i, 1800.0f);
			pop.noiseSeed = 1301 + i * 977;
			Env(pop, 0.001f, 0.0f, 0.04f, 0.0f, 0.008f, 3.0f);
			Filt(pop, Filter::BandPass, 2600.0f, 1.4f);
			sound.layers.push_back(pop);
		}

		return sound;
	}

} // namespace GameSoundDefs
