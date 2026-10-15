#pragma once
#include <Math/Vector3.h>
#include <wrl.h>
#include <d3d12.h>

class DirectXManager;

/// <summary>
/// 画面全体の「空気」の設定。背景色・アンビエント・平行光の効き・高さによる暗化を持ち、
/// ライティングパス（Lighting.PS の b5）へ渡す。
///
/// 既定値は洞窟化する前の見た目（黄土色の背景・アンビエント0.06・平行光そのまま・暗化なし）。
/// シーンが必要なときだけ SetParams で書き換え、終わるときに ResetToDefault で戻す。
/// 何もしないシーン（タイトル等）はこれまでどおりの絵になる。
/// </summary>
class RenderEnvironment
{
public:
	struct Params {
		// 背景（何も描かれていない画素）の色。**画面に出る色（sRGB）で指定する**。
		// ライティングはリニアで計算しているので、GPU へ渡すときにリニアへ直す
		Vector3 backgroundColor{ 0.6f, 0.5f, 0.1f };
		// 環境光（リニア）
		Vector3 ambientColor{ 0.06f, 0.06f, 0.06f };
		// 環境光にアルベドを掛ける割合。0 だと色に関係なく同じ明るさを足す（従来の見た目。暗い場面では
		// 全体に灰色のベールが掛かる）、1 だと物の色に応じて足す
		float ambientAlbedoMix = 0.0f;
		// 平行光（type 0）に掛ける色。洞窟では弱い寒色にして、屋外の太陽に見えないようにする
		Vector3 directionalScale{ 1.0f, 1.0f, 1.0f };

		// 高さによる暗化。環境オブジェクト（Ground など、レンダラーの SetEnvironmentSurface が立っているもの）だけに掛かる。
		// darkStartHeight で暗くなり始め、darkEndHeight で darkStrength まで暗くなる（ワールドの y）
		bool heightDarkEnabled = false;
		float darkStartHeight = 0.0f;
		float darkEndHeight = 10.0f;
		float darkStrength = 0.0f;
	};

	static RenderEnvironment& GetInstance();

	void Initialize(DirectXManager* dxManager);
	void Finalize();

	const Params& GetParams() const { return params_; }
	void SetParams(const Params& params) { params_ = params; }
	/// <summary>既定（洞窟化する前の見た目）へ戻す</summary>
	void ResetToDefault() { params_ = Params{}; }

	/// <summary>背景色をリニアにした値。レンダーターゲットのクリアにも使う</summary>
	Vector3 GetBackgroundLinear() const;

	/// <summary>定数バッファを今の設定で書き換え、ライティングのルート引数へ積む</summary>
	void BindToLighting(ID3D12GraphicsCommandList* cmd, UINT rootParameterIndex);

private:
	RenderEnvironment() = default;
	RenderEnvironment(const RenderEnvironment&) = delete;
	RenderEnvironment& operator=(const RenderEnvironment&) = delete;

	// HLSL の EnvironmentCB と並びを合わせること（Lighting.hlsli）
	struct EnvironmentCB {
		float backgroundColor[3];
		float _pad0;
		float ambientColor[3];
		float ambientAlbedoMix;
		float directionalScale[3];
		float _pad2;
		float darkStartHeight;
		float darkEndHeight;
		float darkStrength; // 暗化が無効なら 0
		float _pad3;
	};

	Params params_{};
	Microsoft::WRL::ComPtr<ID3D12Resource> buffer_;
	EnvironmentCB* mapped_ = nullptr;
};
