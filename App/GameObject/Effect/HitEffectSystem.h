#pragma once
#include <string>
#include "GameObject/Effect/HitStop.h"
#include "Math/Vector2.h"
#include "Math/Vector3.h"

class GameCamera;
class Player;
class GlobalVariables;

/// <summary>
/// 攻撃が当たった1回ぶんの演出リクエスト。
/// ゲームロジック側はこの構造体を埋めて Play() を1回呼ぶだけでよい。
/// </summary>
struct HitEffectRequest {
	/// <summary>ヒット位置（ワールド座標）。VFXとポストエフェクトの中心になる</summary>
	Vector3 position{};
	/// <summary>飛散方向。通常は「攻撃者 → 被弾者」の向き</summary>
	Vector3 direction{ 0.0f, 1.0f, 0.0f };
	/// <summary>攻撃の強さ。演出量のプリセットを引くキーになる</summary>
	HitStopStrength strength = HitStopStrength::Medium;

	/// <summary>ヒットストップの長さ[秒]。攻撃データ由来。0ならヒットストップ無し</summary>
	float hitStopTime = 0.0f;
	/// <summary>ヒットストップ中の揺れ幅。攻撃データ由来</summary>
	float hitStopIntensity = 0.0f;

	/// <summary>
	/// スーパーアーマー等で弾かれたヒットか。
	/// true のときは専用の（控えめな）プリセットが使われ、攻撃が通っていないことを伝える。
	/// </summary>
	bool isArmorHit = false;

	/// <summary>
	/// 再生するVFX（エミッター名）。空なら再生しない。
	/// エミッターは複数のパーティクルグループを束ねられるので、
	/// 「火花＋リング＋煙」を1つの名前で扱える。
	/// </summary>
	std::string vfxName;
};

/// <summary>
/// 攻撃ヒット時の演出を1箇所に集約するクラス。
///
/// これが無かった頃は、ヒットストップ・ポストエフェクト・ライトフラッシュが
/// PlayerWeapon に、パーティクルが各敵クラスに、と散らばっていた。
/// VFXを差し替えるたびにゲームロジックを触る必要がある状態だったのでまとめてある。
///
/// 扱うのは「プレイヤーの攻撃が当たった瞬間」の演出のみ。
/// 敵が自分の時間を止めるための HitStop や、被弾時のカメラシェイクは各所のまま。
/// </summary>
class HitEffectSystem
{
public:
	static HitEffectSystem& GetInstance();

	/// <summary>
	/// シーン開始時に呼ぶ。カメラとプレイヤーは所有せず参照するだけ。
	/// </summary>
	void Initialize(GameCamera* camera, Player* player);
	/// <summary>
	/// シーン終了時に必ず呼ぶ。
	/// カメラ・プレイヤーはシーンと一緒に破棄されるため、参照を切らないと次のシーンで宙吊りになる。
	/// </summary>
	void Finalize();

	/// <summary>ヒット演出をまとめて再生する</summary>
	void Play(const HitEffectRequest& request);

#ifdef _DEBUG
	// エディタ（App/Editor/Windows/HitEffectWindow.cpp）が GlobalVariables を直接いじるための材料。
	// 調整値そのものは GlobalVariables 側にあるので、キーの作り方と接続状態だけ公開する
	// 関数の本体はクラスが完成してから解釈されるので、後ろにある kGroupName を参照できる
	static constexpr const char* GetEditorGroupName() { return kGroupName; }
	static std::string MakeEditorKey(HitStopStrength strength, bool isArmorHit, const char* suffix) {
		return MakeKey(strength, isArmorHit, suffix);
	}
	const GameCamera* GetCamera() const { return camera_; }
	const Player* GetPlayer() const { return player_; }
	bool IsReady() const { return global_ != nullptr; }
#endif // _DEBUG

private:
	HitEffectSystem() = default;
	~HitEffectSystem() = default;
	HitEffectSystem(const HitEffectSystem&) = delete;
	HitEffectSystem& operator=(const HitEffectSystem&) = delete;

	/// <summary>攻撃の強さごとの演出量（設計書 §31 の「攻撃力による演出強度」）</summary>
	struct Preset {
		float particleScale; // VFXの発生数倍率
		float shakeTrauma;   // カメラに加えるトラウマ量[0,1]
	};

	/// <summary>GlobalVariables から現在のプリセットを読む</summary>
	Preset LoadPreset(HitStopStrength strength, bool isArmorHit) const;
	/// <summary>ワールド座標を画面UV(0〜1)に変換する。画面外・カメラ後方なら画面中央</summary>
	static Vector2 CalcScreenUV(const Vector3& worldPosition);

	/// <summary>GlobalVariables のキー名を作る（例: "Heavy" + "ShakeTrauma"）</summary>
	static std::string MakeKey(HitStopStrength strength, bool isArmorHit, const char* suffix);

	// 所有しない。シーンの寿命に紐づくので Finalize() で切る
	GameCamera* camera_ = nullptr;
	Player* player_ = nullptr;

	GlobalVariables* global_ = nullptr;

	static constexpr const char* kGroupName = "HitEffect";
};
