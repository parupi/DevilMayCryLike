#define NOMINMAX
#include "ParticleManager.h"
#include "Math/MathUtils.h"
#include <Debugger/ImGuiManager.h>
#include <algorithm>
#include <Utility/DeltaTime.h>
#include "Graphics/Resource/TextureManager.h"
#include <World3D/Object/Renderer/MeshGenerator.h>

ParticleManager& ParticleManager::GetInstance()
{
	static ParticleManager instance;
	return instance;
}

void ParticleManager::Finalize()
{
	// パーティクルグループのインスタンシングリソースを解放
	for (auto& [name_, group] : particleGroups_) {
		group.particles.clear();
	}
	particleGroups_.clear();

	// メッシュ表面サンプラーのキャッシュを解放
	meshSamplers_.clear();

	// グループごとの頂点・インデックスバッファを解放
	if (dxManager_) {
		auto* rm = dxManager_->GetResourceManager();
		for (auto& [name, gpu] : particleGPU_) {
			if (gpu.vertexHandle != kInvalidBufferHandle) {
				rm->ReleaseBuffer(gpu.vertexHandle);
			}
			if (gpu.indexHandle != kInvalidBufferHandle) {
				rm->ReleaseBuffer(gpu.indexHandle);
			}
		}
	}
	particleGPU_.clear();

	// パーティクル用のリソース
	vertexResource.Reset();

	instancingData_ = nullptr;
	materialData_ = nullptr;
	vertexData_ = nullptr;

	// カメラへの参照も解放（ただし所有権はない）
	camera_ = nullptr;

#ifdef _DEBUG
	if (editor_) {
		editor_->Finalize();
		editor_.reset();
	}
#endif

	// 外部への参照も切っておく（所有権は持っていないのでdeleteはしない）
	dxManager_ = nullptr;
	srvManager_ = nullptr;
	psoManager_ = nullptr;


	// ログ出力など（任意）
	Logger::Log("ParticleManager finalized.\n");
}

void ParticleManager::Initialize(DirectXManager* dxManager, PSOManager* psoManager)
{
	dxManager_ = dxManager;
	srvManager_ = dxManager_->GetSrvManager();
	psoManager_ = psoManager;

	// 乱数エンジン初期化
	std::random_device seedGenerator;
	randomEngine.seed(seedGenerator());

	// リソースの生成と値の設定
	CreateParticleResource();
	CreateMaterialResource();

	// jsonファイルの読み込み
	global_->LoadFiles("Particle");

#ifdef _DEBUG
	editor_ = std::make_unique<ParticleEditor>();
	editor_->Initialize(this);
#endif
}

void ParticleManager::Update()
{
	if (!camera_) return;

	float delta = DeltaTime::GetDeltaTime();

	for (auto& [name, emitter] : emitters_) {
		emitter->Update();
	}

	for (auto& [groupName, group] : particleGroups_) {

		// ① シミュレーション更新
		updateSystem_.Update(group.particles, delta);

		// ② Editorパラメータ読み込み（必要なら）
		group.params = LoadParticleParameters(global_, groupName);
	}

#ifdef _DEBUG
	editor_->Draw();
#endif
}

