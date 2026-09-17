#pragma once
#include <string>
#include "Math/Vector3.h"
#include "Math/Vector4.h"

class DynamicPointLight;

/// <summary>
/// キャラクターに追従するポイントライト。
/// 攻撃がヒットした瞬間に Flash() を呼ぶと、ひときわ強く光ってから元の明るさに戻る。
/// ライトの所有権は LightManager にあり、このクラスの破棄時に自動で登録解除される。
/// </summary>
class CharacterLight
{
public:
	CharacterLight() = default;
	~CharacterLight();

	/// <summary>ライトを生成して LightManager に登録する</summary>
	/// <param name="name">ライト名（重複しないこと。例: name_ + "Light"）</param>
	/// <param name="color">ライトの色</param>
	void Initialize(const std::string& name, const Vector4& color);

	/// <summary>キャラクターの位置に追従させ、フラッシュの減衰を進める</summary>
	/// <param name="ownerPosition">キャラクターのワールド座標</param>
	void Update(const Vector3& ownerPosition, float deltaTime);

	/// <summary>攻撃ヒット時のフラッシュを開始する（ひときわ強く光り、減衰して戻る）</summary>
	void Flash();

	/// <summary>ライトの有効/無効を切り替える（未出現時・死亡時に消灯する）</summary>
	void SetEnabled(bool enabled);

	/// <summary>
	/// ライトの色を一時的に上書きする（スーパーアーマー表示など）。
	/// ClearColorOverride() で Initialize 時の色に戻る。
	/// </summary>
	void SetColorOverride(const Vector4& color);
	/// <summary>色の上書きを解除して Initialize 時の色に戻す</summary>
	void ClearColorOverride();

	/// <summary>
	/// 明るさの倍率を設定する（1=通常）。色の上書きと合わせて状態表示に使う。
	/// </summary>
	void SetIntensityScale(float scale) { intensityScale_ = scale; }

	// ======================
	// 調整用アクセッサ
	// ======================
	void SetBaseIntensity(float intensity) { baseIntensity_ = intensity; }
	void SetBaseRadius(float radius) { baseRadius_ = radius; }
	void SetHeightOffset(float offset) { heightOffset_ = offset; }

private:
	// フラッシュの調整パラメータ
	static constexpr float kFlashIntensityScale = 6.0f; // フラッシュ最大時の明るさ倍率
	static constexpr float kFlashRadiusScale = 1.5f;    // フラッシュ最大時の半径倍率
	static constexpr float kFlashDuration = 0.25f;      // 元の明るさに戻るまでの時間

	DynamicPointLight* light_ = nullptr; // 所有権は LightManager

	Vector4 baseColor_ = { 1.0f, 1.0f, 1.0f, 1.0f }; // Initialize時の色（ClearColorOverrideで戻す先）

	float baseIntensity_ = 1.2f; // 通常時の明るさ
	float baseRadius_ = 8.0f;    // 通常時のライトが届く距離
	float heightOffset_ = 1.2f;  // キャラクター原点からの高さオフセット
	float intensityScale_ = 1.0f; // 明るさの倍率（スーパーアーマー表示などの一時的な増光用）

	float flashTimer_ = 0.0f; // フラッシュの残り時間
};
