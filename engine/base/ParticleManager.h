#pragma once
#pragma once
#include <DirectXManager.h>
#include <SrvManager.h>
#include <random>
#include <Vector4.h>
#include <Matrix4x4.h>
#include <Camera.h>
#include <Vector2.h>
#include "GlobalVariables.h"
#include "PSOManager.h"
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
	void Initialize(DirectXManager* dxManager, SrvManager* srvManager, PSOManager* psoManager);
	// 更新
	void Update();
	// 描画
	void Draw();
	// パーティクルグループを登録する
	void CreateParticleGroup(const std::string name, const std::string textureFilePath);
	// 描画前処理
	void DrawSet(BlendMode blendMode = BlendMode::kAdd);

#ifdef _DEBUG
	void DebugGui();
#endif // DEBUG

private: // 構造体

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

	struct ParticleForGPU {
		Matrix4x4 WVP;
		Matrix4x4 World;
		Vector4 color;
	};

	struct Particle {
		EulerTransform transform;
		Vector3 velocity;
		Vector3 acc;
		Vector4 color;
		float lifeTime;
		float currentTime;
		bool isAlive;
	};

	struct ParticleGroup {
		MaterialData materialData;  // マテリアルデータ
		std::list<Particle> particleList;  // パーティクルのリスト
		uint32_t srvIndex;  // インスタンシング用SRVインデックス
		Microsoft::WRL::ComPtr<ID3D12Resource> instancingResource;  // インスタンシングリソース
		uint32_t instanceCount;  // インスタンス数
		ParticleForGPU* instancingDataPtr;  // インスタンシングデータを書き込むためのポインタ
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

	struct ParticleParameters {
		Vector2 translateX;
		Vector2 translateY;
		Vector2 translateZ;
		Vector2 rotateX;
		Vector2 rotateY;
		Vector2 rotateZ;
		Vector2 scaleX;
		Vector2 scaleY;
		Vector2 scaleZ;
		Vector2 velocityX;
		Vector2 velocityY;
		Vector2 velocityZ;
		Vector2 lifeTime;
		Vector3 colorMin;
		Vector3 colorMax;
	};
private:
	// パーティクル用のリソースの生成
	void CreateParticleResource();
	// WVP用のリソースを生成 
	void CreateMaterialResource();
	// パーティクルを生成する関数
	Particle MakeNewParticle(const std::string name, std::mt19937& randomEngine, const Vector3& translate);

	ParticleParameters LoadParticleParameters(GlobalVariables* global, const std::string& groupName);
public:

	// nameで指定した名前のパーティクルグループにパーティクルを発生させる関数
	std::list<Particle> Emit(const std::string name, const Vector3& position, uint32_t count);

private:
	const uint32_t kNumMaxInstance = 512;	// 最大インスタンス数
	// パーティクル用リソースの宣言
	Microsoft::WRL::ComPtr<ID3D12Resource> instancingResource_;
	Microsoft::WRL::ComPtr<ID3D12Resource> materialResource_;
	D3D12_VERTEX_BUFFER_VIEW vertexBufferView_{};

	ParticleForGPU* instancingData_ = nullptr;
	Material* materialData_;
	VertexData* vertexData_ = nullptr;
private:
	DirectXManager* dxManager_ = nullptr;
	SrvManager* srvManager_ = nullptr;
	PSOManager* psoManager_ = nullptr;
	Camera* camera_ = nullptr;

	// グローバルバリアース
	GlobalVariables* global_ = GlobalVariables::GetInstance();

	// ランダム用変数宣言
	std::mt19937 randomEngine;

	std::unordered_map<std::string, ParticleGroup> particleGroups_;
	
	// 名前ごとにまとめて管理する1つのマップ
	std::unordered_map<std::string, ParticleParameters> particleParams_;
	// アルファ値だけグループごとに変えれるようにしとく
	std::unordered_map<std::string, float> alpha_;


	bool isBillboard = true;
	const float kDeltaTime = 1.0f / 60.0f;

	Matrix4x4 scaleMatrix;
	Matrix4x4 translateMatrix;


	std::list<Particle> particles;
	uint32_t numInstance = 0;

	Microsoft::WRL::ComPtr<ID3D12Resource> vertexResource;

public:
	void SetAlpha(const std::string name, float alpha) { alpha_[name] = alpha; }
	DirectXManager* GetDxManager() { return dxManager_; }
	SrvManager* GetSrvManager() { return srvManager_; }
	Camera* GetCamera() { return camera_; }
	void SetCamera(Camera* camera) { camera_ = camera; }
};