void ParticleManager::Draw()
{
	if (!dxManager_ || !camera_) return;

	auto* commandList = dxManager_->GetCommandList();
	if (!commandList) return;

	commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	for (auto& [groupName, group] : particleGroups_)
	{
		// ① Instance生成
		std::vector<InstanceData> instanceList;
		instanceList.reserve(group.particles.size());

		renderSystem_.BuildInstances(group.particles, camera_, instanceList);

		if (instanceList.empty()) continue;

		// インスタンシングバッファは kNumMaxInstance 個ぶんしか確保していないため、
		// 溢れたぶんは描画しない（超過分を書き込むとバッファ外破壊になる）
		const size_t instanceCount = (std::min)(instanceList.size(), static_cast<size_t>(kNumMaxInstance));

		// ② PSO取得（globalから）
		BlendMode blendMode = static_cast<BlendMode>(global_->GetValueRef<int>(groupName, "BlendMode"));

		auto pso = psoManager_->GetParticlePSO(blendMode);
		if (!pso) continue;

		commandList->SetPipelineState(pso);
		commandList->SetGraphicsRootSignature(psoManager_->GetParticleSignature());

		// ③ GPU転送
		UploadInstanceData(groupName, instanceList, instanceCount);

		// === 頂点・インデックスバッファ設定（グループごとの形状）===
		auto& gpu = particleGPU_[groupName];
		commandList->IASetVertexBuffers(0, 1, &gpu.vbv);
		commandList->IASetIndexBuffer(&gpu.ibv);

		// === 定数バッファ設定 ===
		commandList->SetGraphicsRootConstantBufferView(0, dxManager_->GetResourceManager()->GetGPUVirtualAddress(materialHandle_));

		// === SRV設定 ===
		srvManager_->SetGraphicsRootDescriptorTable(1, gpu.srvIndex);

		// === テクスチャ設定 ===
		srvManager_->SetGraphicsRootDescriptorTable(2, renderStates_[groupName].textureIndex);

		// ④ 描画
		particleRenderer_.Draw(commandList, instanceCount, gpu.indexCount);
	}
}

void ParticleManager::CreateParticleGroup(const std::string name, const std::string textureFilePath, PrimitiveType shape)
{
	if (particleGroups_.contains(name)) {
		// 登録済みの名前なら早期リターン
		return;
	}

	TextureManager::GetInstance().LoadTexture(textureFilePath);

	ParticleGroup group{};
	group.shape = shape;
	particleGroups_.emplace(name, std::move(group));

	RegisterEditorParameters(name);
	CreateParticleGPU(name, shape);
	CreateParticleRenderer(name, textureFilePath);
}

void ParticleManager::CreateEmitter(const std::string& emitterName, const std::string& dataName)
{
	if (emitters_.contains(emitterName))
	{
		return;
	}

	auto emitter = std::make_unique<ParticleEmitter>();
	emitter->Initialize(this, emitterName, dataName);

	emitters_.emplace(emitterName, std::move(emitter));
}

void ParticleManager::DeleteAllEmitters()
{
	emitters_.clear();
}

void ParticleManager::CreateParticleGPU(const std::string& name, PrimitiveType shape)
{
	auto* rm = dxManager_->GetResourceManager();

	ParticleGroupGPU gpu{};

	// --- インスタンシングバッファ ---
	gpu.instancingHandle = rm->CreateUploadBuffer(sizeof(ParticleForGPU) * kNumMaxInstance, L"ParticleInstancing");
	gpu.mappedPtr = reinterpret_cast<ParticleForGPU*>(rm->Map(gpu.instancingHandle));

	gpu.srvIndex = srvManager_->Allocate();
	srvManager_->CreateSRVforStructuredBuffer(gpu.srvIndex, rm->GetResource(gpu.instancingHandle), kNumMaxInstance, sizeof(ParticleForGPU));

	// --- 形状に応じたメッシュ生成 ---
	MeshData meshData;
	switch (shape) {
	case PrimitiveType::Ring:
		meshData = MeshGenerator::CreateRing();
		break;
	case PrimitiveType::Cylinder:
		meshData = MeshGenerator::CreateCylinder();
		break;
	case PrimitiveType::Plane:
	default:
		// XY平面 (Billboard向け)、旧来仕様と同じ -1〜1 の 2x2 サイズ
		meshData.vertices = {
			{{ 1,  1, 0, 1}, {0, 0}, {0, 0, 1}},
			{{-1,  1, 0, 1}, {1, 0}, {0, 0, 1}},
			{{ 1, -1, 0, 1}, {0, 1}, {0, 0, 1}},
			{{-1, -1, 0, 1}, {1, 1}, {0, 0, 1}},
		};
		meshData.indices = {0, 1, 2, 2, 1, 3};
		break;
	}

	// --- 頂点バッファ ---
	const size_t vbSize = sizeof(VertexData) * meshData.vertices.size();
	gpu.vertexHandle = rm->CreateUploadBuffer(vbSize, L"ParticleVB");
	void* vbPtr = rm->Map(gpu.vertexHandle);
	assert(vbPtr);
	std::memcpy(vbPtr, meshData.vertices.data(), vbSize);
	ID3D12Resource* vbResource = rm->GetResource(gpu.vertexHandle);
	gpu.vbv.BufferLocation = vbResource->GetGPUVirtualAddress();
	gpu.vbv.SizeInBytes    = static_cast<UINT>(vbSize);
	gpu.vbv.StrideInBytes  = sizeof(VertexData);

	// --- インデックスバッファ ---
	const size_t ibSize = sizeof(uint32_t) * meshData.indices.size();
	gpu.indexHandle = rm->CreateUploadBuffer(ibSize, L"ParticleIB");
	void* ibPtr = rm->Map(gpu.indexHandle);
	assert(ibPtr);
	std::memcpy(ibPtr, meshData.indices.data(), ibSize);
	ID3D12Resource* ibResource = rm->GetResource(gpu.indexHandle);
	gpu.ibv.BufferLocation = ibResource->GetGPUVirtualAddress();
	gpu.ibv.SizeInBytes    = static_cast<UINT>(ibSize);
	gpu.ibv.Format         = DXGI_FORMAT_R32_UINT;
	gpu.indexCount         = static_cast<uint32_t>(meshData.indices.size());

	particleGPU_.emplace(name, std::move(gpu));
}

