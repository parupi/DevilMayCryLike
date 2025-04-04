#pragma once
#include <function.h>
#include <vector>
#include <optional>
#include <map>
#include <Model.h>
#include "Animator.h"

class Skeleton
{
public:
	struct Joint {
		QuaternionTransform transform;
		Matrix4x4 localMatrix;
		Matrix4x4 skeletonSpaceMatrix;
		std::string name;
		std::vector<int32_t> children;
		int32_t index;
		std::optional<int32_t> parent;
	};

	struct SkeletonData {
		int32_t root;
		std::map<std::string, int32_t> jointMap;
		std::vector<Joint> joints;
	};

	// スキンクラスター
	static const uint32_t kNumMaxInfluence = 4;
	struct VertexInfluence {
		std::array<float, kNumMaxInfluence> weights;
		std::array<int32_t, kNumMaxInfluence> jointIndices;
	};

	struct WellForGPU {
		Matrix4x4 skeletonSpaceMatrix; // 位置用
		Matrix4x4 skeletonSpaceInverseTransposeMatrix;
	};

	struct SkinCluster {
		std::vector<Matrix4x4> inverseBindPoseMatrices;

		Microsoft::WRL::ComPtr<ID3D12Resource> influenceResource;
		D3D12_VERTEX_BUFFER_VIEW influenceBufferView;
		std::span<VertexInfluence> mappedInfluence;

		Microsoft::WRL::ComPtr<ID3D12Resource> paletteResource;
		std::span<WellForGPU> mappedPalette;
		std::pair<D3D12_CPU_DESCRIPTOR_HANDLE, D3D12_GPU_DESCRIPTOR_HANDLE> paletteSrvHandle;
	};

public:
	// 初期化
	void Initialize(Model* model);
	// 更新
	void Update();
	// SkinClusterの更新
	//void Update();
	// 
	void ApplyAnimation(const Animator::Animation* animation, float animationTime);
private:
	// Nodeの階層からSkeletonを作る
	void CreateSkeleton(const Model::Node& rootNode);
	// NodeからJointを作る
	int32_t CreateJoint(const Model::Node& node, const std::optional<int32_t>& parent, std::vector<Joint>& joints);
	// SkinClusterを生成する関数
	SkinCluster CreateSkinCluster();
private:
	// スケルトンデータ
	SkeletonData skeletonData_;
	// スキンクラスター
	SkinCluster skinCluster_;
	// モデルデータ
	Model* model_;

public: // ゲッター // セッター //
	SkeletonData GetSkeletonData() const { return skeletonData_; }
	SkinCluster GetSkinCluster() const { return skinCluster_; }
	std::vector<std::pair<std::string, Matrix4x4>> GetBoneMatrices() const;
};

