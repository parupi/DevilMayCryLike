#include "PointLight.h"
#include <imgui.h>

PointLight::PointLight(const std::string& name) : BaseLight(name)
{
	// 各種パラメータをエディターに追加
	global_->AddItem(name, "Color", Vector4{ 1,1,1 });
	global_->AddItem(name, "Position", Vector3{ 0,0,0 });
	global_->AddItem(name, "Intensity", 1.0f);
	global_->AddItem(name, "Radius", 10.0f);
	global_->AddItem(name, "Decay", 1.0f);
	global_->AddItem(name, "Enabled", true);
}

void PointLight::Update()
{
    DrawLightEditor();

    lightData_.enabled = global_->GetValueRef<bool>(name_, "Enabled");
    lightData_.color = global_->GetValueRef<Vector4>(name_, "Color");
    lightData_.position = global_->GetValueRef<Vector3>(name_, "Position");
    lightData_.intensity = global_->GetValueRef<float>(name_, "Intensity");
    lightData_.radius = global_->GetValueRef<float>(name_, "Radius");
    lightData_.decay = global_->GetValueRef<float>(name_, "Decay");
}

void PointLight::DrawLightEditor()
{
    ImGui::Begin(name_.c_str());

    if (ImGui::TreeNode("Point Light Settings")) {

        // ====== Enabled ======
        bool& enabled = global_->GetValueRef<bool>(name_, "Enabled");
        ImGui::Checkbox("Enabled", &enabled);

        // ====== Color ======
        Vector4& color = global_->GetValueRef<Vector4>(name_, "Color");
        ImGui::ColorEdit4("Color", &color.x);

        // ====== Position ======
        Vector3& position = global_->GetValueRef<Vector3>(name_, "Position");
        ImGui::DragFloat3("Position", &position.x, 0.1f);

        // ====== Intensity ======
        float& intensity = global_->GetValueRef<float>(name_, "Intensity");
        ImGui::DragFloat("Intensity", &intensity, 0.01f, 0.0f, 100.0f);

        // ====== Radius ======
        float& radius = global_->GetValueRef<float>(name_, "Radius");
        ImGui::DragFloat("Radius", &radius, 0.1f, 0.0f, 1000.0f);

        // ====== Decay ======
        float& decay = global_->GetValueRef<float>(name_, "Decay");
        ImGui::DragFloat("Decay", &decay, 0.01f, 0.0f, 10.0f);

        ImGui::TreePop();
    }

    // ====== セーブ & ロード ======
    if (ImGui::Button("Save##Light")) {
        global_->SaveFile(name_);
    }

    ImGui::End();
}

void PointLight::CreateLightResource()
{
    dxManager_->CreateBufferResource(sizeof(PointLightData), resource_);
}

void PointLight::UpdateLightResource()
{
    // 更新されていなければ早期リターン
    if (!isDirty_) return;

    auto data = lightData_;
    void* mapped = nullptr;
    resource_->Map(0, nullptr, &mapped);
    memcpy(mapped, &data, sizeof(PointLightData));
    resource_->Unmap(0, nullptr);
}

void PointLight::SerializeTo(void* dest) const
{
    std::memcpy(dest, &lightData_, sizeof(PointLightData));
}