void ParticleManager::CreateParticleRenderer(const std::string& name, const std::string& textureFilePath)
{
	ParticleRenderState state{};
	state.textureIndex = TextureManager::GetInstance().GetTextureIndexByFilePath(textureFilePath);

	TextureManager::GetInstance().LoadTexture(textureFilePath);

	renderStates_.emplace(name, std::move(state));
}

void ParticleManager::RegisterEditorParameters(const std::string& name)
{
	// パーティクルグループごとにグローバルバリアースを作る
	global_->AddItem(name, "minTranslate", Vector3{});
	global_->AddItem(name, "maxTranslate", Vector3{});

	global_->AddItem(name, "minRotate", Vector3{});
	global_->AddItem(name, "maxRotate", Vector3{});

	global_->AddItem(name, "minScale", Vector3{});
	global_->AddItem(name, "maxScale", Vector3{});

	global_->AddItem(name, "minVelocity", Vector3{});
	global_->AddItem(name, "maxVelocity", Vector3{});

	global_->AddItem(name, "minLifeTime", float{});
	global_->AddItem(name, "maxLifeTime", float{});

	global_->AddItem(name, "minColor", Vector3{});
	global_->AddItem(name, "maxColor", Vector3{});

	global_->AddItem(name, "minAlpha", float{});
	global_->AddItem(name, "maxAlpha", float{});

	global_->AddItem(name, "IsBillboard", bool{});

	global_->AddItem(name, "FadeType", int{});

	global_->AddItem(name, "ShrinkStartRatio", float{});

	global_->AddItem(name, "BlendMode", int{ 2 });

	// 放射方向速度モード (0:None 1:Converge 2:Diverge)
	global_->AddItem(name, "RadialMode", int{});
	global_->AddItem(name, "RadialSpeed", float{ 1.0f });
}

void ParticleManager::UploadInstanceData(const std::string& groupName, const std::vector<InstanceData>& instanceList, size_t instanceCount)
{
	auto& gpuBuffer = particleGPU_[groupName];

	auto* dst = reinterpret_cast<ParticleForGPU*>(gpuBuffer.mappedPtr);

	for (size_t i = 0; i < instanceCount; ++i) {
		dst[i].WVP = instanceList[i].wvp;
		dst[i].World = instanceList[i].world;
		dst[i].color = instanceList[i].color;
	}
}

#ifdef _DEBUG
void ParticleManager::DebugGui()
{

}
#endif // DEBUG

