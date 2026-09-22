#pragma once
#include <string>
#include <vector>
#include <Math/Vector3.h>
#include "Skeleton.h"

/// <summary>
/// CCD IK で動かす骨の並びと、解き方の設定。
///
/// jointNames は **根から末端の順**（例: Shoulder.R → UpperArm.R → LowerArm.R）。
/// effectorJoint は「目標へ寄せたい点」を持つジョイントで、普通はチェーンの先にある手
/// （例: Palm.R）。effector 自身は回しても自分の位置が変わらないので、チェーンには入れない。
/// </summary>
struct IKChain {
	std::vector<std::string> jointNames;
	std::string effectorJoint;

	// 反復回数。腕程度なら 4〜8 で十分（増やすほど目標に寄るが、届かない目標では意味が薄い）
	int32_t iterations = 6;
	// これより近付いたら打ち切る距離（モデル空間）
	float tolerance = 0.005f;
	// 0 で補正なし、1 で解いた姿勢そのまま。アニメーションとの混ぜ具合
	float weight = 1.0f;
	// 1ジョイントが元の姿勢から曲がってよい最大角[度]。
	// 入れないと腕が裏返る（肘が逆へ折れる）
	float maxAngleDegrees = 90.0f;
};

/// <summary>
/// 1フレームぶんの IK の注文。SkinnedInstance がポーズ後処理で解く。
/// 毎フレーム作り直して渡す前提で、掛けたくないフレームは active を false にする
/// </summary>
struct IKRequest {
	IKChain chain;
	// スケルトン（モデル）空間の目標。ワールド座標を入れないこと
	Vector3 target{};
	bool active = false;

	// 末端の向きを合わせたいとき（手首を剣の向きへ、など）。空なら何もしない
	std::string alignJoint;
	Quaternion alignRotation = Identity();
	float alignWeight = 0.0f;
};

/// <summary>
/// Cyclic Coordinate Descent。末端から根へ順に
/// 「ボーン→エフェクタ」を「ボーン→目標」へ重ねる回転を足していく。
///
/// 目標が腕の長さより遠いときは、チェーンが目標の方向へ伸びきった姿勢に収束する。
/// 「手を届かせる」だけでなく「腕を目標の方へ向ける」用途にそのまま使えるのはこのため。
/// </summary>
class IKSolver
{
public:
	/// <summary>
	/// target は**スケルトン（モデル）空間**。ワールド座標を渡さないこと。
	///
	/// ローカル回転だけを書き換えるので、呼んだ側が最後に Skeleton::Update() を回すこと
	/// （チェーン内は解く過程で部分更新している）。
	/// outError に最終的な到達誤差[モデル空間の距離]を返す。
	/// 名前が1つでも見つからなければ何もせず false
	/// </summary>
	static bool Solve(Skeleton& skeleton, const IKChain& chain, const Vector3& target, float* outError = nullptr);

	/// <summary>
	/// ジョイントの向きを、モデル空間で指定した回転へ寄せる（手首を剣の向きに合わせる、など）。
	/// weight は 0〜1。Solve と同じくローカル回転だけを書き換える
	/// </summary>
	static bool AlignJoint(Skeleton& skeleton, const std::string& jointName,
		const Quaternion& targetRotationInModel, float weight);
};
