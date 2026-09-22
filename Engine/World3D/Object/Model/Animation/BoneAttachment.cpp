#include "BoneAttachment.h"
#include "SkinnedInstance.h"
#include "Skeleton.h"
#include <World3D/Object/Renderer/BaseRenderer.h>
#include <World3D/WorldTransform.h>
#include <Utility/Logger.h>
#include <cmath>

void BoneAttachment::Initialize(BaseRenderer* skinnedRenderer, const std::string& jointName)
{
	jointName_ = jointName;
	renderer_ = skinnedRenderer;
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

bool BoneAttachment::GetModelSpaceMatrix(Matrix4x4& out) const
{
	if (!renderer_) return false;

	WorldTransform* rendererTransform = renderer_->GetWorldTransform();
	if (!rendererTransform) return false;

	const Matrix4x4 rendererLocal = MakeAffineMatrix(
		rendererTransform->GetScale(),
		rendererTransform->GetRotation(),
		rendererTransform->GetTranslation());

	// 縮尺も戻したいので、こちらは正規化しない
	out = Inverse(rendererLocal);
	return true;
}

bool BoneAttachment::GetSocketMatrix(Matrix4x4& out) const
{
	if (!instance_ || !renderer_) return false;

	const Joint* joint = instance_->GetSkeleton()->FindJoint(jointName_);
	if (!joint) return false;

	WorldTransform* rendererTransform = renderer_->GetWorldTransform();
	if (!rendererTransform) return false;

	// レンダラーの「ローカル」行列を自分で組む。
	// GetMatWorld() は親（キャラ本体）まで掛かっているので、ここで使うと二重に掛かる
	const Matrix4x4 rendererLocal = MakeAffineMatrix(
		rendererTransform->GetScale(),
		rendererTransform->GetRotation(),
		rendererTransform->GetTranslation());

	Matrix4x4 socket = joint->skeletonSpaceMatrix * rendererLocal;

	// 基底を正規化してモデルの縮尺を落とす。
	// 残すと、追従させたものまでモデルの縮尺（プレイヤーなら0.38倍）で縮む
	for (int row = 0; row < 3; ++row) {
		const float length = std::sqrt(
			socket.m[row][0] * socket.m[row][0] +
			socket.m[row][1] * socket.m[row][1] +
			socket.m[row][2] * socket.m[row][2]);
		if (length < 0.0001f) return false;
		const float inv = 1.0f / length;
		socket.m[row][0] *= inv;
		socket.m[row][1] *= inv;
		socket.m[row][2] *= inv;
	}

	out = socket;
	return true;
}
