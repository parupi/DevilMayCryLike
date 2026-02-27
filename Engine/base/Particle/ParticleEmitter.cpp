#include "ParticleEmitter.h"
#include "ParticleManager.h"
#include "debuger/GlobalVariables.h"
#include <filesystem>
#include <fstream>
#include <nlohmann/json.hpp>

void ParticleEmitter::Initialize(ParticleManager* particleManager, const std::string& name)
{
	particleManager_ = particleManager;

	emitter.name = name;
	emitter.frequency = 0.5f;
	emitter.isActive = true;

	transform_ = std::make_unique<WorldTransform>();
	transform_->Initialize();

	GlobalVariables::GetInstance()->AddItem(emitter.name, "EmitPosition", Vector3{});
	GlobalVariables::GetInstance()->AddItem(emitter.name, "Frequency", float{});
	GlobalVariables::GetInstance()->AddItem(emitter.name, "IsActive", bool{});
	GlobalVariables::GetInstance()->AddItem(emitter.name, "EmitAll", bool{});
	GlobalVariables::GetInstance()->AddItem(emitter.name, "Count", int{});

	Load("Resource/Emitter/" + name + ".json");
}

void ParticleEmitter::Update(Vector3 position)
{
	emitter.isActive = GlobalVariables::GetInstance()->GetValueRef<bool>(emitter.name, "IsActive");

	emitter.frequency = GlobalVariables::GetInstance()->GetValueRef<float>(emitter.name, "Frequency");

	emitter.count = GlobalVariables::GetInstance()->GetValueRef<int>(emitter.name, "Count");

	if (!transform_->GetParent()) {
		emitter.transform.translate = GlobalVariables::GetInstance()->GetValueRef<Vector3>(emitter.name, "EmitPosition");
	} else {
		emitter.transform.translate = transform_->GetWorldPos();
	}

	if (emitter.isActive) {
		emitter.frequency = GlobalVariables::GetInstance()->GetValueRef<float>(emitter.name, "Frequency");

		emitter.count = GlobalVariables::GetInstance()->GetValueRef<int>(emitter.name, "Count");
		if (emitter.count < 0) {
			emitter.count = 0;
		}

		emitter.frequencyTime += DeltaTime::GetDeltaTime();
		if (emitter.frequency <= emitter.frequencyTime) {
			// パーティクルを生成してグループに追加
			Emit();
			emitter.frequencyTime -= emitter.frequency;
		}
	}

	emitAll_ = GlobalVariables::GetInstance()->GetValueRef<bool>(emitter.name, "EmitAll");

	if (emitAll_) {
		Emit();
		GlobalVariables::GetInstance()->SetValue(emitter.name, "EmitAll", false);
	}
}

void ParticleEmitter::Emit()
{
	Vector3 position = transform_->GetWorldPos();

	if (!transform_->GetParent()) {
		position = GlobalVariables::GetInstance()->GetValueRef<Vector3>(emitter.name, "EmitPosition");
	} else {
		position = transform_->GetWorldPos();
	}

	for (auto& p : particles_)
	{
		int finalCount = static_cast<int>(p.count * p.spawnRate);

		if (finalCount > 0)
		{
			particleManager_->Emit(p.name, position, finalCount);
		}
	}
}

void ParticleEmitter::AddParticle(const std::string& name)
{
	EmitterParticle p;
	p.name = name;
	p.count = 1;
	p.spawnRate = 1.0f;

	particles_.push_back(p);
}

void ParticleEmitter::RemoveParticle(size_t index)
{
	if (index < particles_.size())
	{
		particles_.erase(particles_.begin() + index);
	}
}

void ParticleEmitter::Save(const std::string& path)
{
	namespace fs = std::filesystem;

	// パスをfilesystem形式に変換
	fs::path filePath(path);

	// 親ディレクトリ取得
	fs::path directory = filePath.parent_path();

	// ディレクトリが存在しなければ作成（再帰）
	if (!directory.empty() && !fs::exists(directory))
	{
		fs::create_directories(directory);
	}

	// JSON構築
	nlohmann::json j;

	j["EmitterName"] = emitter.name;
	j["Frequency"] = emitter.frequency;
	j["IsActive"] = emitter.isActive;

	j["Particles"] = nlohmann::json::array();

	for (auto& p : particles_)
	{
		nlohmann::json particleJson;
		particleJson["ParticleName"] = p.name;
		particleJson["Count"] = p.count;
		particleJson["SpawnRate"] = p.spawnRate;

		j["Particles"].push_back(particleJson);
	}

	// ファイル出力
	std::ofstream file(filePath);
	if (!file.is_open())
	{
		return;
	}

	file << j.dump(4);
}

void ParticleEmitter::Load(const std::string& path)
{
	std::ifstream file(path);
	if (!file.is_open()) return;

	nlohmann::json j;
	file >> j;

	emitter.frequency = j.value("Frequency", 0.5f);
	emitter.isActive = j.value("IsActive", true);

	particles_.clear();

	for (auto& particleJson : j["Particles"])
	{
		EmitterParticle p;

		p.name = particleJson.value("ParticleName", "");
		p.count = particleJson.value("Count", 1);
		p.spawnRate = particleJson.value("SpawnRate", 1.0f);

		particles_.push_back(p);
	}
}
