#include "Skeleton.h"
#include "Animation.h"
#include <Model/SkinnedModel.h>

void Skeleton::Initialize(SkinnedModel* model)
{
	model_ = model;

	CreateSkeleton(model_->GetModelData().rootNode);
}

void Skeleton::Update()
{
	for (auto& joint : skeletonData_.joints) {
		joint.localMatrix = MakeAffineMatrix(joint.transform.scale, joint.transform.rotate, joint.transform.translate);

		if (joint.parent && *joint.parent < skeletonData_.joints.size()) {
			joint.skeletonSpaceMatrix = joint.localMatrix * skeletonData_.joints[*joint.parent].skeletonSpaceMatrix;
		}
		else {
			joint.skeletonSpaceMatrix = joint.localMatrix;
		}
	}
}

void Skeleton::ApplyAnimation(const AnimationData& animation, float animationTime)
{
	for (auto& joint : skeletonData_.joints) {
		auto it = animation.nodeAnimations.find(joint.name);
		if (it != animation.nodeAnimations.end()) {
			const NodeAnimation& nodeAnimation = it->second;
			joint.transform.translate = Animation::CalculateValue(nodeAnimation.translate.keyframes, animationTime);
			joint.transform.rotate = Animation::CalculateValue(nodeAnimation.rotate.keyframes, animationTime);
			joint.transform.scale = Animation::CalculateValue(nodeAnimation.scale.keyframes, animationTime);
		}
	}
}

void Skeleton::CreateSkeleton(const Node& rootNode) {
	skeletonData_.root = CreateJoint(rootNode, {}, skeletonData_.joints);

	for (const auto& joint : skeletonData_.joints) {
		skeletonData_.jointMap.emplace(joint.name, joint.index);
	}
}

int32_t Skeleton::CreateJoint(const Node& node, const std::optional<int32_t>& parent, std::vector<Joint>& joints) {
	Joint joint;
	joint.name = node.name;
	joint.localMatrix = node.localMatrix;
	joint.skeletonSpaceMatrix = MakeIdentity4x4();
	joint.transform = node.transform;
	joint.index = static_cast<int32_t>(joints.size());
	joint.parent = parent;
	joints.push_back(joint);

	for (const auto& child : node.children) {
		int32_t childIndex = CreateJoint(child, joint.index, joints);
		joints[joint.index].children.push_back(childIndex);
	}

	return joint.index;
}


std::vector<std::pair<std::string, Matrix4x4>> Skeleton::GetBoneMatrices() const
{
	std::vector<std::pair<std::string, Matrix4x4>> boneMatrices;
	for (const auto& joint : skeletonData_.joints) {
		boneMatrices.emplace_back(joint.name, joint.skeletonSpaceMatrix);
	}
	return boneMatrices;
}