#include "BoneAttachment.h"
#include "SkinnedInstance.h"
#include "Skeleton.h"
#include <World3D/Object/Renderer/BaseRenderer.h>
#include <World3D/WorldTransform.h>
#include <Utility/Logger.h>

void BoneAttachment::Initialize(BaseRenderer* skinnedRenderer, const std::string& jointName)
{
	jointName_ = jointName;
	instance_ = skinnedRenderer ? skinnedRenderer->GetSkinnedInstance() : nullptr;

	if (!instance_) {
		Logger::Log("[BoneAttachment] スキンモデルではないレンダラーが渡されました\n");
		return;
	}
	if (!instance_->GetSkeleton()->FindJoint(jointName_)) {
		Logger::Log("[BoneAttachment] ジョイントが見つかりません: " + jointName_ + "\n");
	}
}

bool BoneAttachment::IsValid() const
{
	return instance_ && instance_->GetSkeleton()->FindJoint(jointName_) != nullptr;
}

void BoneAttachment::Apply(WorldTransform* target) const
{
	if (!target) return;

	if (!instance_) {
		target->ClearAttachMatrix();
		return;
	}

	const Joint* joint = instance_->GetSkeleton()->FindJoint(jointName_);
	if (!joint) {
		// ジョイント名が間違っていても、追従を諦めるだけで描画は壊さない
		target->ClearAttachMatrix();
		return;
	}

	target->SetAttachMatrix(joint->skeletonSpaceMatrix);
}
