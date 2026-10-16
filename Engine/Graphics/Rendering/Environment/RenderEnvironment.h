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

		// 高さフォグ。高い所ほど濃くなる霧を、視線が通った分だけ積み上げる（Lighting.PS）。
		// 濃さ(1mあたり) = heightFogDensity * exp(heightFogFalloff * (y - heightFogBaseHeight))
		bool heightFogEnabled = false;
		Vector3 heightFogColor{ 0.0f, 0.0f, 0.0f }; // 画面に出る色（sRGB）
		float heightFogBaseHeight = 0.0f;
		float heightFogDensity = 0.02f;
		float heightFogFalloff = 0.25f;
		float heightFogMaxOpacity = 1.0f;

		// 距離フォグ。カメラから離れるほど霧の色へ寄せて奥行きを出す（Lighting.PS）。
		// 量 = 1 - exp(-density * max(距離 - start, 0))。上限 maxOpacity
		bool distanceFogEnabled = false;
		Vector3 distanceFogColor{ 0.0f, 0.0f, 0.0f }; // 画面に出る色（sRGB）
		float distanceFogDensity = 0.01f;
		float distanceFogStart = 0.0f;
		float distanceFogMaxOpacity = 1.0f;

		// キャラ（環境オブジェクト以外＝プレイヤー・敵・小物）だけに足す光。暗い場面で見失わないようにする。
		// characterAmbient はアルベドに掛けて足し、rim は視線と面の向きがずれた輪郭ほど強く足す（色そのまま）
		// 露出とトーンマップ。ライティング結果をポストエフェクトへ写すときに掛ける（FinalComposite.PS）。
		// 無効なら従来どおり（1 を超えた所は 8bit のバッファで切り捨て）
		bool toneMapEnabled = false;
		float exposure = 1.0f;
		float toneMapShoulderStart = 0.6f; // ここまでは素通し、ここから上を 1 へなめらかに収める

		// SSAO（仕様書 3.4）。SSAORenderPass が AO を作り、ライティングが環境光と直接光に別々の強さで掛ける。
		// 暗い場所で直接光まで強く削ると黒く潰れるので、直接光の方は弱くする
		bool ssaoEnabled = false;
		float ssaoRadius = 0.8f;          // サンプルを散らす半径[m]
		float ssaoBias = 0.05f;           // 自分の面を遮蔽と数えないための余裕[m]
		float ssaoIntensity = 1.0f;       // 遮蔽の強さ
		float ssaoPower = 1.0f;           // 仕上げのカーブ
		float ssaoAmbientStrength = 1.0f; // 環境光に掛ける割合
		float ssaoDirectStrength = 0.3f;  // 直接光（平行光・ポイントライト等）に掛ける割合

		// ボリューメトリックライト（仕様書 7章）。ポイントライトの光が空気中で散る量を視線に沿って解析的に足す。
		// 光ごとの効き具合は BaseLight::SetVolumetricScale（既定 0 = 散らない）。松明だけが立てる
		bool volumetricEnabled = false;
		float volumetricStrength = 0.02f;    // 散乱の強さ
		float volumetricMaxDistance = 60.0f; // 視線をどこまで積分するか[m]（何も無い所＝背景のとき）

		// スポットライトの光の筋（埃パーティクル仕様書 Step 4）。視線と円錐の重なる区間をレイマーチする。
		// 光ごとの効き具合は点光源と同じく BaseLight::SetVolumetricScale
		float spotVolumetricStrength = 0.0f; // 0 で無効
		int spotVolumetricSteps = 12;        // 1本の視線あたりのサンプル数
		float spotFogNoiseScale = 0.25f;     // 霧の濃淡の細かさ（1mあたり）
		Vector3 spotFogNoiseSpeed{ 0.05f, -0.12f, 0.03f }; // 霧の濃淡が流れる速さ[m/秒]
		float spotFogNoiseInfluence = 0.6f;  // 濃淡の強さ（0=一様な霧）
		float spotPhaseG = 0.35f;            // 前方散乱の強さ（0=全方向に同じ / 正=光源の方を向くと明るい）
		float spotApexFade = 8.0f;           // 光源からこの距離[m]までは筋を薄くする（円錐の頂点が照明器具のように見えないように）

		Vector3 characterAmbient{ 0.0f, 0.0f, 0.0f };
		Vector3 rimColor{ 0.0f, 0.0f, 0.0f };
		float rimPower = 3.0f;

		// 上部の霧の層（水平な半透明の板。ノイズで濃淡をつけてゆっくり流す）。UpperFogRenderer が描く
		struct FogLayer {
			bool enabled = false;
			float height = 5.0f;                // 置く高さ（ワールドの y）
			Vector3 color{ 0.05f, 0.06f, 0.08f }; // 画面に出る色（sRGB）
			float opacity = 0.5f;               // 一番濃い所の不透明度
			float noiseScale = 0.05f;           // ノイズの細かさ（1mあたりの周期）
			Vector3 noiseSpeed{ 0.2f, 0.0f, 0.1f }; // 流れる速さ（x,z を使う。m/秒）
			// 壁や床と交わる所を薄くする距離[m]。負なら共通の fogLayerSoftDistance を使う。
			// 床のすぐ上を這う霧は、共通の長い距離だと床と近すぎて全部消えるので短くする
			float softDistance = -1.0f;
			// 視線が水平に近いほど薄くするか。見上げる霧は板だと分かるので薄くし、見下ろす地面の霧では切る
			bool grazingFade = true;
		};
		// 0〜2 が上部の霧（仕様書 2.2 の FogLayer_01〜03）、3 が地面を這う霧（仕様書 4.2）
		static constexpr int kFogLayerCount = 4;
		static constexpr int kGroundMistLayer = 3;
		FogLayer fogLayers[kFogLayerCount];
		Vector3 fogLayerCenter{ 0.0f, 0.0f, 0.0f }; // 板の中心（x,z を使う）
		float fogLayerHalfSize = 30.0f;             // 板の半分の大きさ[m]。部屋より少し大きくして端を壁の裏へ隠す
		float fogLayerEdgeFade = 0.3f;              // 板の端を薄くする幅（半分の大きさに対する割合）
		float fogLayerSoftDistance = 8.0f;          // 壁や岩と交わる所を薄くする距離[m]。短いと板が壁に刺さる線が見える
	};

	static RenderEnvironment& GetInstance();

	void Initialize(DirectXManager* dxManager);
	void Finalize();

	const Params& GetParams() const { return params_; }
	void SetParams(const Params& params) { params_ = params; }
	/// <summary>既定（洞窟化する前の見た目）へ戻す</summary>
	void ResetToDefault() { params_ = Params{}; }

	/// <summary>背景色をリニアにした値</summary>
	Vector3 GetBackgroundLinear() const;

	/// <summary>sRGB の色（0〜1）をリニアへ</summary>
	static Vector3 ToLinear(const Vector3& srgb);

	/// <summary>
	/// 定数バッファを今の設定で書き換え、ライティングのルート引数へ積む。
	/// rootParameterIndex に b5、その次に AO テクスチャ（t8）のテーブルを積む
	/// </summary>
	void BindToLighting(ID3D12GraphicsCommandList* cmd, UINT rootParameterIndex);

	/// <summary>
	/// デバッグ表示（埃パーティクル仕様書 10.3）。雰囲気の設定（Params）とは別に持つ（毎フレーム上書きされないように）。
	/// 0=通常 / 1=光の筋だけ（空間散乱だけを出す） / 2=埃の受けた光だけ / 3=埃の透明度（2・3 は背景を黒にする）
	/// </summary>
	enum class DebugView : int { None = 0, VolumetricOnly = 1, ParticleLightingOnly = 2, DustAlpha = 3 };
	void SetDebugView(DebugView view) { debugView_ = view; }
	DebugView GetDebugView() const { return debugView_; }

	/// <summary>SSAO の結果（SSAORenderPass が作る）。ライティングの t8 に積む</summary>
	void SetAoSrv(D3D12_GPU_DESCRIPTOR_HANDLE srv) { aoSrv_ = srv; }

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
		float heightFogColor[3]; // リニア
		float heightFogDensity;  // 無効なら 0
		float heightFogBaseHeight;
		float heightFogFalloff;
		float heightFogMaxOpacity;
		float _pad4;
		float distanceFogColor[3]; // リニア
		float distanceFogDensity;  // 無効なら 0
		float distanceFogStart;
		float distanceFogMaxOpacity;
		float _pad5[2];
		float characterAmbient[3];
		float rimPower;
		float rimColor[3];
		float _pad6;
		float ssaoAmbientStrength; // 無効なら 0
		float ssaoDirectStrength;  // 無効なら 0
		float volumetricStrength;  // 無効なら 0
		float volumetricMaxDistance;
		float invViewProj[16];     // 背景の画素の視線方向を出すのに使う
		float spotVolumetricStrength; // 無効なら 0
		float spotVolumetricSteps;
		float spotFogNoiseScale;
		float spotFogNoiseInfluence;
		float spotFogNoiseSpeed[3];
		float spotPhaseG;
		float time; // 霧の濃淡を流す時計[秒]
		float spotApexFade;
		float debugView; // DebugView
		float _pad7;
	};

	Params params_{};
	Microsoft::WRL::ComPtr<ID3D12Resource> buffer_;
	EnvironmentCB* mapped_ = nullptr;
	D3D12_GPU_DESCRIPTOR_HANDLE aoSrv_{};
	float time_ = 0.0f;
	DebugView debugView_ = DebugView::None;
};
