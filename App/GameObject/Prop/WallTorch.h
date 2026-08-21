#pragma once
#include "GameObject/Prop/Prop.h"

/// <summary>
/// 壁掛けトーチ。モデル（Torch_wall）を表示し、その穂先に炎のパーティクルと
/// ゆらめくポイントライトを置く。
///
/// **穂先の位置は Prop のライトオフセットと共用する。**
/// 炎とライトを別々の場所に置く理由が無いので1つの値にまとめてある。
/// ステージデータの "light" にそのまま保存されるので、Inspector で合わせれば
/// 次に読み込んだときも同じ場所から燃える。
///
/// モデルの向きの注意: Torch_wall は壁に付く金具が原点の面にあり、
/// **穂先はローカル -X 方向へ突き出す**（.obj では +X だが、エンジンの OBJ 読み込みが
/// X を反転するため）。壁に掛けるときはローカル -X が壁から離れる向きになるよう回すこと。
///
/// 炎の見た目は Resource/VFX/TorchFire.vfx.json（TorchFlame / TorchEmber）が持つ。
/// 最初のトーチが自分で読み込むので、シーン側での登録は要らない。
/// </summary>
class WallTorch : public Prop
{
public:
	// ── 差し替え・調整するならここ ──

	// 既定のモデル。ステージデータ／Inspector で差し替えられる
	static constexpr const char* kModelName = "Torch_wall";

	// 炎VFX。kVfxName は Resource/VFX/<名前>.vfx.json とエミッター名を兼ねる
	static constexpr const char* kVfxName = "TorchFire";
	static constexpr const char* kFlameParticle = "TorchFlame"; // 炎本体（エミッターが束ねる）
	static constexpr const char* kEmberParticle = "TorchEmber"; // 舞い上がる火の粉

	// 穂先（＝炎とライトの位置）。モデルの Fire マテリアルの位置から採った既定値
	static constexpr Vector3 kDefaultTipOffset = { -0.89f, 0.95f, 0.02f };

	// ライトの既定値。松明らしいオレンジ寄りの暖色
	static constexpr Vector3 kDefaultLightColor = { 1.0f, 0.62f, 0.26f };
	static constexpr float kDefaultIntensity = 3.0f;
	static constexpr float kDefaultRadius = 9.0f;
	static constexpr float kDefaultDecay = 1.2f;

	// 炎のゆらぎ。明るさが intensity * (1 ± kFlickerAmount) の範囲で揺れる
	static constexpr float kFlickerAmount = 0.22f;
	static constexpr float kFlickerSpeed = 1.0f;  // ゆらぎ全体の速さ倍率
	static constexpr float kFlickerShake = 0.03f; // 光源自体の揺れ幅[m]。壁の影が動いて炎らしくなる

	// パーティクルの発生間隔[秒]
	static constexpr float kFlameInterval = 0.05f;
	static constexpr float kEmberInterval = 0.35f;

	// これより遠いトーチは炎を出さない[m]（ライトは点いたまま）
	static constexpr float kEmitDistance = 45.0f;

	explicit WallTorch(std::string objectName);

	void Initialize() override;
	void Update(float deltaTime) override;

	/// <summary>火を点ける / 消す。消すと炎もライトも止まる（ライトの設定値は残る）</summary>
	void SetLit(bool lit);
	bool IsLit() const { return isLit_; }

	/// <summary>穂先のワールド座標。炎もライトもここに出る</summary>
	Vector3 GetTipWorldPos();

	// 穂先のオフセットは Prop のライトオフセットそのもの。呼ぶ側で意図が読めるように別名を用意する
	const Vector3& GetTipOffset() const { return GetLightOffset(); }
	Vector3& GetTipOffsetRef() { return GetLightOffsetRef(); }

private:
	/// <summary>炎VFXが未登録なら Resource/VFX/TorchFire.vfx.json を読み込む</summary>
	static void EnsureFireVFXLoaded();

	/// <summary>穂先がカメラから kEmitDistance 以内か。カメラが無ければ常に true</summary>
	static bool IsWithinEmitDistance(const Vector3& position);

	bool isLit_ = true;
	// 火の粉のグループが登録できているか。VFXファイルが無いときに空グループを作らないための保険
	bool canEmitEmber_ = false;

	// ゆらぎ・発生間隔用の時計。トーチごとに位相をずらして、隣同士が同じ瞬間に瞬かないようにする
	float flickerTime_ = 0.0f;
	float flameTimer_ = 0.0f;
	float emberTimer_ = 0.0f;
};