void ParticleManager::CreateParticleResource()
{
	auto* rm = dxManager_->GetResourceManager();

	// -----------------------------
	// インスタンス用バッファ
	// -----------------------------
	instancingHandle_ = rm->CreateUploadBuffer(sizeof(ParticleForGPU) * kNumMaxInstance, L"ParticleGlobalInstancing");

	instancingData_ = reinterpret_cast<ParticleForGPU*>(rm->Map(instancingHandle_));

	// 初期値
	for (uint32_t i = 0; i < kNumMaxInstance; ++i) {
		instancingData_[i].WVP = MakeIdentity4x4();
		instancingData_[i].World = MakeIdentity4x4();
		instancingData_[i].color = { 1, 1, 1, 1 };
	}

	// -----------------------------
	// モデルデータ(Plane)
	// -----------------------------
	ModelData modelData;
	modelData.vertices = {
		{{1, 1, 0, 1}, {0,0}, {0,0,1}},
		{{-1, 1, 0, 1}, {1,0}, {0,0,1}},
		{{1, -1, 0, 1}, {0,1}, {0,0,1}},
		{{1, -1, 0, 1}, {0,1}, {0,0,1}},
		{{-1, 1, 0, 1}, {1,0}, {0,0,1}},
		{{-1, -1, 0, 1}, {1,1}, {0,0,1}},
	};

	// -----------------------------
	// VertexBuffer → DefaultBuffer
	// -----------------------------
	size_t vbSize = sizeof(VertexData) * modelData.vertices.size();

	vertexResource = rm->CreateUploadBufferWithData(modelData.vertices.data(), vbSize);


	vertexBufferView_.BufferLocation = vertexResource->GetGPUVirtualAddress();
	vertexBufferView_.SizeInBytes = UINT(vbSize);
	vertexBufferView_.StrideInBytes = sizeof(VertexData);
}


void ParticleManager::CreateMaterialResource()
{
	auto* rm = dxManager_->GetResourceManager();

	materialHandle_ = rm->CreateUploadBuffer(sizeof(Material), L"ParticleMaterial");

	materialData_ = reinterpret_cast<Material*>(rm->Map(materialHandle_));

	// 初期値
	materialData_->color = { 1, 1, 1, 1 };
	materialData_->enableLighting = true;
	materialData_->uvTransform = MakeIdentity4x4();
}

