#include "Skeleton.h"
#include "Animation.h"
#include <3d/Object/Model/SkinnedModel.h>

void Skeleton::Initialize(SkinnedModel* model)
{
	CreateSkeleton(model->GetModelData().rootNode);
}

void Skeleton::Update()
{
    // すべてのJointを更新。親が若いので通常ループで処理可能
    for (Joint& joint : skeletonData_.joints) {
        joint.localMatrix = MakeAffineMatrix(joint.transform.scale, joint.transform.rotate, joint.transform.translate);
        if (joint.parent) { // 親がいれば親の行列を掛ける
            joint.skeletonSpaceMatrix = joint.localMatrix * skeletonData_.joints[*joint.parent].skeletonSpaceMatrix;
        } else { // 親がいないのでlocalMatrixとskeletonSpaceMatrixは一致する
            joint.skeletonSpaceMatrix = joint.localMatrix;
        }
    }
}

void Skeleton::ApplyNextAnimation(
    AnimationData* current,
    AnimationData* prev,
    float time,
    float blendTime,
    float blendProgress)
{
    assert(current);

    const float blendT = (blendTime > 0.0f) ? std::clamp(blendProgress / blendTime, 0.0f, 1.0f) : 1.0f;

    for (auto& joint : skeletonData_.joints) {
        const std::string& nodeName = joint.name;

        Vector3 scale = { 1, 1, 1 };
        Vector3 translate = { 0, 0, 0 };
        Quaternion rotate = Identity();

        // currentアニメーションから取得
        auto curIt = current->nodeAnimations.find(nodeName);
        if (curIt != current->nodeAnimations.end()) {
            const auto& curNode = curIt->second;
            scale = Animation::CalculateValue(curNode.scale.keyframes, time);
            translate = Animation::CalculateValue(curNode.translate.keyframes, time);
            rotate = Animation::CalculateValue(curNode.rotate.keyframes, time);
        }

        // prevがあるなら補間
        if (prev && blendProgress < blendTime) {
            auto prevIt = prev->nodeAnimations.find(nodeName);
            if (prevIt != prev->nodeAnimations.end()) {
                const auto& prevNode = prevIt->second;

                Vector3 prevScale = Animation::CalculateValue(prevNode.scale.keyframes, time);
                Vector3 prevTranslate = Animation::CalculateValue(prevNode.translate.keyframes, time);
                Quaternion prevRotate = Animation::CalculateValue(prevNode.rotate.keyframes, time);

                scale = Lerp(prevScale, scale, blendT);
                translate = Lerp(prevTranslate, translate, blendT);
                rotate = Slerp(prevRotate, rotate, blendT);
            }
        }

        // 変換を保存
        joint.transform.scale = scale;
        joint.transform.translate = translate;
        joint.transform.rotate = rotate;

        // ローカル行列生成
        joint.localMatrix = MakeAffineMatrix(scale, rotate, translate);
    }

    // 階層をたどってワールド空間行列（skeletonSpaceMatrix）を更新
    for (auto& joint : skeletonData_.joints) {
        if (joint.parent.has_value()) {
            const Matrix4x4& parentMatrix = skeletonData_.joints[joint.parent.value()].skeletonSpaceMatrix;
            joint.skeletonSpaceMatrix = parentMatrix * joint.localMatrix;
        } else {
            joint.skeletonSpaceMatrix = joint.localMatrix;
        }
    }
}

void Skeleton::ApplyAnimation(AnimationData* animation, float animationTime)
{
    for (Joint& joint : skeletonData_.joints) {
        if (auto it = animation->nodeAnimations.find(joint.name); it != animation->nodeAnimations.end()) {
            const NodeAnimation& rootNodeAnimation = (*it).second;
            joint.transform.translate = Animation::CalculateValue(rootNodeAnimation.translate.keyframes, animationTime);
            joint.transform.rotate = Animation::CalculateValue(rootNodeAnimation.rotate.keyframes, animationTime);
            joint.transform.scale = Animation::CalculateValue(rootNodeAnimation.scale.keyframes, animationTime);
        }
    }
}

const Matrix4x4& Skeleton::GetJointMatrix(const std::string& name) const
{
    for (const auto& joint : skeletonData_.joints) {
        if (joint.name == name) {
            return joint.skeletonSpaceMatrix;
        }
    }
    return skeletonData_.joints[0].skeletonSpaceMatrix;
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