#pragma once
#include "ModelLoader.h"
#include "Vector4.h"
#include "Vector3.h"
#include "Vector2.h"
#include "Matrix4x4.h"
#include <fstream>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <Camera.h>
#include <span>
#include <map>
//#include <DebugSphere.h>

class WorldTransform;
//class Animator;
//class Skeleton;

class Model
{
public: // メンバ関数
	// 初期化
	void Initialize(ModelLoader* modelLoader, const std::string& directoryPath, const std::string& fileName);
	// 描画
	void Draw(WorldTransform* transform);
private:
	// 頂点データの生成
	void CreateVertexResource();
	// Indexデータの生成
	void CreateIndexResource();

public: // 構造体
	struct VertexData {
		Vector4 position;
		Vector2 texcoord;
		Vector3 normal;
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

	struct ModelData {
		std::map<std::string, JointWeightData> skinClusterData;
		std::vector<VertexData> vertices;
		std::vector<uint32_t> indices;
		MaterialData material;
		Node rootNode;
		bool isAnimation;
		bool isHasBones;
	};

	///↓↓↓アニメーション用構造体(引っ越し予定)↓↓↓///
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

	struct Animation {
		float duration; // アニメーション全体の尺
		std::map<std::string, NodeAnimation> nodeAnimations;
	};

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
		uint32_t srvIndex;
	};

	// 現在のアニメーションタイム
	float animationTime = 0.0f;
	///↑↑↑アニメーション用構造体(引っ越し予定)↑↑↑///
private:
	ModelLoader* modelLoader_ = nullptr;
	//Skeleton* skeleton_ = nullptr;

	ModelData modelData_;
	Animation animation_;
	SkeletonData skeleton_;
	SkinCluster skinCluster_;

	Microsoft::WRL::ComPtr<ID3D12Resource> vertexResource_ = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> indexResource_ = nullptr;

	VertexData* vertexData_ = nullptr;
	uint32_t* indexData_ = nullptr;

	D3D12_VERTEX_BUFFER_VIEW vertexBufferView_;
	D3D12_INDEX_BUFFER_VIEW indexBufferView_{};

	std::string directoryPath_;

	// デバッグ用変数
	//std::vector<DebugSphere> spheres_;
public:
	// mtlファイルを読む関数
	//static MaterialData LoadMaterialTemplateFile(const std::string& directoryPath, const std::string& filename);
	// モデルを読む関数
	static ModelData LoadModelFile(const std::string& directoryPath, const std::string& filename);
	// ノードをモデルデータに変換する関数
	static Node ReadNode(aiNode* node);
	// ボーンの保持を確認する
	static bool HasBones(const aiScene* scene);

	///↓↓↓アニメーション用関数(引っ越し予定)↓↓↓///
	// アニメーションを読む処理
	Animation LoadAnimationFile(const std::string& directoryPath, const std::string& filename);
	// アニメーションの更新
	void UpdateAnimation();
	// 骨骨の更新
	void UpdateSkeleton(SkeletonData& skeleton);
	// 任意の時刻を取得する (Vector3)
	Vector3 CalculateValue(const std::vector<KeyframeVector3>& keyframes, float time);
	// 任意の時刻を取得する (Quaternion)
	Quaternion CalculateValue(const std::vector<KeyframeQuaternion>& keyframes, float time);
	// Nodeの階層からSkeletonを作る
	SkeletonData CreateSkeleton(const Model::Node& rootNode);
	// NodeからJointを作る
	int32_t CreateJoint(const Model::Node& node, const std::optional<int32_t>& parent, std::vector<Joint>& joints);
	// アニメーションを適用する
	void ApplyAnimation(SkeletonData& skeleton, const Animation& animation, float animationTime);
	// 全体の更新
	void Update();
	// SkinClusterを生成する関数
	SkinCluster CreateSkinCluster(const SkeletonData& skeleton, const ModelData& modelData);
	// スキンクラスターの更新
	void UpdateSkinCluster(SkinCluster& skinCluster, const SkeletonData& skeleton);
	///↑↑↑アニメーション用関数(引っ越し予定)↑↑↑///


	///↓↓↓デバッグ用関数↓↓↓///
	std::vector<Vector3> GetConnectionPositions();
	uint32_t GetConnectionCount();
	///↑↑↑デバッグ用関数↑↑↑///

public: // ゲッター // セッター //
	//void SetVertices(VertexData vertex);
	void SetTexturePath(const std::string& filePath) { modelData_.material.textureFilePath = filePath; }

	ModelData GetModelData() { return modelData_; }
	std::string GetDirectoryPath() { return directoryPath_; }
	DirectXManager* GetDxManager() { return modelLoader_->GetDxManager(); }
	SrvManager* GetSrvManager() { return modelLoader_->GetSrvManager(); }
	//SkinCluster GetSkinCluster(){return }
	//void SetSkeleton(Skeleton* skeleton) { skeleton_ = skeleton; }
	//Skeleton* GetSkeleton() { return skeleton_; }

};

