#pragma once
#include <cstdint>
#include <array>
#include <math/Matrix4x4.h>
#include <vector>
#include <d3d12.h>
#include <span>
#include <wrl.h>
#include <optional>
#include <string>
#include <map>
#include "math/function.h"
#include "math/Vector2.h"
#include <math/Vector4.h>
#include <dxgi1_6.h>

// 骨
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

struct VertexData {
	Vector4 position{};
	Vector2 texcoord{};
	Vector3 normal{};
};

struct SkinningInformation {
	uint32_t vertexCount;
	uint32_t padding[3];
};

struct SkinClusterData {
	std::vector<Matrix4x4> inverseBindPoseMatrices;

	Microsoft::WRL::ComPtr<ID3D12Resource> influenceResource;
	std::span<VertexInfluence> mappedInfluence;
	std::pair<D3D12_CPU_DESCRIPTOR_HANDLE, D3D12_GPU_DESCRIPTOR_HANDLE> influenceSrvHandle;
	uint32_t influenceIndex;

	Microsoft::WRL::ComPtr<ID3D12Resource> paletteResource;
	std::span<WellForGPU> mappedPalette;
	std::pair<D3D12_CPU_DESCRIPTOR_HANDLE, D3D12_GPU_DESCRIPTOR_HANDLE> paletteSrvHandle;
	uint32_t paletteIndex;

	Microsoft::WRL::ComPtr<ID3D12Resource> inputVertexResource;
	std::span<VertexData> mappedInputVertex;
	std::pair<D3D12_CPU_DESCRIPTOR_HANDLE, D3D12_GPU_DESCRIPTOR_HANDLE> inputVertexSrvHandle;
	uint32_t inputVertexIndex;

	Microsoft::WRL::ComPtr<ID3D12Resource> outputVertexResource;
	D3D12_VERTEX_BUFFER_VIEW mappedOutputVertex;
	std::pair<D3D12_CPU_DESCRIPTOR_HANDLE, D3D12_GPU_DESCRIPTOR_HANDLE> outputVertexSrvHandle;
	uint32_t outputVertexIndex;

	Microsoft::WRL::ComPtr<ID3D12Resource> skinningInfoResource;
	SkinningInformation* skinningInfoData;
};

struct Node {
	QuaternionTransform transform;
	Matrix4x4 localMatrix;
	std::string name;
	std::vector<Node> children;
};

struct VertexWeightData {
	float weight;
	uint32_t vertexIndex;
};

struct JointWeightData {
	Matrix4x4 inverseBindPoseMatrix;
	std::vector<VertexWeightData> vertexWeights;
};

struct MeshData {
	std::string name;
	std::vector<VertexData> vertices;
	std::vector<int32_t> indices;
	uint32_t materialIndex; // このメッシュに持たせるマテリアルのインデックス
};

struct Color {
	float r, g, b;
};

struct MaterialData {
	std::string name;
	float Ns;
	Color Ka;	// 環境光色
	Color Kd;	// 拡散反射色
	Color Ks;	// 鏡面反射光
	float Ni;
	float d;
	uint32_t illum;
	std::string textureFilePath;
	uint32_t textureIndex = 0;
};

struct ModelData {
	std::vector<MeshData> meshes;
	std::vector<MaterialData> materials;
};

struct SkinnedMeshData {
	std::string name;
	std::vector<VertexData> vertices;
	std::vector<int32_t> indices;
	uint32_t materialIndex; // このメッシュに持たせるマテリアルのインデックス
	std::string skinClusterName; // このメッシュが参照するスキンクラスタの名前
	std::map<std::string, JointWeightData> skinClusterData;
};

struct SkinnedModelData {
	std::vector<SkinnedMeshData> meshes;
	std::vector<MaterialData> materials;
	Node rootNode;
};



template <typename tValue>
struct Keyframe {
	float time;
	tValue value;
};
using KeyframeVector3 = Keyframe<Vector3>;
using KeyframeQuaternion = Keyframe<Quaternion>;

template<typename tValue>
struct AnimationCurve {
	std::vector<Keyframe<tValue>> keyframes;
};

struct NodeAnimation {
	AnimationCurve<Vector3> translate;
	AnimationCurve<Quaternion> rotate;
	AnimationCurve<Vector3> scale;
};

struct AnimationData {
	float duration; // アニメーション全体の尺
	std::map<std::string, NodeAnimation> nodeAnimations;
};

struct MaterialForGPU {
	Vector4 color;
	bool enableLighting;
	float environmentIntensity;
	float padding[2];   // ← パディングは2つで合計4バイト x 3 ＝ 12バイト
	Matrix4x4 uvTransform;
	float shininess;
	float padding2[3];  // 16バイトアラインメント確保
};

struct alignas(16) GBufferMaterialParam
{
	float roughness;  
	float metal;     
	float padding[2]; 
};

struct UVData {
	Vector2 position = { 0.0f, 0.0f };
	float rotation = 0.0f;
	Vector2 size = { 1.0f, 1.0f };
};