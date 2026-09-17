#pragma once

#include "BaseModel.h"
#include "ModelLoader.h"
#include "Mesh/Mesh.h"
#include "Material/Material.h"
#include "Animation/AnimationClipSet.h"
#include "Animation/Skeleton.h"

class WorldTransform;
class Object3d;
class ModelRenderer;
class SkinnedInstance;

/// <summary>
/// スキンモデルの「アセット」。ModelManager がファイル名ごとに1つだけ持ち、全インスタンスで共有される。
///
/// ここに入れてよいのは不変データだけ（メッシュ・マテリアル・バインドポーズ・クリップ・
/// スキニングの入力リソース）。再生時刻やポーズ、変形後の頂点はここに置かないこと。
/// 置くと「同じモデルを使うキャラが全員同じポーズになる」状態に逆戻りする。
/// 可変側は SkinnedInstance が持ち、ModelRenderer が1体につき1つ生成する。
/// </summary>
class SkinnedModel : public BaseModel
{
public: // メンバ関数
	// 初期化
	void Initialize(ModelLoader* modelLoader, const std::string& fileName);

	// マテリアルの更新のみ。ポーズの更新は SkinnedInstance::Update が行う
	void Update(const Vector3& objectScale = { 1.0f, 1.0f, 1.0f }) override;

	// BaseModel の引数なし描画はインスタンスを特定できないので使わない。
	// ModelRenderer からは必ず〜With(instance) を呼ぶこと
	void Draw() override {}
	void DrawGBuffer() override {}
	void DrawShadow() override {}

	void DrawWith(SkinnedInstance* instance);
	void DrawGBufferWith(SkinnedInstance* instance);
	void DrawShadowWith(SkinnedInstance* instance);

	// このアセットを使う1体ぶんの可変状態を作る
	std::unique_ptr<SkinnedInstance> CreateInstance();

	std::vector<Material*> GetMaterials() override;
#ifdef _DEBUG
	void DebugGui(ModelRenderer* render) override;
#endif // _DEBUG

private:
	// メッシュごとに散らばっている逆バインドポーズ行列を、スケルトン全体の1本の配列にまとめる。
	// 同じジョイントの逆バインドポーズはどのメッシュでも同じ値なので、パレットはモデルに1つで足りる
	void BuildInverseBindPoseMatrices();

private:
	std::vector<std::unique_ptr<Mesh>> meshes_;
	std::vector<std::unique_ptr<Material>> materials_;

	// アニメーションクリップ集合（不変）
	AnimationClipSet clipSet_;
	// バインドポーズのスケルトン。インスタンスはこれをコピーして自分のポーズにする
	Skeleton bindSkeleton_;
	// ジョイント順に並べた逆バインドポーズ行列
	std::vector<Matrix4x4> inverseBindPoseMatrices_;

	ModelLoader* modelLoader_ = nullptr;

	SkinnedModelData modelData_;

	// 読み込み元のファイル名（拡張子なし）。イベントの書き出し先を組み立てるのに使う
	std::string modelName_;

public:
	const SkinnedModelData& GetModelData() const { return modelData_; }
	const AnimationClipSet* GetClipSet() const { return &clipSet_; }
	// アニメーションイベントの登録用。ロード直後のセットアップでだけ触ること
	AnimationClipSet* GetClipSetMutable() { return &clipSet_; }
	const Skeleton& GetBindSkeleton() const { return bindSkeleton_; }
	const std::vector<Matrix4x4>& GetInverseBindPoseMatrices() const { return inverseBindPoseMatrices_; }

	size_t GetMeshCount() const { return meshes_.size(); }
	const SkinningResource* GetSkinningResource(size_t index) const { return meshes_[index]->GetSkinningResource(); }

	const std::string& GetModelName() const { return modelName_; }
	DirectXManager* GetDxManager() { return modelLoader_->GetDxManager(); }
	SrvManager* GetSrvManager() { return modelLoader_->GetSrvManager(); }
	void SetLocalMatrix(const Matrix4x4& matrix) { modelData_.rootNode.localMatrix = matrix; }

	Material* GetMaterials(uint32_t index) { return materials_[index].get(); }
};
