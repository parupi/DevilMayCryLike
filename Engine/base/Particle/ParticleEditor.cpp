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
        Vector3& minTranslate = global_->GetValueRef<Vector3>(groupName, "minTranslate");
        Vector3& maxTranslate = global_->GetValueRef<Vector3>(groupName, "maxTranslate");

        if (ImGui::TreeNode("Translate"))
        {
            ImGui::DragFloat3("Min Translate", &minTranslate.x, 0.1f);
            ImGui::DragFloat3("Max Translate", &maxTranslate.x, 0.1f);
            ImGui::TreePop();
        }

        Vector3& minRotate = global_->GetValueRef<Vector3>(groupName, "minRotate");
        Vector3& maxRotate = global_->GetValueRef<Vector3>(groupName, "maxRotate");

        if (ImGui::TreeNode("Rotate"))
        {
            ImGui::DragFloat3("Min Rotate", &minRotate.x, 0.1f);
            ImGui::DragFloat3("Max Rotate", &maxRotate.x, 0.1f);
            ImGui::TreePop();
        }

        // 以下同様に既存コードを貼り付けるだけ

        if (ImGui::Button("Save Particle"))
        {
            global_->SaveFile("Particle", groupName);
        }

        ImGui::TreePop();
    }
}

void ParticleEditor::DrawEmitterEditor()
{
    auto& emitters = manager_->GetEmitters();
    if (emitters.empty()) return;

    std::vector<const char*> names;
    std::vector<std::string> keys;

    for (auto& [name, emitter] : emitters)
    {
        names.push_back(name.c_str());
        keys.push_back(name);
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

        bool& emitAll = global_->GetValueRef<bool>(emitterName, "EmitAll");
        if (ImGui::Button("Emit Now"))
        {
            emitAll = true;
        }

        if (ImGui::Button("Save Emitter"))
        {
            global_->SaveFile("Particle", emitterName);
        }

        ImGui::TreePop();
    }
}

#endif