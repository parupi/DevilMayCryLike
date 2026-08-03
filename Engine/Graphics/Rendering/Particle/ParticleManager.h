#pragma once
#include "Graphics/Device/DirectXManager.h"
#include "Graphics/Resource/SrvManager.h"
#include <random>
#include <Math/Vector4.h>
#include <Math/Matrix4x4.h>
#include "World3D/Camera/BaseCamera.h"
#include <Math/Vector2.h>
#include "Debugger/GlobalVariables.h"
#include "Graphics/Rendering/PSO/PSOManager.h"
#include "InstanceData.h"
#include "Particle.h"
#include "ParticleUpdateSystem.h"
#include "ParticleRenderSystem.h"
#include "ParticleGroup.h"
#include "ParticleRenderer.h"
#include "ParticleEmitter.h"
#include "MeshShapeSampler.h"
#include "ParticleMath.h"
#include "VFXFile.h"
#include <memory>
#include "Editor/Windows/ParticleEditor.h"

// StructuredBuffer の要素。Particle.VS.hlsl の同名構造体と並び順を合わせること
struct ParticleForGPU {
	Matrix4x4 WVP;
	Matrix4x4 World;
	Vector4 color;
	Vector4 uvOffsetScale; // xy=UVオフセット / zw=UVスケール（スプライトシートのコマ）
};

struct ParticleGroupGPU
{
	uint32_t instancingHandle;
	ParticleForGPU* mappedPtr;
	uint32_t srvIndex;
	BufferHandle vertexHandle = kInvalidBufferHandle;
	BufferHandle indexHandle  = kInvalidBufferHandle;
	D3D12_VERTEX_BUFFER_VIEW vbv{};
	D3D12_INDEX_BUFFER_VIEW  ibv{};
	uint32_t indexCount = 0;
};

struct ParticleRenderState
{
	BlendMode blendMode;
	bool isBillboard;
	uint32_t textureIndex;
};

class ParticleManager
{
private:
	ParticleManager() = default;
	ParticleManager(const ParticleManager&) = delete;
	ParticleManager& operator=(const ParticleManager&) = delete;
public:
	// シングルトンインスタンスの取得
	static ParticleManager& GetInstance();
	// 終了
	void Finalize();
	// 初期化
	void Initialize(DirectXManager* dxManager, PSOManager* psoManager);
	/// <summary>更新</summary>
	/// <param name="deltaTime">
	/// VFX用のデルタタイム（TimeManager::GetVFXDelta()）を渡すこと。
	/// 実時間を渡すとヒットストップ中もパーティクルだけ通常速度で動いてしまう。
	/// </param>
	void Update(float deltaTime);
	// 描画
	void Draw();
	// パーティクルグループを登録する
	void CreateParticleGroup(const std::string name_, const std::string textureFilePath, PrimitiveType shape = PrimitiveType::Plane);
	// エミッターを生成する関数
	void CreateEmitter(const std::string& emitterName, const std::string& dataName = "");
	// 全てのエミッターを削除する関数
	void DeleteAllEmitters();
#ifdef _DEBUG
	// パーティクル用エディタ。描画は Engine/Editor/Windows/ が回すので、ここでは実体を貸すだけ
	ParticleEditor* GetEditor() { return editor_.get(); }
#endif // DEBUG

public: // 構造体

	struct Color {
		float r, g, b;
	};

	struct MaterialData {
		std::string name_;
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

	struct VertexData {
		Vector4 position;
		Vector2 texcoord;
		Vector3 normal;
	};

	struct ModelData {
		std::vector<VertexData> vertices;
		MaterialData material;
	};

	struct Material {
		Vector4 color;
		bool enableLighting;
		float padding[3];
		Matrix4x4 uvTransform;
	};

private:
	// パーティクル用のリソースの生成
	void CreateParticleResource();
	// WVP用のリソースを生成 
	void CreateMaterialResource();
	// パーティクルを生成する関数
	// direction に nullptr 以外を渡すと、useDirectional が有効なグループでは方向付きの速度になる
	Particle MakeNewParticle(const std::string& name_, const Vector3& translate, const Vector3* direction);

	// Emit / EmitFromMesh の共通実装
	void EmitInternal(const std::string& name, const Vector3& position, uint32_t count, const Vector3* direction);

	ParticleParameters LoadParticleParameters(GlobalVariables* global, const std::string& groupName);

	void CreateParticleGPU(const std::string& name, PrimitiveType shape);

	void CreateParticleRenderer(const std::string& name, const std::string& textureFilePath);

	void RegisterEditorParameters(const std::string& name);

	void UploadInstanceData(const std::string& groupName, const std::vector<InstanceData>& instanceList, size_t instanceCount);
public:

	// nameで指定した名前のパーティクルグループにパーティクルを発生させる関数
	void Emit(const std::string name_, const Vector3& position, uint32_t count);

	/// <summary>
	/// 方向を指定してパーティクルを発生させる（ヒット方向へ火花を飛ばす等）。
	/// グループの useDirectional が false の場合、方向は無視され通常の Emit と同じ挙動になる。
	/// </summary>
	void Emit(const std::string& name, const Vector3& position, uint32_t count, const Vector3& direction);

	/// <summary>
	/// 登録済みエミッターをワンショットで再生する。
	/// エミッターは複数のパーティクルグループを束ねられるので、
	/// 「火花＋煙＋リング」のような複合VFXを Resource/Emitter/*.json の1定義で扱える。
	/// </summary>
	/// <param name="countScale">発生数の倍率（攻撃の強さで演出量を変えるのに使う）</param>
	/// <returns>そのエミッターが登録されていれば true</returns>
	bool PlayVFX(const std::string& emitterName, const Vector3& position, float countScale = 1.0f);
	bool PlayVFX(const std::string& emitterName, const Vector3& position, const Vector3& direction, float countScale = 1.0f);

