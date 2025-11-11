#include "DirectionalLight.h"
#ifdef USE_IMGUI
#include <imgui.h>
#endif // IMGUI

DirectionalLight::DirectionalLight(const std::string& name) : BaseLight(name)
{
    // エディター項目登録
    global_->AddItem(name, "Color", Vector4{ 1, 1, 1, 1 });
    global_->AddItem(name, "Direction", Vector3{ 0, -1, 0 });
    global_->AddItem(name, "Intensity", 1.0f);
    global_->AddItem(name, "Enabled", true);
}

void DirectionalLight::Update()
{
#ifdef _DEBUG
    DrawLightEditor();
#endif // DEBUG

    lightData_.enabled = global_->GetValueRef<bool>(name_, "Enabled");
    lightData_.color = global_->GetValueRef<Vector4>(name_, "Color");
    lightData_.direction = global_->GetValueRef<Vector3>(name_, "Direction");
    lightData_.intensity = global_->GetValueRef<float>(name_, "Intensity");
}

void DirectionalLight::DrawLightEditor()
{
#ifdef USE_IMGUI
    ImGui::Begin(name_.c_str());

    if (ImGui::TreeNode("Directional Light Settings")) {

        bool& enabled = global_->GetValueRef<bool>(name_, "Enabled");
        ImGui::Checkbox("Enabled", &enabled);

        Vector4& color = global_->GetValueRef<Vector4>(name_, "Color");
        ImGui::ColorEdit4("Color", &color.x);

        Vector3& direction = global_->GetValueRef<Vector3>(name_, "Direction");
        ImGui::DragFloat3("Direction", &direction.x, 0.01f, -1.0f, 1.0f);

        float& intensity = global_->GetValueRef<float>(name_, "Intensity");
        ImGui::DragFloat("Intensity", &intensity, 0.01f, 0.0f, 100.0f);

        ImGui::TreePop();
    }

    if (ImGui::Button("Save##DirectionalLight")) {
        global_->SaveFile(name_);
    }

    ImGui::End();
#endif // IMGUI
}

void DirectionalLight::CreateLightResource()
{
    dxManager_->CreateBufferResource(sizeof(DirectionalLightData), resource_);
}

void DirectionalLight::UpdateLightResource()
{
    if (!isDirty_) return;

    void* mapped = nullptr;
    resource_->Map(0, nullptr, &mapped);
    std::memcpy(mapped, &lightData_, sizeof(DirectionalLightData));
    resource_->Unmap(0, nullptr);
}

void DirectionalLight::SerializeTo(void* dest) const
{
    std::memcpy(dest, &lightData_, sizeof(DirectionalLightData));
}
