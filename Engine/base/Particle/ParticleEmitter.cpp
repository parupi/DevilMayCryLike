#include "ParticleEmitter.h"
#include "ParticleManager.h"

void ParticleEmitter::Initialize(std::string name)
{
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

#ifdef _DEBUG
	UpdateParam();
#endif // _DEBUG
}

void ParticleEmitter::Emit()
{
	if (!transform_->GetParent()) {
		emitter.transform.translate = GlobalVariables::GetInstance()->GetValueRef<Vector3>(emitter.name, "EmitPosition");
	} else {
		emitter.transform.translate = transform_->GetWorldPos();
	}

	ParticleManager::GetInstance()->Emit(particleName_, emitter.transform.translate, emitter.count);
}

void ParticleEmitter::UpdateParam() const
{
#ifdef USE_IMGUI
	ImGui::Begin(emitter.name.c_str());
	if (ImGui::TreeNode("Emitter")) {
		// Emit Position
		Vector3& emitPos = GlobalVariables::GetInstance()->GetValueRef<Vector3>(emitter.name, "EmitPosition");
		if (ImGui::TreeNode("Emitter Position")) {
			ImGui::DragFloat3("Emit Position", &emitPos.x, 0.1f);
			ImGui::TreePop();
		}

		// Frequency
		float& frequency = GlobalVariables::GetInstance()->GetValueRef<float>(emitter.name, "Frequency");
		if (ImGui::TreeNode("Frequency")) {
			ImGui::DragFloat("Emit Frequency", &frequency, 0.01f, 0.01f, 1000.0f);
			ImGui::TreePop();
		}

		// Count
		int& count = GlobalVariables::GetInstance()->GetValueRef<int>(emitter.name, "Count");
		if (ImGui::TreeNode("Count")) {
			ImGui::DragInt("Emit Count", &count, 1, 0, 1000);
			ImGui::TreePop();
		}

		// IsActive
		bool& isActive = GlobalVariables::GetInstance()->GetValueRef<bool>(emitter.name, "IsActive");
		ImGui::Checkbox("Is Active", &isActive);

		// EmitAll (一回限りの即時発生ボタン風)
		bool& emitAll = GlobalVariables::GetInstance()->GetValueRef<bool>(emitter.name, "EmitAll");
		if (ImGui::Button("Emit All")) {
			emitAll = true; // Update() 側で処理される
		}
		ImGui::TreePop();
	}

	// ====== Save ======
	if (ImGui::Button("Save")) {
		GlobalVariables::GetInstance()->SaveFile(emitter.name);
		std::string message = std::format("{}.json saved.", emitter.name);
		MessageBoxA(nullptr, message.c_str(), "GlobalVariables", 0);
	}

	ImGui::End();
#endif // IMGUI
}
