#include "Skeleton.h"
#include "AnimationClipSet.h"
#include <algorithm>
#include <cassert>
#include <utility>

void Skeleton::BuildFromNode(const Node& rootNode)
{
	skeletonData_.joints.clear();
	skeletonData_.jointMap.clear();

	skeletonData_.root = CreateJoint(rootNode, {}, skeletonData_.joints);

	for (const auto& joint : skeletonData_.joints) {
		skeletonData_.jointMap.emplace(joint.name, joint.index);
	}
}

int32_t Skeleton::CreateJoint(const Node& node, const std::optional<int32_t>& parent, std::vector<Joint>& joints)
{
	Joint joint;
	joint.name = node.name;
	joint.localMatrix = node.localMatrix;
	joint.skeletonSpaceMatrix = MakeIdentity4x4();
	joint.transform = node.transform;
	joint.bindTransform = node.transform;
	joint.index = static_cast<int32_t>(joints.size());
	joint.parent = parent;
	joints.push_back(joint);

	for (const auto& child : node.children) {
		int32_t childIndex = CreateJoint(child, joint.index, joints);
		joints[joint.index].children.push_back(childIndex);
	}

	return joint.index;
}

void Skeleton::Update()
{
	// すべてのJointを更新。親が若いので通常ループで処理可能。
	// このエンジンは行ベクトル規約（MakeAffineMatrix が S*R*T を返し、v' = v * M で使う）なので、
	// 子から親への合成は必ず「子 * 親」の順。逆に書くと姿勢が壊れるので触らないこと
	for (Joint& joint : skeletonData_.joints) {
		joint.localMatrix = MakeAffineMatrix(joint.transform.scale, joint.transform.rotate, joint.transform.translate);
		if (joint.parent) {
			joint.skeletonSpaceMatrix = joint.localMatrix * skeletonData_.joints[*joint.parent].skeletonSpaceMatrix;
		} else {
			joint.skeletonSpaceMatrix = joint.localMatrix;
		}
	}
}

namespace {
	// クリップから1ジョイント分の姿勢を取り出す。
	// チャンネルが無いジョイントは fallback（＝バインドポーズ）をそのまま返す
	QuaternionTransform SampleJoint(
		const AnimationData* clip,
		const std::string& jointName,
		float time,
		const QuaternionTransform& fallback)
	{
		if (!clip) return fallback;

		auto it = clip->nodeAnimations.find(jointName);
		if (it == clip->nodeAnimations.end()) return fallback;

		const NodeAnimation& node = it->second;
		QuaternionTransform result = fallback;
		// カーブごとに独立して存在しうるので、空のものは fallback を残す
		if (!node.scale.keyframes.empty()) {
			result.scale = SampleCurve(node.scale.keyframes, time);
		}
		if (!node.translate.keyframes.empty()) {
			result.translate = SampleCurve(node.translate.keyframes, time);
		}
		if (!node.rotate.keyframes.empty()) {
			result.rotate = SampleCurve(node.rotate.keyframes, time);
		}
		return result;
	}
}

void Skeleton::ApplyClip(const AnimationData* clip, float time)
{
	for (auto& joint : skeletonData_.joints) {
		joint.transform = SampleJoint(clip, joint.name, time, joint.bindTransform);
	}
}

void Skeleton::ApplyClipMasked(const AnimationData* clip, float time, const std::vector<uint8_t>& mask, float weight)
{
	if (!clip || mask.size() != skeletonData_.joints.size()) return;

	const float w = std::clamp(weight, 0.0f, 1.0f);
	if (w <= 0.0f) return;

	for (size_t i = 0; i < skeletonData_.joints.size(); ++i) {
		if (!mask[i]) continue;

		Joint& joint = skeletonData_.joints[i];
		// レイヤー側にチャンネルが無いジョイントは下のレイヤーの姿勢を残す。
		// ここでバインドポーズへ落とすと、マスク内の骨だけ勝手に初期姿勢へ戻ってしまう
		auto it = clip->nodeAnimations.find(joint.name);
		if (it == clip->nodeAnimations.end()) continue;

		const QuaternionTransform layered = SampleJoint(clip, joint.name, time, joint.transform);
		if (w >= 1.0f) {
			joint.transform = layered;
		} else {
			joint.transform.scale = Lerp(joint.transform.scale, layered.scale, w);
			joint.transform.translate = Lerp(joint.transform.translate, layered.translate, w);
			joint.transform.rotate = Slerp(joint.transform.rotate, layered.rotate, w);
		}
	}
}

bool Skeleton::BuildSubtreeMask(const std::string& rootJointName, std::vector<uint8_t>& outMask) const
{
	outMask.assign(skeletonData_.joints.size(), 0);

	auto it = skeletonData_.jointMap.find(rootJointName);
	if (it == skeletonData_.jointMap.end()) {
		outMask.clear();
		return false;
	}

	// ジョイントは親が必ず先に並んでいるので、前から1回なめれば子孫まで塗れる
	outMask[it->second] = 1;
	for (size_t i = 0; i < skeletonData_.joints.size(); ++i) {
		const Joint& joint = skeletonData_.joints[i];
		if (joint.parent.has_value() && outMask[*joint.parent]) {
			outMask[i] = 1;
		}
	}
	return true;
}

void Skeleton::CapturePose(std::vector<QuaternionTransform>& out) const
{
	out.resize(skeletonData_.joints.size());
	for (size_t i = 0; i < skeletonData_.joints.size(); ++i) {
		out[i] = skeletonData_.joints[i].transform;
	}
}

void Skeleton::BlendFromPose(const std::vector<QuaternionTransform>& src, float t)
{
	if (src.size() != skeletonData_.joints.size()) return;

	const float blendT = std::clamp(t, 0.0f, 1.0f);
	for (size_t i = 0; i < skeletonData_.joints.size(); ++i) {
		QuaternionTransform& dst = skeletonData_.joints[i].transform;
		dst.scale = Lerp(src[i].scale, dst.scale, blendT);
		dst.translate = Lerp(src[i].translate, dst.translate, blendT);
		dst.rotate = Slerp(src[i].rotate, dst.rotate, blendT);
	}
}

void Skeleton::ResetToBindPose()
{
	for (auto& joint : skeletonData_.joints) {
		joint.transform = joint.bindTransform;
	}
}

const Joint* Skeleton::FindJoint(const std::string& name) const
{
	auto it = skeletonData_.jointMap.find(name);
	if (it == skeletonData_.jointMap.end()) return nullptr;
	return &skeletonData_.joints[it->second];
}

Joint* Skeleton::FindJoint(const std::string& name)
{
	return const_cast<Joint*>(std::as_const(*this).FindJoint(name));
}

const Matrix4x4& Skeleton::GetJointMatrix(const std::string& name) const
{
	// 見つからないときに黙って別のジョイントを返すと原因が追えなくなるので単位行列を返す
	static const Matrix4x4 kIdentity = MakeIdentity4x4();
	const Joint* joint = FindJoint(name);
	return joint ? joint->skeletonSpaceMatrix : kIdentity;
}

std::vector<std::pair<std::string, Matrix4x4>> Skeleton::GetBoneMatrices() const
{
	std::vector<std::pair<std::string, Matrix4x4>> boneMatrices;
	boneMatrices.reserve(skeletonData_.joints.size());
	for (const auto& joint : skeletonData_.joints) {
		boneMatrices.emplace_back(joint.name, joint.skeletonSpaceMatrix);
	}
	return boneMatrices;
}
