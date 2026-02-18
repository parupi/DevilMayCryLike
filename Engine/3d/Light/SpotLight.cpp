#include "SpotLight.h"
#ifdef _DEBUG
#include <imgui.h>
#endif // DEBUG

SpotLight::SpotLight(const std::string& name)
{
    name_ = name;
    // ライトの情報を初期化
    lightData_ = {};
    lightData_.type = static_cast<int>(LightType::Spot);
    lightData_.enabled = 1;
    lightData_.intensity = 1.0f;
    lightData_.decay = 1.0f;
    lightData_.color = { 1,1,1,1 };
    lightData_.position = { 0.0f, 0.0f, 0.0f };
    lightData_.direction = { 0.0f, 1.0f, 0.0f };
    lightData_.cosAngle = 0.9f;
    lightData_.distance = 20.0f;

    Initialize();
}

void SpotLight::Initialize()
{
    global_ = GlobalVariables::GetInstance();
    // エディター項目登録
    global_->AddItem(name_, "Color", Vector4{ 1, 1, 1, 1 });
    global_->AddItem(name_, "Position", Vector3{ 0, 0, 0 });
    global_->AddItem(name_, "Direction", Vector3{ 0, -1, 0 });
    global_->AddItem(name_, "Intensity", 1.0f);
    global_->AddItem(name_, "Distance", 20.0f);
    global_->AddItem(name_, "Decay", 1.0f);
    global_->AddItem(name_, "CosAngle", 0.9f); // 角度の余弦値（0.9 ≒ 約25°）
    global_->AddItem(name_, "Enabled", true);
}

void SpotLight::Update()
{
#ifdef _DEBUG
    DrawLightEditor();
#endif // DEBUG

    lightData_.enabled = global_->GetValueRef<bool>(name_, "Enabled");
    lightData_.color = global_->GetValueRef<Vector4>(name_, "Color");
    lightData_.position = global_->GetValueRef<Vector3>(name_, "Position");
    Vector3 dir = global_->GetValueRef<Vector3>(name_, "Direction");
    lightData_.direction = dir;
    lightData_.intensity = global_->GetValueRef<float>(name_, "Intensity");
    lightData_.distance = global_->GetValueRef<float>(name_, "Distance");
    lightData_.decay = global_->GetValueRef<float>(name_, "Decay");
    lightData_.cosAngle = global_->GetValueRef<float>(name_, "CosAngle");
}

void SpotLight::DrawLightEditor()
{
    ImGui::Begin(name_.c_str());

    if (ImGui::TreeNode("Spot Light Settings")) {

        bool& enabled = global_->GetValueRef<bool>(name_, "Enabled");
        ImGui::Checkbox("Enabled", &enabled);

        Vector4& color = global_->GetValueRef<Vector4>(name_, "Color");
        ImGui::ColorEdit4("Color", &color.x);

        Vector3& position = global_->GetValueRef<Vector3>(name_, "Position");
        ImGui::DragFloat3("Position", &position.x, 0.1f);

        Vector3& direction = global_->GetValueRef<Vector3>(name_, "Direction");
        if (ImGui::DragFloat3("Direction", &direction.x, 0.01f, -1.0f, 1.0f)) {
            // 変更された瞬間に正規化
            if (Length(direction) > 0.0001f) {
                direction = Normalize(direction);
            }
        }

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
        global_->SaveFile("Light", name_);
    }

    ImGui::End();
}