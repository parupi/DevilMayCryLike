#pragma once
#include <Math/Vector2.h>
#include <Math/Vector4.h>
#include <array>
#include <cstdint>
#include <random>

class Sprite;

/// <summary>
/// プレイヤーのHPバー（画面左上）。ハートを並べていた頃の置き換え。
///
/// 見た目はボスのHPバー（BossHealthBar）と同じ作りを小さくしたもの。
/// 画像を使わず white.png を重ねて組んでいるので、色と大きさはこのクラスの定数だけで決まる。
///   背景 / 遅れて減る白いゲージ / HPゲージ / 金属のフレーム ＋ 左端の菱形の飾り
///
/// 演出:
///   被弾   … HPゲージは即座に減り、白ゲージが少し遅れて追いかける。赤いフラッシュ・横揺れ・削れた位置から火花
///   瀕死   … 最大HPの kLowHealthRatio 以下になると、ゲージが赤く脈打つ（心音と同じ境目）
///   回復   … 演出なしでそのまま合わせる（トレーニングの全回復など）
///
/// 持ち主は Player。スプライトは SpriteManager が生成順に描くので、ポーズの暗幕より先に作ること
/// （Player::Initialize で作れば、今までのハートと同じ順番になる）。
/// </summary>
class PlayerHealthBar
{
public:
	/// <summary>瀕死とみなすHPの割合。心音（Player::UpdateMovementSound）もこれを見る</summary>
	static constexpr float kLowHealthRatio = 0.25f;

	void Initialize(int32_t maxHp);

	/// <summary>毎フレーム呼ぶ</summary>
	/// <param name="hp">今のHP</param>
	/// <param name="maxHp">最大HP</param>
	/// <param name="alpha">全体の濃さ。死亡演出中にフェードアウトさせる（Player の hudAlpha_）</param>
	void Update(int32_t hp, int32_t maxHp, float alpha);

private:
	/// <summary>火花1粒。削れた位置から飛び散る</summary>
	struct Spark {
		Sprite* sprite = nullptr;
		Vector2 position{};
		Vector2 velocity{};
		float life = 0.0f;
		float maxLife = 0.0f;
	};

	void OnDamaged(float previousRatio, float newRatio);
	void EmitSparks(float x, float y, int32_t count);
	void UpdateDelayGauge(float dt);
	void UpdateSparks(float dt);
	void ApplyLayout(float alpha);
	Sprite* CreateRect(const char* name, const char* texture = "white.png");

private:
	// ==========================
	// レイアウト（画面 1280x720、左上）
	// ==========================
	static constexpr float kLeft = 44.0f;          // ゲージの左端
	static constexpr float kBarY = 40.0f;          // バーの中心の高さ
	static constexpr float kBarWidth = 300.0f;
	static constexpr float kBarHeight = 12.0f;
	static constexpr float kFramePad = 3.0f;       // フレームの太さ

	// ==========================
	// 演出の時間（ボスのHPバーと揃えてある）
	// ==========================
	static constexpr float kDelayHoldTime = 0.15f;   // 白ゲージが動き出すまで。被弾量が読めるよう、ボスより少し長く残す
	static constexpr float kDelayCatchUpTime = 0.35f;// 白ゲージが追いつくまで
	static constexpr float kHitFlashTime = 0.12f;
	static constexpr float kShakeTime = 0.18f;
	static constexpr float kShakeAmplitude = 5.0f;   // px
	static constexpr float kLowHealthPulseSpeed = 6.0f;

	static constexpr int32_t kSparkCount = 10;

	// ==========================
	// スプライト（作った順＝描く順）
	// ==========================
	Sprite* shadow_ = nullptr;          // フレームの落ち影
	Sprite* frame_ = nullptr;           // 金属のフレーム（暗い鋼色）
	Sprite* frameLineTop_ = nullptr;    // フレーム上辺の金の縁
	Sprite* frameLineBottom_ = nullptr; // フレーム下辺の暗い金の縁
	Sprite* background_ = nullptr;      // 黒に近い暗い色
	Sprite* delay_ = nullptr;           // 遅れて減る白いゲージ
	Sprite* hp_ = nullptr;              // HPゲージ
	Sprite* hpGloss_ = nullptr;         // HPゲージの光沢
	Sprite* hpEdge_ = nullptr;          // HPゲージの先端の明るい線
	Sprite* flash_ = nullptr;           // 被弾でバーを光らせる（加算）
	Sprite* capOuter_ = nullptr;        // 左端の菱形の飾り（金）
	Sprite* capInner_ = nullptr;        // 菱形の中の芯（HPの色）
	std::array<Spark, kSparkCount> sparks_{};

	// ==========================
	// 状態
	// ==========================
	bool initialized_ = false;
	int32_t lastHp_ = 0;
	float hpRatio_ = 1.0f;       // HPゲージ（即時）
	float delayRatio_ = 1.0f;    // 白ゲージ（遅れて追う）
	float delayFrom_ = 1.0f;     // 追いかけ始めた位置
	float delayHoldTimer_ = 0.0f;
	float delayCatchUpTimer_ = 0.0f;
	float hitFlashTimer_ = 0.0f;
	float shakeTimer_ = 0.0f;
	float pulseTime_ = 0.0f;     // 瀕死の脈動

	std::mt19937 rng_{ std::random_device{}() };
};
