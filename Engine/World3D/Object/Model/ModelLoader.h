#pragma once
#include "Graphics/Device/DirectXManager.h"
#include "Graphics/Resource/SrvManager.h"
#include "ModelStructs.h"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

class AnimationClipSet;

class ModelLoader
{
public: // メンバ関数
	void Initialize(DirectXManager* dxManager, SrvManager* srvManager);

	// モデルを読む関数
	static ModelData LoadModelFile(const std::string& filename);

	/// <summary>
	/// スキンモデルを読む。outClips を渡すと、同じ aiScene からアニメーションクリップも一緒に取る。
	/// gltf を2回開かないためにこの形にしてある（以前はモデルとアニメで別々に読んでいて、
	/// ロード時間が丸ごと2倍かかっていた）
	/// </summary>
	static SkinnedModelData LoadSkinnedModel(const std::string& filename, AnimationClipSet* outClips = nullptr);


	// ノードをモデルデータに変換する関数
	static Node ReadNode(aiNode* node);

	/// <summary>
	/// モデル名から拡張子抜きのパスを作る。規約は Resource/Models/&lt;モデル名&gt;/&lt;ファイル名&gt; で、
	/// ファイル名はモデル名の最後の要素。
	///   "Sword"           → Resource/Models/Sword/Sword
	///   "Enemys/Skeleton" → Resource/Models/Enemys/Skeleton/Skeleton
	/// 種類ごとにフォルダを切って整理できるよう、区切りを含む名前も受け付ける。
	/// .anim.json など付随ファイルのパスもこれを使って作ること
	/// </summary>
	static std::string MakeAssetBasePath(const std::string& modelName);
private:

	DirectXManager* dxManager_ = nullptr;
	SrvManager* srvManager_ = nullptr;

public: // ゲッター//セッター//
	DirectXManager* GetDxManager() const { return dxManager_; }
	SrvManager* GetSrvManager() const { return srvManager_; }
};

