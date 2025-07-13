#include "ParticleEmitter.h"
#include "ParticleManager.h"

void ParticleEmitter::Initialize(std::string name_)
{
	emitter.name_ = name_;
	emitter.frequency = 0.5f;
	emitter.isActive = true;
	
	GlobalVariables::GetInstance()->AddItem(emitter.name_, "EmitPosition", Vector3{});
	GlobalVariables::GetInstance()->AddItem(emitter.name_, "Frequency", float{});
	GlobalVariables::GetInstance()->AddItem(emitter.name_, "IsActive", bool{});
	GlobalVariables::GetInstance()->AddItem(emitter.name_, "EmitAll", bool{});
	GlobalVariables::GetInstance()->AddItem(emitter.name_, "Count", int{});
}

void ParticleEmitter::Update(Vector3 Position, uint32_t count)
{
	count; Position;
	emitter.isActive = GlobalVariables::GetInstance()->GetValueRef<bool>(emitter.name_, "IsActive");

	//emitter.transform.translate = GlobalVariables::GetInstance()->GetValueRef<Vector3>(emitter.name_, "EmitPosition");
	emitter.frequency = GlobalVariables::GetInstance()->GetValueRef<float>(emitter.name_, "Frequency");

	emitter.count = GlobalVariables::GetInstance()->GetValueRef<int>(emitter.name_, "Count");

	if (emitter.isActive) {
		//emitter.transform.translate = GlobalVariables::GetInstance()->GetValueRef<Vector3>(emitter.name_, "EmitPosition");
		emitter.transform.translate = Position;
		emitter.frequency = GlobalVariables::GetInstance()->GetValueRef<float>(emitter.name_, "Frequency");

		emitter.count = GlobalVariables::GetInstance()->GetValueRef<int>(emitter.name_, "Count");
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

	emitAll_ = GlobalVariables::GetInstance()->GetValueRef<bool>(emitter.name_, "EmitAll");

	if (emitAll_) {
		Emit();
		GlobalVariables::GetInstance()->SetValue(emitter.name_, "EmitAll", false);
	}
}

void ParticleEmitter::Emit() const
{
	ParticleManager::GetInstance()->Emit(emitter.name_, emitter.transform.translate, emitter.count);
}
