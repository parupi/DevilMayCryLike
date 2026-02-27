#pragma once
#include "Graphics/Device/DirectXManager.h"
#include "Graphics/Resource/SrvManager.h"
#include <random>
#include <math/Vector4.h>
#include <math/Matrix4x4.h>
#include "3d/Camera/BaseCamera.h"
#include <math/Vector2.h>
#include "debuger/GlobalVariables.h"
#include "Graphics/Rendering/PSO/PSOManager.h"
#include "InstanceData.h"
#include "Particle.h"
#include <3d/Object/Renderer/InstancingRenderer.h>
#include "ParticleUpdateSystem.h"
#include "ParticleRenderSystem.h"
#include "ParticleGroup.h"
#include "ParticleRenderer.h"
#include "ParticleEmitter.h"
#include <memory>
#include "ParticleEditor.h"

struct ParticleForGPU {
	Matrix4x4 WVP;
	Matrix4x4 World;
	Vector4 color;
};

struct ParticleGroupGPU
{
	uint32_t instancingHandle;
	ParticleForGPU* mappedPtr;
	uint32_t srvIndex;
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
	static ParticleManager* instance;

	ParticleManager() = default;
	~ParticleManager() = default;
	ParticleManager(ParticleManager&) = default;
	ParticleManager& operator=(ParticleManager&) = default;
public:
	// シングルトンインスタンスの取得
	static ParticleManager* GetInstance();
	// 終了
	void Finalize();
	// 初期化
	void Initialize(DirectXManager* dxManager, PSOManager* psoManager);
	// 更新
	void Update();
	// 描画
	void Draw();
	// パーティクルグループを登録する
	void CreateParticleGroup(const std::string name_, const std::string textureFilePath);
	// エミッターを生成する関数
	void CreateEmitter(const std::string& emitterName, const std::string& particleName);

#ifdef _DEBUG
	void DebugGui();
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
	Particle MakeNewParticle(const std::string name_, const Vector3& translate);

	ParticleParameters LoadParticleParameters(GlobalVariables* global, const std::string& groupName);

	void DrawEditor(GlobalVariables* global, const std::string& groupName);

	void CreateParticleGPU(const std::string& name);

	void CreateParticleRenderer(const std::string& name, const std::string& textureFilePath);

	void RegisterEditorParameters(const std::string& name);

	void UploadInstanceData(const std::string& groupName, const std::vector<InstanceData>& instanceList);
	// 描画前処理
	void DrawSet(BlendMode blendMode = BlendMode::kAdd);
public:

	// nameで指定した名前のパーティクルグループにパーティクルを発生させる関数
	void Emit(const std::string name_, const Vector3& position, uint32_t count);

private:
	const uint32_t kNumMaxInstance = 512;	// 最大インスタンス数
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
	GlobalVariables* global_ = GlobalVariables::GetInstance();

	// ランダム用変数宣言
	std::mt19937 randomEngine;

	std::unordered_map<std::string, ParticleGroup> particleGroups_;
	std::unordered_map<std::string, ParticleGroupGPU> particleGPU_;
	std::unordered_map<std::string, ParticleRenderState> renderStates_;
	std::unordered_map<std::string, std::unique_ptr<ParticleEmitter>> emitters_;

public:
	DirectXManager* GetDxManager() { return dxManager_; }
	SrvManager* GetSrvManager() { return srvManager_; }
	BaseCamera* GetCamera() { return camera_; }
	void SetCamera(BaseCamera* camera) { camera_ = camera; }

	const std::unordered_map<std::string, ParticleGroup>& GetParticleGroups() { return particleGroups_; }
	const std::unordered_map<std::string, std::unique_ptr<ParticleEmitter>>& GetEmitters() { return emitters_; }
	
};