	// モデルのメッシュ表面からパーティクルを発生させる関数
	// worldMatrix でモデルローカル座標→ワールド座標に変換する（回転・スケール込み）
	void EmitFromMesh(const std::string& groupName, const std::string& modelName, const Matrix4x4& worldMatrix, uint32_t count);

private:
	// 1グループあたりの最大インスタンス数。
	// 格子状の壁のように「細かい粒を面で敷き詰める」表現は512では足りないため引き上げてある。
	// 1グループあたり 2048 * 144byte ≒ 288KB のアップロードバッファを確保する。
	const uint32_t kNumMaxInstance = 2048;
	// パーティクル用リソースの宣言
	uint32_t instancingHandle_ = 0;
	uint32_t materialHandle_ = 0;
	uint32_t vertexHandle_ = 0;

	Microsoft::WRL::ComPtr<ID3D12Resource> vertexResource;
	D3D12_VERTEX_BUFFER_VIEW vertexBufferView_{};

	ParticleForGPU* instancingData_ = nullptr;
	Material* materialData_;
	VertexData* vertexData_ = nullptr;
private:
	DirectXManager* dxManager_ = nullptr;
	SrvManager* srvManager_ = nullptr;
	PSOManager* psoManager_ = nullptr;
	BaseCamera* camera_ = nullptr;

	// 更新用のシステム
	ParticleUpdateSystem updateSystem_;
	// 描画設定用のシステム
	ParticleRenderSystem renderSystem_;
	// 最終的な描画クラス
	ParticleRenderer particleRenderer_;

#ifdef _DEBUG
	// エディター用のクラス
	std::unique_ptr<ParticleEditor> editor_;
#endif

	// グローバルバリアース
	GlobalVariables* global_ = &GlobalVariables::GetInstance();

	// ランダム用変数宣言
	std::mt19937 randomEngine;

	// モデル名→メッシュ表面サンプラーのキャッシュ（初回要求時に構築、有効なものだけ保持）
	MeshShapeSampler* GetMeshSampler(const std::string& modelName);

	std::unordered_map<std::string, ParticleGroup> particleGroups_;
	std::unordered_map<std::string, ParticleGroupGPU> particleGPU_;
	std::unordered_map<std::string, ParticleRenderState> renderStates_;
	std::unordered_map<std::string, std::unique_ptr<ParticleEmitter>> emitters_;
	std::unordered_map<std::string, std::unique_ptr<MeshShapeSampler>> meshSamplers_;

public:
	DirectXManager* GetDxManager() { return dxManager_; }
	SrvManager* GetSrvManager() { return srvManager_; }
	BaseCamera* GetCamera() { return camera_; }
	void SetCamera(BaseCamera* camera) { camera_ = camera; }

	const std::unordered_map<std::string, ParticleGroup>& GetParticleGroups() { return particleGroups_; }
	const std::unordered_map<std::string, std::unique_ptr<ParticleEmitter>>& GetEmitters() { return emitters_; }

	/// <summary>
	/// カーブを編集するための可変アクセス（エディタ用）。存在しないグループ名なら nullptr。
	/// カーブは GlobalVariables ではなく別ファイル管理なので、変更後は SaveParticleCurves() を呼ぶこと。
	/// </summary>
	ParticleCurves* GetParticleCurves(const std::string& groupName);
	/// <summary>カーブを Resource/Particle/&lt;groupName&gt;.curve.json へ保存する</summary>
	void SaveParticleCurves(const std::string& groupName);

	// ======================
	// VFX ファイル（設計書 §21-22）
	// ======================

	/// <summary>
	/// Resource/VFX/&lt;vfxName&gt;.vfx.json を読み、パーティクルグループとエミッターをまとめて登録する。
	///
	/// テクスチャ・形状もファイルに入っているので、**新しいVFXを足すのに C++ の変更が要らない**。
	/// パラメータは GlobalVariables へ流し込むため、エディタの編集経路は従来のまま使える。
	/// </summary>
	/// <returns>ファイルが無い・壊れている場合は false（呼び出し側で従来の登録へ落とせる）</returns>
	bool LoadVFX(const std::string& vfxName);

	/// <summary>
	/// 読み込み済みのVFXを現在の値で .vfx.json へ書き戻す。
	/// エディタで調整した内容（パラメータ・カーブ・エミッターの構成）がそのまま保存される。
	/// </summary>
	bool SaveVFX(const std::string& vfxName);

	/// <summary>
	/// 既存のエミッター（Resource/Emitter/*.json 経路で作ったもの）と、
	/// それが束ねているパーティクルグループを1つの .vfx.json に書き出す移行用。
	/// </summary>
	bool ExportEmitterAsVFX(const std::string& emitterName, const std::string& vfxName);

	/// <summary>そのグループがどのVFXに属しているか。属していなければ nullptr</summary>
	const std::string* GetOwningVFX(const std::string& groupName) const;

	/// <summary>Resource/VFX/ にある .vfx.json の一覧</summary>
	std::vector<std::string> ListVFXNames() const;

private:
	/// <summary>VFX定義を組み立てる（SaveVFX / ExportEmitterAsVFX の共通処理）</summary>
	bool BuildVFXDefinition(const std::string& vfxName, const std::string& emitterName, VFXDefinition& outDefinition);

	// パーティクルグループ名 → それを定義している .vfx.json の名前。
	// エディタの保存先をどちらにするか決めるのに使う
	std::unordered_map<std::string, std::string> groupOwnerVFX_;
};
