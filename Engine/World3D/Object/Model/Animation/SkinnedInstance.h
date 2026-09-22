#pragma once
#include <functional>
#include <memory>
#include <string>
#include <utility>
#include <vector>
#include <d3d12.h>
#include "World3D/Object/Model/ModelStructs.h"
#include "Graphics/Resource/ResourceManager.h"
#include "Skeleton.h"
#include "AnimationPlayer.h"
#include "BoneModifier.h"
#include "IKSolver.h"

class SkinnedModel;
class SkinningResource;
class DirectXManager;
class SrvManager;

/// <summary>
/// スキンモデル1体ぶんの可変状態。ModelRenderer が1つずつ所有する。
///
/// メッシュ・マテリアル・インフルエンス・クリップといった不変データは
/// SkinnedModel（ModelManager が共有で持つアセット）側にあり、
/// こちらは「このキャラのポーズ」と「変形後の頂点」だけを持つ。
/// この分離があるので、同じモデルの敵を何体出してもそれぞれ別の動きができる。
///
/// 行列パレットはモデル全体で1つ。メッシュごとに逆バインドポーズが違うわけではないので、
/// メッシュ数ぶん持つと Transpose(Inverse()) を無駄に繰り返すことになる。
/// </summary>
class SkinnedInstance
{
public:
	~SkinnedInstance();

	void Initialize(SkinnedModel* asset);

	// アニメーションを進めてポーズを作り、行列パレットへ書き込む
	void Update(float deltaTime);
	// CSスキニングを発行する。PSOは呼び出し側（Object3dManager::DispatchSkinning）が設定済み
	void DispatchSkinning();

	AnimationPlayer* GetPlayer() { return player_.get(); }
	const AnimationPlayer* GetPlayer() const { return player_.get(); }
	Skeleton* GetSkeleton() { return &skeleton_; }
	const Skeleton* GetSkeleton() const { return &skeleton_; }

	/// <summary>
	/// アニメーションの上から掛けるボーン補正。
	/// 攻撃ごとにモーションの表情を変える、といった用途はここへ登録する
	/// </summary>
	BoneModifier* GetBoneModifier() { return &boneModifier_; }
	const BoneModifier* GetBoneModifier() const { return &boneModifier_; }

	/// <summary>
	/// ポーズ後処理の差し込み口。ボーン補正の後、行列パレットへ書き込む前に、
	/// 登録した順に呼ばれる。IK のように「補正の結果を見てさらに動かす」ものはここへ繋ぐ。
	///
	/// key は用途の名前（"ArmIK" など）。同じ key は上書きなので、
	/// ゲーム側とエディタが別々の後処理を持っても踏み合わない
	/// </summary>
	void AddPoseCallback(const std::string& key, std::function<void(Skeleton&)> callback);
	void RemovePoseCallback(const std::string& key);

	/// <summary>
	/// IK の注文。ボーン補正の後・ポーズコールバックの前に解かれる。
	/// 毎フレーム設定する前提で、掛けたくないフレームは active を false にする
	/// </summary>
	void SetIKRequest(const IKRequest& request) { ikRequest_ = request; }
	const IKRequest& GetIKRequest() const { return ikRequest_; }
	/// <summary>直近に解いた IK の到達誤差（モデル空間の距離）</summary>
	float GetIKError() const { return ikError_; }

	SkinnedModel* GetAsset() const { return asset_; }

	size_t GetMeshCount() const { return meshOutputs_.size(); }
	// 変形後の頂点バッファ。描画時はメッシュ本来のVBVではなくこちらを使う
	const D3D12_VERTEX_BUFFER_VIEW& GetOutputVBV(size_t meshIndex) const;

private:
	void CreatePalette(uint32_t jointCount);
	void CreateMeshOutput(const SkinningResource* shared);

private:
	// メッシュ1つぶんの出力側リソース
	struct MeshOutput {
		const SkinningResource* shared = nullptr;
		BufferHandle outputHandle = kInvalidBufferHandle;
		BufferHandle infoHandle = kInvalidBufferHandle;
		uint32_t uavIndex = UINT32_MAX;
		D3D12_GPU_DESCRIPTOR_HANDLE uav{};
		D3D12_VERTEX_BUFFER_VIEW vbv{};
		uint32_t vertexCount = 0;
	};

	SkinnedModel* asset_ = nullptr;
	DirectXManager* dxManager_ = nullptr;
	SrvManager* srvManager_ = nullptr;

	Skeleton skeleton_;
	std::unique_ptr<AnimationPlayer> player_;
	// ポーズ後処理。アニメーション → ボーン補正 → コールバック の順に掛かる。
	// 登録順を保ちたいので map ではなく vector
	BoneModifier boneModifier_;
	IKRequest ikRequest_;
	float ikError_ = 0.0f;
	std::vector<std::pair<std::string, std::function<void(Skeleton&)>>> poseCallbacks_;

	// モデル全体で1つの行列パレット
	BufferHandle paletteHandle_ = kInvalidBufferHandle;
	WellForGPU* mappedPalette_ = nullptr;
	uint32_t paletteSrvIndex_ = UINT32_MAX;
	D3D12_GPU_DESCRIPTOR_HANDLE paletteSRV_{};
	uint32_t jointCount_ = 0;

	std::vector<MeshOutput> meshOutputs_;
};
