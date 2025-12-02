#define NOMINMAX
#include "ParticleManager.h"
#include "math/function.h"
#include <numbers>
#include <debuger/ImGuiManager.h>
#include <algorithm> 
#include <base/utility/DeltaTime.h>
#include <base/TextureManager.h>

std::random_device seedGenerator;
std::mt19937 randomEngine(seedGenerator());

ParticleManager* ParticleManager::instance = nullptr;

ParticleManager* ParticleManager::GetInstance()
{
	if (instance == nullptr) {
		instance = new ParticleManager();
	}
	return instance;
}

void ParticleManager::Finalize()
{
	// パーティクルグループのインスタンシングリソースを解放
	for (auto& [name_, group] : particleGroups_) {
		//group.instancingResource.Reset();
		group.instancingDataPtr = nullptr;
		group.particleList.clear();
	}
	particleGroups_.clear();

	// パーティクル用のリソース
	//instancingResource_.Reset();
	//materialResource_.Reset();
	vertexResource.Reset();

	instancingData_ = nullptr;
	materialData_ = nullptr;
	vertexData_ = nullptr;

	// パーティクルパラメータ・α値もクリア
	particleParams_.clear();
	alpha_.clear();

	// 全体のパーティクルリストもクリア
	particles.clear();

	// カメラへの参照も解放（ただし所有権はない）
	camera_ = nullptr;

	// 外部への参照も切っておく（所有権は持っていないのでdeleteはしない）
	dxManager_ = nullptr;
	srvManager_ = nullptr;
	psoManager_ = nullptr;

	delete instance;
	instance = nullptr;

	// ログ出力など（任意）
	Logger::Log("ParticleManager finalized.\n");
}

void ParticleManager::Initialize(DirectXManager* dxManager, PSOManager* psoManager)
{
	dxManager_ = dxManager;
	srvManager_ = dxManager_->GetSrvManager();
	psoManager_ = psoManager;

	// リソースの生成と値の設定
	CreateParticleResource();
	CreateMaterialResource();

	// jsonファイルの読み込み
	global_->LoadFiles();
}