Particle ParticleManager::MakeNewParticle(const std::string name, const Vector3& translate)
{
	Particle particle{};

	auto& params = particleGroups_[name].params;

	auto FixRange = [](const Vector2& range) {
		auto [minVal, maxVal] = std::minmax(range.x, range.y);
		return Vector2{ minVal, maxVal };
		};

	auto FixRangeVec3 = [](const Vector3& minVal, const Vector3& maxVal) {
		return std::pair<Vector3, Vector3>{
			{(std::min)(minVal.x, maxVal.x), (std::min)(minVal.y, maxVal.y), (std::min)(minVal.z, maxVal.z)},
			{ (std::max)(minVal.x, maxVal.x), (std::max)(minVal.y, maxVal.y), (std::max)(minVal.z, maxVal.z) }
		};
		};

	// 各範囲を修正
	params.translateX = FixRange(params.translateX);
	params.translateY = FixRange(params.translateY);
	params.translateZ = FixRange(params.translateZ);

	params.rotateX = FixRange(params.rotateX);
	params.rotateY = FixRange(params.rotateY);
	params.rotateZ = FixRange(params.rotateZ);

	params.scaleX = FixRange(params.scaleX);
	params.scaleY = FixRange(params.scaleY);
	params.scaleZ = FixRange(params.scaleZ);

	params.velocityX = FixRange(params.velocityX);
	params.velocityY = FixRange(params.velocityY);
	params.velocityZ = FixRange(params.velocityZ);

	params.lifeTime = FixRange(params.lifeTime);

	auto [colorMin, colorMax] = FixRangeVec3(params.colorMin, params.colorMax);
	params.colorMin = colorMin;
	params.colorMax = colorMax;

	std::uniform_real_distribution<float> distTranslationX(params.translateX.x, params.translateX.y);
	std::uniform_real_distribution<float> distTranslationY(params.translateY.x, params.translateY.y);
	std::uniform_real_distribution<float> distTranslationZ(params.translateZ.x, params.translateZ.y);

	std::uniform_real_distribution<float> distRotationX(params.rotateX.x, params.rotateX.y);
	std::uniform_real_distribution<float> distRotationY(params.rotateY.x, params.rotateY.y);
	std::uniform_real_distribution<float> distRotationZ(params.rotateZ.x, params.rotateZ.y);

	std::uniform_real_distribution<float> distScaleX(params.scaleX.x, params.scaleX.y);
	std::uniform_real_distribution<float> distScaleY(params.scaleY.x, params.scaleY.y);
	std::uniform_real_distribution<float> distScaleZ(params.scaleZ.x, params.scaleZ.y);

	std::uniform_real_distribution<float> distVelocityX(params.velocityX.x, params.velocityX.y);
	std::uniform_real_distribution<float> distVelocityY(params.velocityY.x, params.velocityY.y);
	std::uniform_real_distribution<float> distVelocityZ(params.velocityZ.x, params.velocityZ.y);

	std::uniform_real_distribution<float> distTime(params.lifeTime.x, params.lifeTime.y);
	std::uniform_real_distribution<float> distColorR(params.colorMin.x, params.colorMax.x);
	std::uniform_real_distribution<float> distColorG(params.colorMin.y, params.colorMax.y);
	std::uniform_real_distribution<float> distColorB(params.colorMin.z, params.colorMax.z);
	particle.transform.scale = { distScaleX(randomEngine), distScaleY(randomEngine), distScaleZ(randomEngine) };
	particle.transform.rotate = { distRotationX(randomEngine), distRotationY(randomEngine), distRotationZ(randomEngine) };
	Vector3 randomTranslate = { distTranslationX(randomEngine), distTranslationY(randomEngine), distTranslationZ(randomEngine) };
	particle.transform.translate = translate + randomTranslate;
	particle.velocity = { distVelocityX(randomEngine), distVelocityY(randomEngine), distVelocityZ(randomEngine) };
	particle.color = { distColorR(randomEngine) , distColorG(randomEngine) , distColorB(randomEngine) , 1.0f };
	particle.lifeTime = distTime(randomEngine);
	particle.currentTime = 0.0f;
	particle.isBillboard = params.isBillboard;

	// 放射方向速度モード（発生位置オフセットに応じた速度を与える）
	switch (static_cast<RadialMode>(params.radialMode)) {
	case RadialMode::Converge:
		// 寿命が尽きる瞬間に発生中心へちょうど到達する速度（RadialSpeedは倍率）
		if (particle.lifeTime > 0.0f) {
			particle.velocity = randomTranslate * (-params.radialSpeed / particle.lifeTime);
		}
		break;
	case RadialMode::Diverge: {
		// 発生中心から外向きに RadialSpeed [m/s] で飛ばす（±50%のばらつき付き）
		Vector3 dir = randomTranslate;
		float len = Length(dir);
		if (len < 0.0001f) {
			// 中心ぴったりに湧いた場合はランダム方向へ
			std::uniform_real_distribution<float> distDir(-1.0f, 1.0f);
			dir = { distDir(randomEngine), distDir(randomEngine), distDir(randomEngine) };
			len = Length(dir);
			if (len < 0.0001f) { dir = { 0.0f, 1.0f, 0.0f }; len = 1.0f; }
		}
		std::uniform_real_distribution<float> distSpeedScale(0.5f, 1.5f);
		particle.velocity = dir * (params.radialSpeed * distSpeedScale(randomEngine) / len);
		break;
	}
	default:
		break;
	}

	return particle;
}

