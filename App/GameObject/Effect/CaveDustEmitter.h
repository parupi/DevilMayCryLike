#pragma once
#include <Math/Vector3.h>
#include <random>
#include <vector>

/// <summary>
/// 洞窟の空気中を漂う埃（仕様書 4.1）。プレイヤーのまわりの円柱の中にだけ粒を撒く。
///
/// - 粒の見た目は Resource/VFX/CaveDust.vfx.json の CaveDustMote（加算・ゆっくり漂ってフェードイン・アウト）
/// - 出す数・範囲はステージの雰囲気（StageEnvironment::GetDustSettings）。雰囲気が無ければ何もしない
/// - プレイヤーから離れた所には出さないので、遠くの粒が勝手に減る
/// - 光源の近くの明るい埃は松明（WallTorch の TorchMote）が出す
/// - 天井から差すスポットライト（StageSpotLight の「埃を照らす」が正のもの）の光の円錐の中にも撒く。
///   プレイヤーから遠くても、光の中に埃が浮かんで見えるようにするため
/// </summary>
class CaveDustEmitter
{
public:
	static constexpr const char* kVfxName = "CaveDust";
	static constexpr const char* kParticleName = "CaveDustMote";

	CaveDustEmitter();

	/// <summary>
	/// 毎フレーム呼ぶ。プレイヤーとカメラの間を中心に撒く
	/// （カメラはプレイヤーの後ろ上にいるので、プレイヤーのまわりだけだと遠くて見えにくく、カメラ距離フェードで消える）
	/// </summary>
	void Update(float deltaTime, const Vector3& playerPosition, const Vector3& cameraPosition);

private:
	void EmitOne(const Vector3& center);
	/// <summary>スポットライトの円錐の中のランダムな位置に1粒撒く</summary>
	void EmitInCone(const Vector3& apex, const Vector3& axis, float tanOuter, float length);
	void CollectSpotLights();

	struct Cone {
		Vector3 apex;
		Vector3 axis;
		float tanOuter;
		float length;
	};
	std::vector<Cone> cones_;
	float coneTimer_ = 0.0f;
	bool conePrewarmed_ = false;

	bool canEmit_ = false;
	// 最初の1回だけ、寿命ぶんの粒をまとめて撒く（始まった瞬間に空気が空っぽだと不自然なので）
	bool prewarmed_ = false;
	float timer_ = 0.0f;
	std::mt19937 rng_;
};
