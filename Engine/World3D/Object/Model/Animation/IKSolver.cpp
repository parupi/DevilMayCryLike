#include "IKSolver.h"
#include <algorithm>
#include <cmath>
#include <numbers>

namespace {
	// 2つのクォータニオンの間の角度[度]
	float AngleDegreesBetween(const Quaternion& a, const Quaternion& b)
	{
		float dot = std::abs(Dot(Normalize(a), Normalize(b)));
		dot = std::clamp(dot, -1.0f, 1.0f);
		// q と -q は同じ回転なので、絶対値を取ってから 2*acos
		return 2.0f * std::acos(dot) * 180.0f / static_cast<float>(std::numbers::pi);
	}
}

bool IKSolver::Solve(Skeleton& skeleton, const IKChain& chain, const Vector3& target, float* outError)
{
	if (chain.jointNames.empty()) return false;

	const int32_t effector = skeleton.FindJointIndex(chain.effectorJoint);
	if (effector < 0) return false;

	// 名前を先に引く。1つでも欠けていたら、途中まで回して中途半端な姿勢にしない
	std::vector<int32_t> indices;
	indices.reserve(chain.jointNames.size());
	for (const std::string& name : chain.jointNames) {
		const int32_t index = skeleton.FindJointIndex(name);
		if (index < 0) return false;
		indices.push_back(index);
	}

	// 解く前の姿勢。ウェイトと角度制限はここからの差で掛ける
	std::vector<Quaternion> originalRotations;
	originalRotations.reserve(indices.size());
	for (int32_t index : indices) {
		originalRotations.push_back(skeleton.GetSkeletonData().joints[index].transform.rotate);
	}

	// チェーンの根。最後にここから下をまとめて更新する
	const int32_t rootIndex = indices.front();

	float error = 0.0f;
	for (int32_t iteration = 0; iteration < chain.iterations; ++iteration) {
		Vector3 effectorPosition = skeleton.GetJointPosition(effector);
		error = Length(target - effectorPosition);
		if (error <= chain.tolerance) break;

		// 末端側から根へ。根から回すと、末端の細かい調整が毎回打ち消される
		for (auto it = indices.rbegin(); it != indices.rend(); ++it) {
			const int32_t index = *it;
			const Vector3 jointPosition = skeleton.GetJointPosition(index);

			const Vector3 toEffector = effectorPosition - jointPosition;
			const Vector3 toTarget = target - jointPosition;
			// ボーンの長さが無い／目標がジョイント上にある場合は回す向きが決まらない
			if (Length(toEffector) < 0.0001f || Length(toTarget) < 0.0001f) continue;

			// モデル空間での「今の向き → 目標の向き」。
			// LookRotation はこの規約だと共役が返るので使わないこと
			const Quaternion delta = FromToRotation(toEffector, toTarget);
			skeleton.AddJointRotation(index, delta, BoneRotationSpace::Model);

			// 回したジョイントより先の位置が変わったので、その場で作り直してから次へ進む
			skeleton.UpdateSubtree(index);
			effectorPosition = skeleton.GetJointPosition(effector);
		}
	}

	// ウェイトと角度制限。Slerp は最短経路を取るので、
	// 「元の姿勢から t だけ回す」がそのまま角度の制限になる
	const float weight = std::clamp(chain.weight, 0.0f, 1.0f);
	for (size_t i = 0; i < indices.size(); ++i) {
		QuaternionTransform* transform = skeleton.GetJointTransform(indices[i]);
		if (!transform) continue;

		const Quaternion solved = transform->rotate;
		const Quaternion original = originalRotations[i];

		float t = weight;
		if (chain.maxAngleDegrees > 0.0f) {
			const float angle = AngleDegreesBetween(original, solved);
			if (angle > chain.maxAngleDegrees) {
				t *= chain.maxAngleDegrees / angle;
			}
		}
		transform->rotate = Normalize(Slerp(original, solved, std::clamp(t, 0.0f, 1.0f)));
	}
	skeleton.UpdateSubtree(rootIndex);

	if (outError) {
		*outError = Length(target - skeleton.GetJointPosition(effector));
	}
	return true;
}

bool IKSolver::AlignJoint(Skeleton& skeleton, const std::string& jointName,
	const Quaternion& targetRotationInModel, float weight)
{
	const int32_t index = skeleton.FindJointIndex(jointName);
	if (index < 0) return false;

	const float w = std::clamp(weight, 0.0f, 1.0f);
	if (w <= 0.001f) return true;

	// 今のモデル空間での向きから、目標の向きへ持っていく差分を作る。
	// クォータニオンの積は行列と順番が逆なので、差分は「目標 * 逆」の順
	const Quaternion current = skeleton.GetSkeletonSpaceRotation(index);
	const Quaternion delta = targetRotationInModel * Inverse(current);

	skeleton.AddJointRotation(index, Slerp(Identity(), Normalize(delta), w), BoneRotationSpace::Model);
	skeleton.UpdateSubtree(index);
	return true;
}