ParticleParameters ParticleManager::LoadParticleParameters(GlobalVariables* global, const std::string& groupName)
{
	ParticleParameters params{};

	// Translate
	params.translateX = { global->GetValueRef<Vector3>(groupName, "minTranslate").x, global->GetValueRef<Vector3>(groupName, "maxTranslate").x };
	params.translateY = { global->GetValueRef<Vector3>(groupName, "minTranslate").y, global->GetValueRef<Vector3>(groupName, "maxTranslate").y };
	params.translateZ = { global->GetValueRef<Vector3>(groupName, "minTranslate").z, global->GetValueRef<Vector3>(groupName, "maxTranslate").z };

	// GetValue<Vector3>
	params.rotateX = { global->GetValueRef<Vector3>(groupName, "minRotate").x, global->GetValueRef<Vector3>(groupName, "maxRotate").x };
	params.rotateY = { global->GetValueRef<Vector3>(groupName, "minRotate").y, global->GetValueRef<Vector3>(groupName, "maxRotate").y };
	params.rotateZ = { global->GetValueRef<Vector3>(groupName, "minRotate").z, global->GetValueRef<Vector3>(groupName, "maxRotate").z };

	// Scale
	params.scaleX = { global->GetValueRef<Vector3>(groupName, "minScale").x, global->GetValueRef<Vector3>(groupName, "maxScale").x };
	params.scaleY = { global->GetValueRef<Vector3>(groupName, "minScale").y, global->GetValueRef<Vector3>(groupName, "maxScale").y };
	params.scaleZ = { global->GetValueRef<Vector3>(groupName, "minScale").z, global->GetValueRef<Vector3>(groupName, "maxScale").z };

	// Velocity
	params.velocityX = { global->GetValueRef<Vector3>(groupName, "minVelocity").x, global->GetValueRef<Vector3>(groupName, "maxVelocity").x };
	params.velocityY = { global->GetValueRef<Vector3>(groupName, "minVelocity").y, global->GetValueRef<Vector3>(groupName, "maxVelocity").y };
	params.velocityZ = { global->GetValueRef<Vector3>(groupName, "minVelocity").z, global->GetValueRef<Vector3>(groupName, "maxVelocity").z };

	// LifeTime
	params.lifeTime = { global->GetValueRef<float>(groupName, "minLifeTime"), global->GetValueRef<float>(groupName, "maxLifeTime") };

	// Color
	params.colorMin = global->GetValueRef<Vector3>(groupName, "minColor");
	params.colorMax = global->GetValueRef<Vector3>(groupName, "maxColor");

	params.isBillboard = global->GetValueRef<bool>(groupName, "IsBillboard");

	// Radial
	params.radialMode = global->GetValueRef<int>(groupName, "RadialMode");
	params.radialSpeed = global->GetValueRef<float>(groupName, "RadialSpeed");

	return params;
}

void ParticleManager::Emit(const std::string name, const Vector3& position, uint32_t count)
{
	auto& particles = particleGroups_[name].particles;

	particles.reserve(particles.size() + count);

	for (uint32_t i = 0; i < count; ++i) {
		particles.emplace_back(MakeNewParticle(name, position));
	}
}

MeshShapeSampler* ParticleManager::GetMeshSampler(const std::string& modelName)
{
	auto it = meshSamplers_.find(modelName);
	if (it != meshSamplers_.end()) {
		return it->second.get();
	}

	// 初回要求時に構築する。モデルが未ロードなら構築失敗として何も返さない
	// （キャッシュしないので、あとからロードされれば次回の要求で構築される）
	auto sampler = std::make_unique<MeshShapeSampler>();
	if (!sampler->Build(modelName)) {
		return nullptr;
	}

	MeshShapeSampler* raw = sampler.get();
	meshSamplers_.emplace(modelName, std::move(sampler));
	return raw;
}

void ParticleManager::EmitFromMesh(const std::string& groupName, const std::string& modelName, const Matrix4x4& worldMatrix, uint32_t count)
{
	MeshShapeSampler* sampler = GetMeshSampler(modelName);
	if (!sampler) return;

	auto& particles = particleGroups_[groupName].particles;

	particles.reserve(particles.size() + count);

	for (uint32_t i = 0; i < count; ++i) {
		// メッシュ表面上の点（モデルローカル）をワールドへ変換して発生させる
		Vector3 localPos = sampler->Sample(randomEngine);
		Vector3 worldPos = Transform(localPos, worldMatrix);
		particles.emplace_back(MakeNewParticle(groupName, worldPos));
	}
}