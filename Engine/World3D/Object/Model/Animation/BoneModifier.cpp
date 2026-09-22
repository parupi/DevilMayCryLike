#include "BoneModifier.h"
#include <algorithm>

void BoneModifier::Set(const std::string& jointName, const Quaternion& rotation, float weight, BoneRotationSpace space)
{
	if (jointName.empty()) return;

	for (BoneRotationEntry& entry : entries_) {
		if (entry.jointName != jointName) continue;
		entry.rotation = rotation;
		entry.weight = weight;
		entry.space = space;
		return;
	}

	BoneRotationEntry entry;
	entry.jointName = jointName;
	entry.rotation = rotation;
	entry.weight = weight;
	entry.space = space;
	entries_.push_back(std::move(entry));
}

void BoneModifier::SetEuler(const std::string& jointName, const Vector3& degrees, float weight, BoneRotationSpace space)
{
	Set(jointName, EulerDegree(degrees), weight, space);
}

void BoneModifier::Remove(const std::string& jointName)
{
	entries_.erase(
		std::remove_if(entries_.begin(), entries_.end(),
			[&jointName](const BoneRotationEntry& entry) { return entry.jointName == jointName; }),
		entries_.end());
}

bool BoneModifier::Apply(Skeleton& skeleton) const
{
	if (!enabled_ || entries_.empty()) return false;

	const float global = std::clamp(globalWeight_, 0.0f, 1.0f);
	if (global <= 0.0f) return false;

	bool applied = false;
	for (const BoneRotationEntry& entry : entries_) {
		const float weight = std::clamp(entry.weight, 0.0f, 1.0f) * global;
		// 効かない補正でスケルトンを回し直さないよう、ここで弾く
		if (weight <= 0.001f) continue;

		const Quaternion rotation = Normalize(entry.rotation);
		// 単位クォータニオンからの Slerp。アニメーションの回転と混ぜると、
		// 補正していない軸まで引っぱられる
		const Quaternion delta = (weight >= 0.999f) ? rotation : Slerp(Identity(), rotation, weight);

		if (skeleton.AddJointRotation(entry.jointName, delta, entry.space)) {
			applied = true;
		}
	}
	return applied;
}
