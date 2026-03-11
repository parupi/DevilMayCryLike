#ifdef _DEBUG

#include "ParticleEditor.h"
#include "ParticleManager.h"
#include "ParticleEmitter.h"
#include "debuger/GlobalVariables.h"
#include <imgui.h>
#include <format>
#include <Windows.h>

void ParticleEditor::Initialize(ParticleManager* manager)
{
	manager_ = manager;
	global_ = GlobalVariables::GetInstance();
}

void ParticleEditor::Draw()
{
	ImGui::Begin("Particle Editor");

	if (ImGui::CollapsingHeader("Particles", ImGuiTreeNodeFlags_DefaultOpen)) 
	{
		DrawParticleEditor();
	}

	if (ImGui::CollapsingHeader("Emitters", ImGuiTreeNodeFlags_DefaultOpen))
	{
		DrawEmitterEditor();
	}

	ImGui::End();
}

void ParticleEditor::DrawParticleEditor()
{
	ImGui::Separator();
	ImGui::Text("Create New Particle");

	static char newParticleName[64] = "";
	static char newTexturePath[128] = "";

	ImGui::InputText("Particle Name", newParticleName, IM_ARRAYSIZE(newParticleName));
	ImGui::InputText("Texture Path", newTexturePath, IM_ARRAYSIZE(newTexturePath));

	if (ImGui::Button("Create Particle"))
	{
		if (strlen(newParticleName) > 0)
		{
			manager_->CreateParticleGroup(newParticleName, newTexturePath);

			newParticleName[0] = '\0';
			newTexturePath[0] = '\0';
		}
	}

	ImGui::Separator();

	auto& groups = manager_->GetParticleGroups();

	if (groups.empty()) return;

	// 名前リスト作成
	std::vector<const char*> names;
	std::vector<std::string> keys;

	for (auto& [name, group] : groups)
	{
		names.push_back(name.c_str());
		keys.push_back(name);
	}

	ImGui::Combo("Particle Group", &selectedParticleIndex_, names.data(), (int)names.size());

	if (selectedParticleIndex_ >= keys.size()) return;

	const std::string& groupName = keys[selectedParticleIndex_];

	// ===== ここから既存DrawEditor移植 =====

	if (ImGui::TreeNode("Particle Settings"))
	{
		// ====== Translate ======
		Vector3& minTranslate = global_->GetValueRef<Vector3>(groupName, "minTranslate");
		Vector3& maxTranslate = global_->GetValueRef<Vector3>(groupName, "maxTranslate");
		if (ImGui::TreeNode("Translate")) {
			ImGui::DragFloat3("Min Translate", &minTranslate.x, 0.1f);
			ImGui::DragFloat3("Max Translate", &maxTranslate.x, 0.1f);
			ImGui::TreePop();
		}

		// ====== Rotate ======
		Vector3& minRotate = global_->GetValueRef<Vector3>(groupName, "minRotate");
		Vector3& maxRotate = global_->GetValueRef<Vector3>(groupName, "maxRotate");
		if (ImGui::TreeNode("Rotate")) {
			ImGui::DragFloat3("Min Rotate", &minRotate.x, 0.1f);
			ImGui::DragFloat3("Max Rotate", &maxRotate.x, 0.1f);
			ImGui::TreePop();
		}

		// ====== Scale ======
		Vector3& minScale = global_->GetValueRef<Vector3>(groupName, "minScale");
		Vector3& maxScale = global_->GetValueRef<Vector3>(groupName, "maxScale");
		if (ImGui::TreeNode("Scale")) {
			ImGui::DragFloat3("Min Scale", &minScale.x, 0.1f);
			ImGui::DragFloat3("Max Scale", &maxScale.x, 0.1f);
			ImGui::TreePop();
		}

		// ====== Velocity ======
		Vector3& minVelocity = global_->GetValueRef<Vector3>(groupName, "minVelocity");
		Vector3& maxVelocity = global_->GetValueRef<Vector3>(groupName, "maxVelocity");
		if (ImGui::TreeNode("Velocity")) {
			ImGui::DragFloat3("Min Velocity", &minVelocity.x, 0.1f);
			ImGui::DragFloat3("Max Velocity", &maxVelocity.x, 0.1f);
			ImGui::TreePop();
		}

		// ====== LifeTime ======
		float& minLifeTime = global_->GetValueRef<float>(groupName, "minLifeTime");
		float& maxLifeTime = global_->GetValueRef<float>(groupName, "maxLifeTime");
		if (ImGui::TreeNode("LifeTime")) {
			ImGui::DragFloat("Min LifeTime", &minLifeTime, 0.1f, 0.0f, 9999.0f);
			ImGui::DragFloat("Max LifeTime", &maxLifeTime, 0.1f, 0.0f, 9999.0f);
			ImGui::TreePop();
		}

		// ====== Color ======
		Vector3& minColor = global_->GetValueRef<Vector3>(groupName, "minColor");
		Vector3& maxColor = global_->GetValueRef<Vector3>(groupName, "maxColor");
		if (ImGui::TreeNode("Color")) {
			ImGui::ColorEdit3("Min Color", &minColor.x);
			ImGui::ColorEdit3("Max Color", &maxColor.x);
			ImGui::TreePop();
		}

		bool& isBillboard = global_->GetValueRef<bool>(groupName, "IsBillboard");
		if (ImGui::TreeNode("Billboard")) {
			ImGui::Checkbox("IsBillboard", &isBillboard);
			ImGui::TreePop();
		}

		ImGui::TreePop();
	}

	// ====== FadeType ======
	if (ImGui::TreeNode("Fade Settings")) {

		int& fadeTypeInt = global_->GetValueRef<int>(groupName, "FadeType");
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
			float& shrinkStart = global_->GetValueRef<float>(groupName, "ShrinkStartRatio");

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
		int& blendModeInt = global_->GetValueRef<int>(groupName, "BlendMode");
		const char* blendNames[] = { "None", "Normal", "Add", "Subtract", "Multiply", "Screen" };
		ImGui::Combo("Blend Mode", &blendModeInt, blendNames, IM_ARRAYSIZE(blendNames));
		ImGui::TreePop();
	}

	if (ImGui::Button("Save Particle"))
	{
		global_->SaveFile("Particle", groupName);
	}
}

