#pragma once
#include <Math/Vector3.h>
#include <random>

/// <summary>
/// 洞窟の空気中を漂う埃（仕様書 4.1）。プレイヤーのまわりの円柱の中にだけ粒を撒く。
///
/// - 粒の見た目は Resource/VFX/CaveDust.vfx.json の CaveDustMote（加算・ゆっくり漂ってフェードイン・アウト）
/// - 出す数・範囲はステージの雰囲気（StageEnvironment::GetDustSettings）。雰囲気が無ければ何もしない
/// - プレイヤーから離れた所には出さないので、遠くの粒が勝手に減る
/// - 光源の近くの明るい埃は松明（WallTorch の TorchMote）が出す
/// </summary>
class CaveDustEmitter
{
public:
	static constexpr const char* kVfxName = "CaveDust";
	static constexpr const char* kParticleName = "CaveDustMote";

	CaveDustEmitter();

	/// <summary>毎フレーム呼ぶ。center は撒く範囲の中心（プレイヤーの位置）</summary>
	void Update(float deltaTime, const Vector3& center);

private:
	void EmitOne(const Vector3& center);

	bool canEmit_ = false;
	// 最初の1回だけ、寿命ぶんの粒をまとめて撒く（始まった瞬間に空気が空っぽだと不自然なので）
	bool prewarmed_ = false;
	float timer_ = 0.0f;
	std::mt19937 rng_;
};
