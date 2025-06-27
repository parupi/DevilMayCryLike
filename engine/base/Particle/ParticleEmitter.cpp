#include "ParticleEmitter.h"
#include "ParticleManager.h"

void ParticleEmitter::Initialize(std::string name)
{
	emitter.name = name;
	emitter.frequency = 0.5f;
	emitter.isActive = true;
	
	GlobalVariables::GetInstance()->AddItem(emitter.name, "EmitPosition", Vector3{});
	GlobalVariables::GetInstance()->AddItem(emitter.name, "Frequency", float{});
	GlobalVariables::GetInstance()->AddItem(emitter.name, "IsActive", bool{});
	GlobalVariables::GetInstance()->AddItem(emitter.name, "EmitAll", bool{});
	GlobalVariables::GetInstance()->AddItem(emitter.name, "Count", int{});
}

void ParticleEmitter::Update(Vector3 Position, uint32_t count)
{
	emitter.isActive = GlobalVariables::GetInstance()->GetValueRef<bool>(emitter.name, "IsActive");

	emitter.transform.translate = GlobalVariables::GetInstance()->GetValueRef<Vector3>(emitter.name, "EmitPosition");
	emitter.frequency = GlobalVariables::GetInstance()->GetValueRef<float>(emitter.name, "Frequency");

	emitter.count = GlobalVariables::GetInstance()->GetValueRef<int>(emitter.name, "Count");

	if (emitter.isActive) {
		emitter.transform.translate = GlobalVariables::GetInstance()->GetValueRef<Vector3>(emitter.name, "EmitPosition");
		emitter.frequency = GlobalVariables::GetInstance()->GetValueRef<float>(emitter.name, "Frequency");

		emitter.count = GlobalVariables::GetInstance()->GetValueRef<int>(emitter.name, "Count");
		if (emitter.count < 0) {
			emitter.count = 0;
		}

		emitter.frequencyTime += kDeltaTime;
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

void ParticleEmitter::Emit() const
{
	ParticleManager::GetInstance()->Emit(emitter.name, emitter.transform.translate, emitter.count);
}
