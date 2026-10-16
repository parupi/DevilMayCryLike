#pragma once
#include <Math/Vector3.h>
#include <random>
#include <vector>

/// <summary>
/// 洞窟の天井（張り出しの岩や鍾乳石）から落ちる水滴と、床で跳ねる波紋・しぶき・音（仕様書 4.4）。
///
/// - 落とす場所は、ステージにある張り出しの岩（Cave/CaveOverhang_*）と鍾乳石（Cave/Stalactite*）の位置から決める
///   （仕様書「水滴の落下位置は、実際に岩が存在する場所を優先する」）。プレイヤーの近くの岩からだけ落とす
/// - 落ちる高さが一定なので着地までの時間も決まる。水滴の寿命をその時間に合わせ、着地の時刻に波紋・しぶき・音を出す
/// - 見た目は Resource/VFX/CaveDrip.vfx.json、頻度などはステージの雰囲気（StageEnvironment::GetDripSettings）
/// </summary>
class CaveDripEmitter
{
public:
	static constexpr const char* kVfxName = "CaveDrip";
	static constexpr const char* kDropParticle = "CaveWaterDrop";
	static constexpr const char* kRippleParticle = "CaveWaterRipple";
	static constexpr const char* kSplashParticle = "CaveWaterSplash";

	CaveDripEmitter();

	/// <summary>毎フレーム呼ぶ。center は落とす範囲の中心（プレイヤーの位置）</summary>
	void Update(float deltaTime, const Vector3& center);

private:
	/// <summary>ステージにある岩の位置から、水滴を落とせる場所（天井側の xz）を集める</summary>
	void CollectSources();
	void SpawnDrop(const Vector3& center);

	struct Source {
		Vector3 position;   // 岩の原点（壁の上段タイルと同じ位置）
		Vector3 inward;     // 部屋の内側の向き（xz）。張り出しの下に落とすのに使う
		bool isStalactite;  // 鍾乳石なら真下に落とす
	};
	struct PendingLanding {
		Vector3 position;
		float timeLeft;
	};

	bool canEmit_ = false;
	std::vector<Source> sources_;
	std::vector<PendingLanding> landings_;
	float spawnTimer_ = 0.0f;
	float nextInterval_ = 1.0f;
	float appliedFallTime_ = -1.0f;
	std::mt19937 rng_;
};
