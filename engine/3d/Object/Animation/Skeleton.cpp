#include "Skeleton.h"

void Skeleton::Initialize(Model* model)
{
	model_ = model;

    CreateSkeleton(model_->GetModelData().rootNode);

	skinCluster_ = CreateSkinCluster();
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

	// スキニング行列を更新
	for (size_t jointIndex = 0; jointIndex < skeletonData_.joints.size(); ++jointIndex) {
		assert(jointIndex < skinCluster_.inverseBindPoseMatrices.size());
		skinCluster_.mappedPalette[jointIndex].skeletonSpaceMatrix =
			skeletonData_.joints[jointIndex].skeletonSpaceMatrix * skinCluster_.inverseBindPoseMatrices[jointIndex];
		skinCluster_.mappedPalette[jointIndex].skeletonSpaceInverseTransposeMatrix =
			Transpose(Inverse(skinCluster_.mappedPalette[jointIndex].skeletonSpaceMatrix));
	}
}

void Skeleton::ApplyAnimation(const Animator::Animation* animation, float animationTime)
{
	for (auto& joint : skeletonData_.joints) {
		auto it = animation->nodeAnimations.find(joint.name);
		if (it != animation->nodeAnimations.end()) {
			const Animator::NodeAnimation& nodeAnimation = it->second;
			joint.transform.translate = Animator::CalculateValue(nodeAnimation.translate.keyframes, animationTime);
			joint.transform.rotate = Animator::CalculateValue(nodeAnimation.rotate.keyframes, animationTime);
			joint.transform.scale = Animator::CalculateValue(nodeAnimation.scale.keyframes, animationTime);
		}
	}
}

void Skeleton::CreateSkeleton(const Model::Node& rootNode) {
	skeletonData_.root = CreateJoint(rootNode, {}, skeletonData_.joints);

	for (const auto& joint : skeletonData_.joints) {
		skeletonData_.jointMap[joint.name] = joint.index;
	}
}

int32_t Skeleton::CreateJoint(const Model::Node& node, const std::optional<int32_t>& parent, std::vector<Joint>& joints) {
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

Skeleton::SkinCluster Skeleton::CreateSkinCluster()
{
	SkinCluster skinCluster;

	// palette用のResource確保
	skinCluster.paletteResource = model_->GetDxManager()->CreateBufferResource(sizeof(WellForGPU) * skeletonData_.joints.size());
	WellForGPU* mappedPalette = nullptr;
	skinCluster.paletteResource->Map(0, nullptr, reinterpret_cast<void**>(&mappedPalette));
	skinCluster.mappedPalette = { mappedPalette, skeletonData_.joints.size() };
	skinCluster.paletteSrvHandle.first = model_->GetSrvManager()->GetCPUDescriptorHandle(4);
	skinCluster.paletteSrvHandle.second = model_->GetSrvManager()->GetGPUDescriptorHandle(4);

	//palette用のSRVを生成
	D3D12_SHADER_RESOURCE_VIEW_DESC paletteSrvDesc{};
	paletteSrvDesc.Format = DXGI_FORMAT_UNKNOWN;
	paletteSrvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	paletteSrvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
	paletteSrvDesc.Buffer.FirstElement = 0;
	paletteSrvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;
	paletteSrvDesc.Buffer.NumElements = UINT(skeletonData_.joints.size());
	paletteSrvDesc.Buffer.StructureByteStride = sizeof(WellForGPU);
	model_->GetDxManager()->GetDevice()->CreateShaderResourceView(skinCluster.paletteResource.Get(), &paletteSrvDesc, skinCluster.paletteSrvHandle.first);

	// influence用のResourceを確保
	skinCluster.influenceResource = model_->GetDxManager()->CreateBufferResource(sizeof(VertexInfluence) * model_->GetModelData().vertices.size());
	VertexInfluence* mappedInfluence = nullptr;
	skinCluster.influenceResource->Map(0, nullptr, reinterpret_cast<void**>(&mappedInfluence));
	std::memset(mappedInfluence, 0, sizeof(VertexInfluence) * model_->GetModelData().vertices.size());
	skinCluster.mappedInfluence = { mappedInfluence, model_->GetModelData().vertices.size() };

	// Influence用のVBVを生成
	skinCluster.influenceBufferView.BufferLocation = skinCluster.influenceResource->GetGPUVirtualAddress();
	skinCluster.influenceBufferView.SizeInBytes = UINT(sizeof(VertexInfluence) * model_->GetModelData().vertices.size());
	skinCluster.influenceBufferView.StrideInBytes = sizeof(VertexInfluence);

	// InverseBindPoseMatrixを格納する場所を探し、単位行列で埋める
	skinCluster.inverseBindPoseMatrices.resize(skeletonData_.joints.size());
	std::generate(skinCluster.inverseBindPoseMatrices.begin(), skinCluster.inverseBindPoseMatrices.end(), MakeIdentity4x4);

	for (const auto& jointWeight : model_->GetModelData().skinClusterData) { // ModelのSkinClusterの情報を解析
		auto it = skeletonData_.jointMap.find(jointWeight.first); // JointWeight.firstはJoint名
		if (it == skeletonData_.jointMap.end()) { // 見つからなかったら次に回す
			continue;
		}
		// (*it).secondにはjointのindexが入っている
		skinCluster.inverseBindPoseMatrices[(*it).second] = jointWeight.second.inverseBindPoseMatrix;
		for (const auto& vertexWeight : jointWeight.second.vertexWeights) {
			auto& currentInfluence = skinCluster.mappedInfluence[vertexWeight.vertexIndex];
			for (uint32_t index = 0; index < kNumMaxInfluence; ++index) {
				if (currentInfluence.weights[index] == 0.0f) {
					currentInfluence.weights[index] = vertexWeight.weight;
					currentInfluence.jointIndices[index] = (*it).second;
					break;
				}
			}
		}
	}

	return skinCluster;
}

std::vector<std::pair<std::string, Matrix4x4>> Skeleton::GetBoneMatrices() const
{
	std::vector<std::pair<std::string, Matrix4x4>> boneMatrices;
	for (const auto& joint : skeletonData_.joints) {
		boneMatrices.emplace_back(joint.name, joint.skeletonSpaceMatrix);
	}
	return boneMatrices;
}