void ParticleManager::Update()
{
	if (camera_ == nullptr) return;

	Matrix4x4 cameraMatrix = MakeAffineMatrix({ 1.0f, 1.0f, 1.0f }, camera_->GetRotate(), camera_->GetTranslate());
	Matrix4x4 viewMatrix = Inverse(cameraMatrix);
	Matrix4x4 projectionMatrix = MakePerspectiveFovMatrix(0.45f, float(1280) / float(720), 0.1f, 100.0f);
	Matrix4x4 viewProjectionMatrix = viewMatrix * projectionMatrix;

	Matrix4x4 backToFrontMatrix = MakeRotateYMatrix(std::numbers::pi_v<float>);
	Matrix4x4 billboardMatrix = backToFrontMatrix * cameraMatrix;
	billboardMatrix.m[3][0] = 0.0f;
	billboardMatrix.m[3][1] = 0.0f;
	billboardMatrix.m[3][2] = 0.0f;

	for (auto& [groupName, particleGroup] : particleGroups_) {
		numInstance = 0;
		particleGroup.instanceCache.clear();
		particleGroup.instanceCache.reserve(particleGroup.particleList.size());

		for (auto particleIterator = particleGroup.particleList.begin(); particleIterator != particleGroup.particleList.end();) {
			if ((*particleIterator).lifeTime <= (*particleIterator).currentTime) {
				// パーティクルが生存時間を超えたら削除
				particleIterator = particleGroup.particleList.erase(particleIterator);
				continue;
			}

			if (numInstance >= kNumMaxInstance) {
				break;
			}

			// エディターで変更したBlendModeを設定しておく
			particleGroup.blendMode = static_cast<BlendMode>(global_->GetValueRef<int>(groupName, "BlendMode"));

			// パーティクルの更新処理
			float alpha = 1.0f;
			float t = (*particleIterator).currentTime / (*particleIterator).lifeTime;

			isBillboard_ = global_->GetValueRef<bool>(groupName, "IsBillboard");

			(*particleIterator).fadeType = static_cast<FadeType>(global_->GetValueRef<int>(groupName, "FadeType"));

			(*particleIterator).transform.translate += (*particleIterator).velocity * DeltaTime::GetDeltaTime();
			(*particleIterator).currentTime += DeltaTime::GetDeltaTime();

			switch ((*particleIterator).fadeType) {
			case FadeType::Alpha: {
				// 透明度でフェードアウト
				alpha = 1.0f - t;
				break;
			}
			case FadeType::ScaleShrink: {
				// 寿命の90%を過ぎたら急速に縮む
				const float shrinkStart = global_->GetValueRef<float>(groupName, "ShrinkStartRatio");
				if (t > shrinkStart) {
					float progress = (t - shrinkStart) / (1.0f - shrinkStart);
					float scaleFactor = std::max(0.0f, 1.0f - progress * 1.0f); // 線形縮小
					(*particleIterator).transform.scale = (*particleIterator).initialScale * scaleFactor;
				} else {
					(*particleIterator).initialScale = (*particleIterator).transform.scale;
				}
				break;
			}
			case FadeType::None:
			default:
				// 何もしない（そのまま表示）
				break;
			}

			// ワールド行列の計算
			scaleMatrix = MakeScaleMatrix((*particleIterator).transform.scale);
			translateMatrix = MakeTranslateMatrix((*particleIterator).transform.translate);
			Matrix4x4 worldMatrix{};
			if (isBillboard_) {
				worldMatrix = scaleMatrix * billboardMatrix * translateMatrix;
			} else {
				worldMatrix = MakeAffineMatrix((*particleIterator).transform.scale, (*particleIterator).transform.rotate, (*particleIterator).transform.translate);
			}
			Matrix4x4 worldViewProjectionMatrix = worldMatrix * viewProjectionMatrix;

			// ★ CPUキャッシュへ詰める
			if (numInstance < kNumMaxInstance) {
				InstanceData id{};
				id.world = worldMatrix;
				id.wvp = worldViewProjectionMatrix;
				id.color = (*particleIterator).color;
				id.color.w = alpha;
				particleGroup.instanceCache.push_back(id);

				// 既存のGPU配列も今は維持（後で削除予定）
				instancingData_[numInstance].WVP = worldViewProjectionMatrix;
				instancingData_[numInstance].World = worldMatrix;
				instancingData_[numInstance].color = (*particleIterator).color;
				instancingData_[numInstance].color.w = alpha;

				++numInstance;
			}

			std::vector<InstanceData> instanceList;
			for (auto& particle : particleGroup.particleList) {
				InstanceData data{};
				data.world = worldMatrix;
				data.wvp = worldViewProjectionMatrix;
				data.color = particle.color;
				data.color.w = alpha;
				instanceList.push_back(data);
			}
			particleGroup.renderer->SetInstanceList(instanceList);

			++particleIterator;
		}

		// インスタンス数の更新
		particleGroup.instanceCount = numInstance;

		// GPU メモリにインスタンスデータを書き込む
		if (particleGroup.instancingDataPtr) {
			std::memcpy(particleGroup.instancingDataPtr, instancingData_, sizeof(ParticleForGPU) * numInstance);
		}

		particleParams_[groupName] = LoadParticleParameters(global_, groupName);

#ifdef _DEBUG
		DrawEditor(global_, groupName);
#endif // _DEBUG
	}
}

void ParticleManager::Draw()
{
	auto commandList = dxManager_->GetCommandList();

	// プリミティブ形状設定
	commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	commandList->IASetVertexBuffers(0, 1, &vertexBufferView_);

	// すべてのパーティクルグループを描画
	for (auto& [groupName, particleGroup] : particleGroups_) {
		if (particleGroup.instanceCount == 0) {
			continue;
		}

		// === 各グループ専用ブレンドモードに切り替え ===
		commandList->SetPipelineState(psoManager_->GetParticlePSO(particleGroup.blendMode));
		
		commandList->SetGraphicsRootSignature(psoManager_->GetParticleSignature());

		// === 定数バッファ設定 ===
		commandList->SetGraphicsRootConstantBufferView(0, dxManager_->GetResourceManager()->GetGPUVirtualAddress(materialHandle_));

		// === SRV設定 ===
		srvManager_->SetGraphicsRootDescriptorTable(1, particleGroup.srvIndex);

		// === テクスチャ設定 ===
		srvManager_->SetGraphicsRootDescriptorTable(2, TextureManager::GetInstance()->GetTextureIndexByFilePath(particleGroup.materialData.textureFilePath));

		// === 描画 ===
		particleGroup.renderer->Draw();
	}
}