void ParticleEditor::DrawEmitterEditor()
{
	// ===== New Emitter Create =====
	ImGui::Separator();
	ImGui::Text("Create New Emitter");

	static char newEmitterName[64] = "";

	ImGui::InputText("Emitter Name", newEmitterName, IM_ARRAYSIZE(newEmitterName));

	if (ImGui::Button("Create Emitter"))
	{
		if (strlen(newEmitterName) > 0)
		{
			manager_->CreateEmitter(newEmitterName);

			newEmitterName[0] = '\0';
		}
	}

	ImGui::Separator();

	auto& emitters = manager_->GetEmitters();
	if (emitters.empty()) return;

	std::vector<const char*> names;
	std::vector<std::string> keys;

	for (auto& [name, emitter] : emitters)
	{
		names.push_back(name.c_str());
		keys.push_back(name);
	}

	ParticleEmitter* emitter_ptr = nullptr;

	if (selectedEmitterIndex_ < keys.size())
	{
		emitter_ptr = emitters.at(keys[selectedEmitterIndex_]).get();
	}

	ImGui::Combo("Emitter", &selectedEmitterIndex_, names.data(), (int)names.size());

	if (selectedEmitterIndex_ >= keys.size()) return;

	const std::string& emitterName = keys[selectedEmitterIndex_];

	if (ImGui::TreeNode("Emitter Settings"))
	{
		Vector3& emitPos = global_->GetValueRef<Vector3>(emitterName, "EmitPosition");
		ImGui::DragFloat3("Emit Position", &emitPos.x, 0.1f);

		float& frequency = global_->GetValueRef<float>(emitterName, "Frequency");
		ImGui::DragFloat("Frequency", &frequency, 0.01f, 0.01f, 1000.0f);

		int& count = global_->GetValueRef<int>(emitterName, "Count");
		ImGui::DragInt("Count", &count, 1, 0, 1000);

		bool& isActive = global_->GetValueRef<bool>(emitterName, "IsActive");
		ImGui::Checkbox("Is Active", &isActive);

		ImGui::TreePop();
	}

	if (ImGui::TreeNode("Particles")) {

		auto& particles = emitter_ptr->GetParticles();

		for (int i = 0; i < particles.size(); ++i)
		{
			ImGui::PushID(i);

			ImGui::Separator();

			ImGui::Text("Particle: %s", particles[i].name.c_str());

			ImGui::DragInt("Count", &particles[i].count, 1, 0, 1000);
			ImGui::DragFloat("SpawnRate", &particles[i].spawnRate, 0.01f, 0.0f, 10.0f);

			if (ImGui::Button("Remove"))
			{
				emitter_ptr->RemoveParticle(i);
				ImGui::PopID();
				break;
			}

			ImGui::PopID();
		}

		const auto& groups = manager_->GetParticleGroups();
		ImGui::Separator();
		ImGui::Text("Add Particle");

		static int selectedIndex = 0;

		std::vector<const char*> names;
		std::vector<std::string> keys;

		for (auto& [name, group] : groups)
		{
			names.push_back(name.c_str());
			keys.push_back(name);
		}

		ImGui::Combo("Particle", &selectedIndex, names.data(), (int)names.size());

		if (ImGui::Button("Add"))
		{
			emitter_ptr->AddParticle(keys[selectedIndex]);
		}

		ImGui::TreePop();

		ImGui::Separator();

		if (ImGui::Button("Save Emitter"))
		{
			emitter_ptr->Save("Resource/Emitter/" + emitterName + ".json");
		}

		ImGui::SameLine();

		if (ImGui::Button("Load Emitter"))
		{
			emitter_ptr->Load("Resource/Emitter/" + emitterName + ".json");
		}
	}

	ImGui::Separator();

	bool& emitAll = global_->GetValueRef<bool>(emitterName, "EmitAll");
	if (ImGui::Button("Emit Now"))
	{
		emitAll = true;
	}

	if (ImGui::Button("Save Emitter"))
	{
		global_->SaveFile("Particle", emitterName);
	}
}

#endif