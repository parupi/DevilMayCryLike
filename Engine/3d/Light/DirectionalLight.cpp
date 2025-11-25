#include "DirectionalLight.h"
#include <imgui.h>

DirectionalLight::DirectionalLight(const std::string& name)
{
    name_ = name;
    // ライトの情報を初期化
    lightData_ = {};
    lightData_.type = static_cast<int>(LightType::Directional);
    lightData_.enabled = 1;
    lightData_.intensity = 1.0f;
    lightData_.color = { 1,1,1,1 };
    lightData_.direction = { 0,-1,0 };

    Initialize();
}

void DirectionalLight::Initialize()
{
    global_ = GlobalVariables::GetInstance();
    // エディター項目登録
    global_->AddItem(name_, "Color", Vector4{ 1, 1, 1, 1 });
    global_->AddItem(name_, "Direction", Vector3{ 0, -1, 0 });
    global_->AddItem(name_, "Intensity", 1.0f);
    global_->AddItem(name_, "Enabled", true);
}

void DirectionalLight::Update()
{
#ifdef _DEBUG
    DrawLightEditor();
#endif // DEBUG

    lightData_.enabled = global_->GetValueRef<bool>(name_, "Enabled");
    lightData_.color = global_->GetValueRef<Vector4>(name_, "Color");
    Vector3 dir = global_->GetValueRef<Vector3>(name_, "Direction");
    lightData_.direction = dir;
    lightData_.intensity = global_->GetValueRef<float>(name_, "Intensity");
}

void DirectionalLight::DrawLightEditor()
{
    ImGui::Begin(name_.c_str());

    if (ImGui::TreeNode("Directional Light Settings")) {

        bool& enabled = global_->GetValueRef<bool>(name_, "Enabled");
        ImGui::Checkbox("Enabled", &enabled);

        Vector4& color = global_->GetValueRef<Vector4>(name_, "Color");
        ImGui::ColorEdit4("Color", &color.x);

        Vector3& direction = global_->GetValueRef<Vector3>(name_, "Direction");
        if (ImGui::DragFloat3("Direction", &direction.x, 0.01f, -1.0f, 1.0f)) {
            // 変更された瞬間に正規化
            if (Length(direction) > 0.0001f) {
                direction = Normalize(direction);
            }
        }

        float& intensity = global_->GetValueRef<float>(name_, "Intensity");
        ImGui::DragFloat("Intensity", &intensity, 0.01f, 0.0f, 100.0f);

        ImGui::TreePop();
    }

    if (ImGui::Button("Save##DirectionalLight")) {
        global_->SaveFile(name_);
    }

    ImGui::End();
}