void ParticleManager::CreateParticleGroup(const std::string name, const std::string textureFilePath)
{
	if (particleGroups_.contains(name)) {
		// 登録済みの名前なら早期リターン
		return;
	}

	// グループを追加
	particleGroups_[name] = ParticleGroup();
	ParticleGroup& particleGroup = particleGroups_[name];

	// テクスチャを読み込む（未読み込みならロードする）
	particleGroup.materialData.textureFilePath = textureFilePath;
	TextureManager::GetInstance()->LoadTexture(particleGroup.materialData.textureFilePath);
	particleGroup.materialData.textureIndex = TextureManager::GetInstance()->GetTextureIndexByFilePath(textureFilePath);

	// レンダラーを生成
	particleGroup.renderer = std::make_unique<InstancingRenderer>(name, PrimitiveType::Plane, textureFilePath);

	auto* rm = dxManager_->GetResourceManager();

	particleGroup.instancingHandle = rm->CreateUploadBuffer(sizeof(ParticleForGPU) * kNumMaxInstance, L"ParticleInstancing");

	particleGroup.instancingDataPtr = reinterpret_cast<ParticleForGPU*>(rm->Map(particleGroup.instancingHandle));


	// SRVを作成するDescriptorの場所を決める
	particleGroup.srvIndex = srvManager_->Allocate();

	// SRVの生成
	srvManager_->CreateSRVforStructuredBuffer(particleGroup.srvIndex, rm->GetResource(particleGroup.instancingHandle), kNumMaxInstance, sizeof(ParticleForGPU));

	// インスタンス数を初期化
	particleGroup.instanceCount = 0;

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

	global_->AddItem(name, "BlendMode", int{2});
}

void ParticleManager::DrawSet(BlendMode blendMode)
{
	dxManager_->GetCommandList()->SetPipelineState(psoManager_->GetParticlePSO(blendMode));
	dxManager_->GetCommandList()->SetGraphicsRootSignature(psoManager_->GetParticleSignature());
	dxManager_->GetCommandList()->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
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
	instancingHandle_ =
		rm->CreateUploadBuffer(sizeof(ParticleForGPU) * kNumMaxInstance, L"ParticleGlobalInstancing");

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

ParticleManager::Particle ParticleManager::MakeNewParticle(const std::string name_/*, std::mt19937& randomEngine*/, const Vector3& translate)
{
	Particle particle{};

	auto& params = particleParams_[name_];

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

	return particle;
}

ParticleManager::ParticleParameters ParticleManager::LoadParticleParameters(GlobalVariables* global, const std::string& groupName)
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

	return params;
}

