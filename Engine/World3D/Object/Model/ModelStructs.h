#pragma once
#include <cstdint>
#include <array>
#include <Math/Matrix4x4.h>
#include <vector>
#include <d3d12.h>
#include <span>
#include <wrl.h>
#include <optional>
#include <string>
#include <map>
#include "Math/MathUtils.h"
#include "Math/Vector2.h"
#include <Math/Vector4.h>
#include <dxgi1_6.h>

// 骨
struct Joint {
	QuaternionTransform transform;
	// ノード階層から作った時点の姿勢。再生中のクリップにこのジョイントのチャンネルが無いときは
	// 単位変換ではなくこれへ戻す（そうしないとチャンネルを持たないノードのスケールや軸合わせが飛ぶ）
	QuaternionTransform bindTransform;
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
	float r = 1.0f, g = 1.0f, b = 1.0f;
};

struct MaterialData {
	std::string name;
	float Ns = 50.0f;			// 光沢度
	Color Ka{};					// 環境光色
	Color Kd{};					// 拡散反射色
	Color Ks{};					// 鏡面反射光
	float Ni = 1.0f;
	float d = 1.0f;				// 不透明度
	uint32_t illum = 2;
	std::string textureFilePath;
	uint32_t textureIndex = 0;
	bool hasTexture = false;	// mtlのmap_Kd等で有効なテクスチャが指定されていたか
	// シェーダーに渡す基本色。テクスチャ付きなら白(テクスチャそのまま)、
	// テクスチャ無しならmtlのKd/dが入る
	Vector4 baseColor{1.0f, 1.0f, 1.0f, 1.0f};
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

// クリップ上の一点で鳴らしたい合図。
// 「この瞬間に当たり判定を出す」「ここでSEを鳴らす」をモーション側に持たせるためのもの。
// ゲーム側でタイマーを別管理すると、モーションを調整するたびに数値合わせが発生する
struct AnimationEvent {
	float time = 0.0f;
	std::string tag;
};

struct AnimationData {
	float duration; // アニメーション全体の尺
	std::map<std::string, NodeAnimation> nodeAnimations;
	// time 昇順で保持する（AnimationClipSet::AddEvent がソートを維持する）
	std::vector<AnimationEvent> events;
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

struct GBufferMaterialParam
{
	float roughness;
	float metal;
	float dissolveThreshold; // -1.0 = disabled, 0.0-1.0 = dissolve amount
	float dissolveEdgeWidth; // width of edge emissive glow in noise space
	Matrix4x4 uvTransform;
	Vector4 dissolveEdgeColor; // rgb = emissive color, a = intensity multiplier
	Vector4 materialColor;     // baseColorテクスチャに乗算する色(mtlのKd等)
};

struct UVData {
	Vector2 position = { 0.0f, 0.0f };
	float rotation = 0.0f;
	Vector2 scale = { 1.0f, 1.0f };
};