#include "SpotLight.h"
#ifdef USE_IMGUI
#include <imgui.h>
#endif // IMGUI

SpotLight::SpotLight(const std::string& name) : BaseLight(name)
{
    // エディター項目登録
    global_->AddItem(name, "Color", Vector4{ 1, 1, 1, 1 });
    global_->AddItem(name, "Position", Vector3{ 0, 0, 0 });
    global_->AddItem(name, "Direction", Vector3{ 0, -1, 0 });
    global_->AddItem(name, "Intensity", 1.0f);
    global_->AddItem(name, "Distance", 20.0f);
    global_->AddItem(name, "Decay", 1.0f);
    global_->AddItem(name, "CosAngle", 0.9f); // 角度の余弦値（0.9 ≒ 約25°）
    global_->AddItem(name, "Enabled", true);
}

void SpotLight::Update()
{
#ifdef _DEBUG
    DrawLightEditor();
#endif // DEBUG

    lightData_.enabled = global_->GetValueRef<bool>(name_, "Enabled");
    lightData_.color = global_->GetValueRef<Vector4>(name_, "Color");
    lightData_.position = global_->GetValueRef<Vector3>(name_, "Position");
    lightData_.direction = global_->GetValueRef<Vector3>(name_, "Direction");
    lightData_.intensity = global_->GetValueRef<float>(name_, "Intensity");
    lightData_.distance = global_->GetValueRef<float>(name_, "Distance");
    lightData_.decay = global_->GetValueRef<float>(name_, "Decay");
    lightData_.cosAngle = global_->GetValueRef<float>(name_, "CosAngle");
}

void SpotLight::DrawLightEditor()
{
#ifdef USE_IMGUI
    ImGui::Begin(name_.c_str());

    if (ImGui::TreeNode("Spot Light Settings")) {

        bool& enabled = global_->GetValueRef<bool>(name_, "Enabled");
        ImGui::Checkbox("Enabled", &enabled);

        Vector4& color = global_->GetValueRef<Vector4>(name_, "Color");
        ImGui::ColorEdit4("Color", &color.x);

        Vector3& position = global_->GetValueRef<Vector3>(name_, "Position");
        ImGui::DragFloat3("Position", &position.x, 0.1f);

        Vector3& direction = global_->GetValueRef<Vector3>(name_, "Direction");
        ImGui::DragFloat3("Direction", &direction.x, 0.01f, -1.0f, 1.0f);

        float& intensity = global_->GetValueRef<float>(name_, "Intensity");
        ImGui::DragFloat("Intensity", &intensity, 0.01f, 0.0f, 100.0f);

        float& distance = global_->GetValueRef<float>(name_, "Distance");
        ImGui::DragFloat("Distance", &distance, 0.1f, 0.0f, 1000.0f);

        float& decay = global_->GetValueRef<float>(name_, "Decay");
        ImGui::DragFloat("Decay", &decay, 0.01f, 0.0f, 10.0f);

        float& cosAngle = global_->GetValueRef<float>(name_, "CosAngle");
        ImGui::SliderFloat("Cos Angle", &cosAngle, 0.0f, 1.0f);

        ImGui::TreePop();
    }

    if (ImGui::Button("Save##SpotLight")) {
        global_->SaveFile(name_);
    }

    ImGui::End();
#endif // IMGUI
}

void SpotLight::CreateLightResource()
{
    dxManager_->CreateBufferResource(sizeof(SpotLightData), resource_);
}

void SpotLight::UpdateLightResource()
{
    if (!isDirty_) return;

    void* mapped = nullptr;
    resource_->Map(0, nullptr, &mapped);
    std::memcpy(mapped, &lightData_, sizeof(SpotLightData));
    resource_->Unmap(0, nullptr);
}

void SpotLight::SerializeTo(void* dest) const
{
    std::memcpy(dest, &lightData_, sizeof(SpotLightData));
}