void ParticleManager::DrawEditor(GlobalVariables* global, const std::string& groupName)
{
	ImGui::Begin(groupName.c_str());

	if (ImGui::TreeNode("Particle")) {
		// ====== Translate ======
		Vector3& minTranslate = global->GetValueRef<Vector3>(groupName, "minTranslate");
		Vector3& maxTranslate = global->GetValueRef<Vector3>(groupName, "maxTranslate");
		if (ImGui::TreeNode("Translate")) {
			ImGui::DragFloat3("Min Translate", &minTranslate.x, 0.1f);
			ImGui::DragFloat3("Max Translate", &maxTranslate.x, 0.1f);
			ImGui::TreePop();
		}

		// ====== Rotate ======
		Vector3& minRotate = global->GetValueRef<Vector3>(groupName, "minRotate");
		Vector3& maxRotate = global->GetValueRef<Vector3>(groupName, "maxRotate");
		if (ImGui::TreeNode("Rotate")) {
			ImGui::DragFloat3("Min Rotate", &minRotate.x, 0.1f);
			ImGui::DragFloat3("Max Rotate", &maxRotate.x, 0.1f);
			ImGui::TreePop();
		}

		// ====== Scale ======
		Vector3& minScale = global->GetValueRef<Vector3>(groupName, "minScale");
		Vector3& maxScale = global->GetValueRef<Vector3>(groupName, "maxScale");
		if (ImGui::TreeNode("Scale")) {
			ImGui::DragFloat3("Min Scale", &minScale.x, 0.1f);
			ImGui::DragFloat3("Max Scale", &maxScale.x, 0.1f);
			ImGui::TreePop();
		}

		// ====== Velocity ======
		Vector3& minVelocity = global->GetValueRef<Vector3>(groupName, "minVelocity");
		Vector3& maxVelocity = global->GetValueRef<Vector3>(groupName, "maxVelocity");
		if (ImGui::TreeNode("Velocity")) {
			ImGui::DragFloat3("Min Velocity", &minVelocity.x, 0.1f);
			ImGui::DragFloat3("Max Velocity", &maxVelocity.x, 0.1f);
			ImGui::TreePop();
		}

		// ====== LifeTime ======
		float& minLifeTime = global->GetValueRef<float>(groupName, "minLifeTime");
		float& maxLifeTime = global->GetValueRef<float>(groupName, "maxLifeTime");
		if (ImGui::TreeNode("LifeTime")) {
			ImGui::DragFloat("Min LifeTime", &minLifeTime, 0.1f, 0.0f, 9999.0f);
			ImGui::DragFloat("Max LifeTime", &maxLifeTime, 0.1f, 0.0f, 9999.0f);
			ImGui::TreePop();
		}

		// ====== Color ======
		Vector3& minColor = global->GetValueRef<Vector3>(groupName, "minColor");
		Vector3& maxColor = global->GetValueRef<Vector3>(groupName, "maxColor");
		if (ImGui::TreeNode("Color")) {
			ImGui::ColorEdit3("Min Color", &minColor.x);
			ImGui::ColorEdit3("Max Color", &maxColor.x);
			ImGui::TreePop();
		}

		bool& isBillboard = global->GetValueRef<bool>(groupName, "IsBillboard");
		if (ImGui::TreeNode("Billboard")) {
			ImGui::Checkbox("IsBillboard", &isBillboard);
			ImGui::TreePop();
		}

		// ====== FadeType ======
		if (ImGui::TreeNode("Fade Settings")) {

			int& fadeTypeInt = global->GetValueRef<int>(groupName, "FadeType");
			const char* fadeTypeNames[] = {
				"None", "Alpha", "ScaleShrink"
			};

			// コンボで選択
			ImGui::Combo("Fade Type", &fadeTypeInt, fadeTypeNames, IM_ARRAYSIZE(fadeTypeNames));

			FadeType fadeType = static_cast<FadeType>(fadeTypeInt);

			// フェードタイプごとに個別パラメータを出す
			switch (fadeType) {
			case FadeType::Alpha:
				
				break;
			case FadeType::ScaleShrink: {
				// このフェードタイプ専用のパラメータ
				float& shrinkStart = global->GetValueRef<float>(groupName, "ShrinkStartRatio");

				ImGui::SliderFloat("Shrink Start Ratio", &shrinkStart, 0.0f, 1.0f);
				break;
			}
			case FadeType::None:
			default:
				
				break;
			}
			ImGui::TreePop();
		}

		// ===== Blend Settings =====
		if (ImGui::TreeNode("Blend Mode")) {
			int& blendModeInt = global->GetValueRef<int>(groupName, "BlendMode");
			const char* blendNames[] = {"None", "Normal", "Add", "Subtract", "Multiply", "Screen"};
			ImGui::Combo("Blend Mode", &blendModeInt, blendNames, IM_ARRAYSIZE(blendNames));
			ImGui::TreePop();
		}


		ImGui::TreePop();
	}

	ImGui::End();
}

std::list<ParticleManager::Particle> ParticleManager::Emit(const std::string name, const Vector3& position, uint32_t count)
{
	ParticleGroup& particleGroup = particleGroups_[name];
	std::list<Particle> newParticles;
	for (uint32_t nowCount = 0; nowCount < count; ++nowCount) {
		Particle particle = MakeNewParticle(name, position);
		newParticles.push_back(particle);
	}
	particleGroup.particleList.splice(particleGroup.particleList.end(), newParticles);
	return newParticles;
}