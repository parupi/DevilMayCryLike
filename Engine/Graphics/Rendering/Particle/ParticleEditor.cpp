#ifdef _DEBUG

#include "ParticleEditor.h"
#include "ParticleManager.h"
#include "ParticleEmitter.h"
#include "ParticleGroup.h"
#include "Debugger/GlobalVariables.h"
#include <imgui.h>
#include <format>
#include <Windows.h>

// ─────────────────────────────────────────────────────────────────
// Initialize / Finalize
// ─────────────────────────────────────────────────────────────────

void ParticleEditor::Initialize(ParticleManager* manager)
{
    manager_ = manager;
    global_ = &GlobalVariables::GetInstance();

    ed::Config cfg;
    cfg.SettingsFile = "ParticleNodeEditor.json";
    nodeCtx_ = ed::CreateEditor(&cfg);
}

void ParticleEditor::Finalize()
{
    if (nodeCtx_) {
        ed::DestroyEditor(nodeCtx_);
        nodeCtx_ = nullptr;
    }
}

// ─────────────────────────────────────────────────────────────────
// Draw  (3 ウィンドウを開く)
// ─────────────────────────────────────────────────────────────────

void ParticleEditor::Draw()
{
    DrawParticleWindow();
    DrawEmitterWindow();
    DrawNodeGraph();
}

// ─────────────────────────────────────────────────────────────────
// ウィンドウ 1 : Particle Groups
// ─────────────────────────────────────────────────────────────────

void ParticleEditor::DrawParticleWindow()
{
    ImGui::SetNextWindowSize(ImVec2(420, 600), ImGuiCond_FirstUseEver);
    ImGui::Begin("Particle Groups");

    // ── 新規作成 ─────────────────────────────────────────────────
    ImGui::Text("Create New Particle Group");
	ImGui::Separator();

    static char newName[64]    = "";
    static char newTex[128]    = "";
    static int  newShapeIndex  = 0;

    const char* shapeLabels[] = { "Plane (Billboard)", "Ring", "Cylinder" };

    ImGui::InputText("Name",    newName, IM_ARRAYSIZE(newName));
    ImGui::InputText("Texture", newTex,  IM_ARRAYSIZE(newTex));
    ImGui::Combo("Shape", &newShapeIndex, shapeLabels, IM_ARRAYSIZE(shapeLabels));

    if (ImGui::Button("Create") && strlen(newName) > 0) {
        PrimitiveType shape = static_cast<PrimitiveType>(newShapeIndex);
        manager_->CreateParticleGroup(newName, newTex, shape);
        newName[0] = '\0';
        newTex[0]  = '\0';
    }

    // ── グループ一覧 ─────────────────────────────────────────────
    ImGui::Text("Edit");
    ImGui::Separator();

    auto& groups = manager_->GetParticleGroups();
    if (groups.empty()) { ImGui::End(); return; }

    std::vector<const char*> names;
    std::vector<std::string> keys;
    for (auto& [name, _] : groups) {
        names.push_back(name.c_str());
        keys.push_back(name);
    }

    if (selectedParticleIndex_ >= (int)keys.size()) selectedParticleIndex_ = 0;
    ImGui::Combo("Group", &selectedParticleIndex_, names.data(), (int)names.size());

    const std::string& gName = keys[selectedParticleIndex_];

    // Shape 表示（変更不可：形状はGPUリソースに紐づくため作成時のみ）
    const char* shapeNames[] = { "Plane", "Ring", "Cylinder" };
    ImGui::Text("Shape: %s", shapeNames[(int)groups.at(gName).shape]);

    ImGui::Spacing();

    // ── パラメータ ────────────────────────────────────────────────
    if (ImGui::CollapsingHeader("Transform Range")) {
        Vector3& minT = global_->GetValueRef<Vector3>(gName, "minTranslate");
        Vector3& maxT = global_->GetValueRef<Vector3>(gName, "maxTranslate");
        ImGui::DragFloat3("Min Translate", &minT.x, 0.1f);
        ImGui::DragFloat3("Max Translate", &maxT.x, 0.1f);

        Vector3& minR = global_->GetValueRef<Vector3>(gName, "minRotate");
        Vector3& maxR = global_->GetValueRef<Vector3>(gName, "maxRotate");
        ImGui::DragFloat3("Min Rotate", &minR.x, 0.1f);
        ImGui::DragFloat3("Max Rotate", &maxR.x, 0.1f);

        Vector3& minS = global_->GetValueRef<Vector3>(gName, "minScale");
        Vector3& maxS = global_->GetValueRef<Vector3>(gName, "maxScale");
        ImGui::DragFloat3("Min Scale", &minS.x, 0.01f);
        ImGui::DragFloat3("Max Scale", &maxS.x, 0.01f);
    }

    if (ImGui::CollapsingHeader("Velocity")) {
        Vector3& minV = global_->GetValueRef<Vector3>(gName, "minVelocity");
        Vector3& maxV = global_->GetValueRef<Vector3>(gName, "maxVelocity");
        ImGui::DragFloat3("Min Velocity", &minV.x, 0.1f);
        ImGui::DragFloat3("Max Velocity", &maxV.x, 0.1f);
    }

    if (ImGui::CollapsingHeader("Lifetime")) {
        float& minL = global_->GetValueRef<float>(gName, "minLifeTime");
        float& maxL = global_->GetValueRef<float>(gName, "maxLifeTime");
        ImGui::DragFloat("Min LifeTime", &minL, 0.1f, 0.0f, 9999.0f);
        ImGui::DragFloat("Max LifeTime", &maxL, 0.1f, 0.0f, 9999.0f);
    }

    if (ImGui::CollapsingHeader("Color")) {
        Vector3& minC = global_->GetValueRef<Vector3>(gName, "minColor");
        Vector3& maxC = global_->GetValueRef<Vector3>(gName, "maxColor");
        ImGui::ColorEdit3("Min Color", &minC.x);
        ImGui::ColorEdit3("Max Color", &maxC.x);
        bool& billboard = global_->GetValueRef<bool>(gName, "IsBillboard");
        ImGui::Checkbox("Billboard", &billboard);
    }

    if (ImGui::CollapsingHeader("Fade")) {
        int& fadeInt = global_->GetValueRef<int>(gName, "FadeType");
        const char* fadeNames[] = { "None", "Alpha", "ScaleShrink" };
        ImGui::Combo("Fade Type", &fadeInt, fadeNames, IM_ARRAYSIZE(fadeNames));
        if (static_cast<FadeType>(fadeInt) == FadeType::ScaleShrink) {
            float& shrink = global_->GetValueRef<float>(gName, "ShrinkStartRatio");
            ImGui::SliderFloat("Shrink Start", &shrink, 0.0f, 1.0f);
        }
    }

    if (ImGui::CollapsingHeader("Blend Mode")) {
        int& blend = global_->GetValueRef<int>(gName, "BlendMode");
        const char* blendNames[] = { "None", "Normal", "Add", "Subtract", "Multiply", "Screen" };
        ImGui::Combo("Blend", &blend, blendNames, IM_ARRAYSIZE(blendNames));
    }

    ImGui::Spacing();
    if (ImGui::Button("Save##particle")) {
        global_->SaveFile("Particle", gName);
    }

    ImGui::End();
}

// ─────────────────────────────────────────────────────────────────
// ウィンドウ 2 : Emitters
// ─────────────────────────────────────────────────────────────────

void ParticleEditor::DrawEmitterWindow()
{
    ImGui::SetNextWindowSize(ImVec2(360, 500), ImGuiCond_FirstUseEver);
    ImGui::Begin("Emitters");

    // ── 新規作成 ─────────────────────────────────────────────────
    ImGui::Text("Create New Emitter");
    ImGui::Separator();

    static char newName[64] = "";
    ImGui::InputText("Name##em", newName, IM_ARRAYSIZE(newName));
    if (ImGui::Button("Create##em") && strlen(newName) > 0) {
        manager_->CreateEmitter(newName);
        newName[0] = '\0';
    }

    // ── エミッター一覧 ────────────────────────────────────────────
    ImGui::Text("Edit");
    ImGui::Separator();

    auto& emitters = manager_->GetEmitters();
    if (emitters.empty()) { ImGui::End(); return; }

    std::vector<const char*> names;
    std::vector<std::string> keys;
    for (auto& [name, _] : emitters) {
        names.push_back(name.c_str());
        keys.push_back(name);
    }

    if (selectedEmitterIndex_ >= (int)keys.size()) selectedEmitterIndex_ = 0;
    ImGui::Combo("Emitter", &selectedEmitterIndex_, names.data(), (int)names.size());

    const std::string& eName     = keys[selectedEmitterIndex_];
    ParticleEmitter*   emitterPtr = emitters.at(eName).get();

    // ── 基本設定 ─────────────────────────────────────────────────
    if (ImGui::CollapsingHeader("Settings", ImGuiTreeNodeFlags_DefaultOpen)) {
        Vector3& pos  = global_->GetValueRef<Vector3>(eName, "EmitPosition");
        float&   freq = global_->GetValueRef<float>(eName, "Frequency");
        int&     cnt  = global_->GetValueRef<int>(eName, "Count");
        bool&    act  = global_->GetValueRef<bool>(eName, "IsActive");

        ImGui::DragFloat3("Position",  &pos.x, 0.1f);
        ImGui::DragFloat("Frequency", &freq, 0.01f, 0.01f, 1000.0f);
        ImGui::DragInt("Count",       &cnt,  1, 0, 1000);
        ImGui::Checkbox("Active",     &act);
    }

    // ── 接続済みパーティクル (count/spawnRate の調整) ──────────────
    if (ImGui::CollapsingHeader("Connected Particles", ImGuiTreeNodeFlags_DefaultOpen)) {
        auto& particles = emitterPtr->GetParticles();

        if (particles.empty()) {
            ImGui::TextDisabled("No particles connected.");
            ImGui::TextDisabled("Connect via the Node Graph window.");
        }

        for (int i = 0; i < (int)particles.size(); ++i) {
            ImGui::PushID(i);
            ImGui::Separator();
            ImGui::Text("%s", particles[i].name.c_str());
            ImGui::DragInt("Count##p",    &particles[i].count,     1, 0, 1000);
            ImGui::DragFloat("Rate##p",   &particles[i].spawnRate, 0.01f, 0.0f, 10.0f);
            if (ImGui::SmallButton("Remove")) {
                emitterPtr->RemoveParticle(i);
                ImGui::PopID();
                break;
            }
            ImGui::PopID();
        }
    }

    ImGui::Spacing();

    if (ImGui::Button("Emit Now")) {
        global_->SetValue(eName, "EmitAll", true);
    }
    ImGui::SameLine();
    if (ImGui::Button("Save##em")) {
        emitterPtr->Save("Resource/Emitter/" + eName + ".json");
    }
    ImGui::SameLine();
    if (ImGui::Button("Load##em")) {
        emitterPtr->Load("Resource/Emitter/" + eName + ".json");
    }

    ImGui::End();
}

// ─────────────────────────────────────────────────────────────────
// ウィンドウ 3 : Node Graph
// ─────────────────────────────────────────────────────────────────

void ParticleEditor::DrawNodeGraph()
{
    auto& emitters = manager_->GetEmitters();
    auto& groups   = manager_->GetParticleGroups();

    // ID をあらかじめ確保
    for (auto& [name, _] : emitters) GetOrCreateEmitterNodeId(name);
    for (auto& [name, _] : groups)   GetOrCreateParticleNodeId(name);

    ImGui::SetNextWindowSize(ImVec2(900, 600), ImGuiCond_FirstUseEver);
    if (!ImGui::Begin("Particle Node Graph")) {
        // ウィンドウが折りたたまれているときは ed::Begin/End を呼ばずに終了
        ImGui::End();
        return;
    }
    ImGui::TextDisabled("Drag from Emitter [OUT] pin to Particle [IN] pin to connect. Right-click link to delete.");
    ImGui::Spacing();

    ed::SetCurrentEditor(nodeCtx_);
    ed::Begin("particle_graph", ImVec2(0, 0));

    // ── Emitter ノード ────────────────────────────────────────────
    float emitterY = 20.0f;
    for (auto& [emName, emitter] : emitters) {
        uintptr_t nodeId = emitterNodeIds_.at(emName);
        uintptr_t outPin = OutputPinId(nodeId);

        if (firstFrame_) {
            ed::SetNodePosition(ed::NodeId(nodeId), ImVec2(50.0f, emitterY));
            emitterY += 140.0f;
        }

        ed::PushStyleColor(ed::StyleColor_NodeBg,     ImColor(180, 100, 30, 220));
        ed::PushStyleColor(ed::StyleColor_NodeBorder,  ImColor(220, 140, 60, 255));
        ed::BeginNode(ed::NodeId(nodeId));

        ImGui::Text("[ Emitter ]");
        ImGui::Text("%s", emName.c_str());

        auto& particles = emitter.get()->GetParticles();
        ImGui::TextDisabled("%d particle(s)", (int)particles.size());

        ImGui::SameLine();

        // Output pin
        ed::BeginPin(ed::PinId(outPin), ed::PinKind::Output);
        ImGui::Text("OUT >");
        ed::EndPin();

        ed::EndNode();
        ed::PopStyleColor(2);
    }

    // ── Particle Group ノード ─────────────────────────────────────
    float particleY = 20.0f;
    const char* shapeLabels[] = { "Plane", "Ring", "Cylinder" };
    for (auto& [pgName, group] : groups) {
        uintptr_t nodeId = particleNodeIds_.at(pgName);
        uintptr_t inPin  = InputPinId(nodeId);

        if (firstFrame_) {
            ed::SetNodePosition(ed::NodeId(nodeId), ImVec2(500.0f, particleY));
            particleY += 120.0f;
        }

        ed::PushStyleColor(ed::StyleColor_NodeBg,    ImColor(30, 80, 180, 220));
        ed::PushStyleColor(ed::StyleColor_NodeBorder, ImColor(60, 140, 220, 255));
        ed::BeginNode(ed::NodeId(nodeId));

        // Input pin
        ed::BeginPin(ed::PinId(inPin), ed::PinKind::Input);
        ImGui::Text("< IN");
        ed::EndPin();

        ImGui::SameLine();
        ImGui::Text("[ Particle Group ]");
        ImGui::Text("%s", pgName.c_str());
        ImGui::TextDisabled("Shape: %s", shapeLabels[(int)group.shape]);

        ed::EndNode();
        ed::PopStyleColor(2);
    }

    // ── リンク描画 ────────────────────────────────────────────────
    for (auto& [emName, emitter] : emitters) {
        if (!emitterNodeIds_.count(emName)) continue;
        uintptr_t outPin = OutputPinId(emitterNodeIds_.at(emName));

        for (auto& ep : emitter.get()->GetParticles()) {
            if (!particleNodeIds_.count(ep.name)) continue;
            uintptr_t inPin  = InputPinId(particleNodeIds_.at(ep.name));
            uintptr_t linkId = GetOrCreateLinkId(emName, ep.name);

            ed::Link(ed::LinkId(linkId), ed::PinId(outPin), ed::PinId(inPin),
                     ImColor(255, 200, 50, 255), 2.0f);
        }
    }

    // ── 新規リンク作成 ────────────────────────────────────────────
    // Begin() 内部で m_InActive=true が無条件に設定されるため
    // 戻り値に関わらず EndCreate() を必ず呼ぶ必要がある
    ed::BeginCreate(ImColor(255, 255, 255, 255), 2.0f);
    {
        ed::PinId startPin, endPin;
        if (ed::QueryNewLink(&startPin, &endPin)) {
            uintptr_t startId = startPin.Get();
            uintptr_t endId   = endPin.Get();

            // ピンの向きを正規化（input→output でドラッグされた場合に対応）
            if (IsInputPin(startId) && IsOutputPin(endId)) std::swap(startId, endId);

            if (IsOutputPin(startId) && IsInputPin(endId)) {
                std::string emName = EmitterNameFromOutputPin(startId);
                std::string pgName = ParticleNameFromInputPin(endId);

                // 既に接続済みかチェック
                bool alreadyConnected = false;
                if (emitters.count(emName)) {
                    for (auto& ep : emitters.at(emName).get()->GetParticles()) {
                        if (ep.name == pgName) { alreadyConnected = true; break; }
                    }
                }

                if (!emName.empty() && !pgName.empty() && !alreadyConnected) {
                    if (ed::AcceptNewItem(ImColor(0, 255, 128, 255), 2.0f)) {
                        emitters.at(emName).get()->AddParticle(pgName);
                    }
                } else {
                    ed::RejectNewItem(ImColor(255, 80, 80, 255), 2.0f);
                }
            } else {
                ed::RejectNewItem(ImColor(255, 80, 80, 255), 2.0f);
            }
        }
    }
    ed::EndCreate(); // 常に呼ぶ

    // ── リンク削除 ────────────────────────────────────────────────
    // BeginDelete() も同様に m_InActive を無条件設定するため常に EndDelete() を呼ぶ
    ed::BeginDelete();
    {
        ed::LinkId deletedId;
        while (ed::QueryDeletedLink(&deletedId)) {
            if (ed::AcceptDeletedItem(true)) {
                uintptr_t rawId = deletedId.Get();
                for (auto it = linkIds_.begin(); it != linkIds_.end(); ++it) {
                    if (it->second != rawId) continue;

                    const std::string& emName = it->first.first;
                    const std::string& pgName = it->first.second;

                    if (emitters.count(emName)) {
                        auto& ep = emitters.at(emName).get()->GetParticles();
                        for (size_t i = 0; i < ep.size(); ++i) {
                            if (ep[i].name == pgName) {
                                emitters.at(emName).get()->RemoveParticle(i);
                                break;
                            }
                        }
                    }
                    linkIds_.erase(it);
                    break;
                }
            }
        }
    }
    ed::EndDelete(); // 常に呼ぶ

    ed::End();
    ed::SetCurrentEditor(nullptr);

    firstFrame_ = false;

    ImGui::End();
}

// ─────────────────────────────────────────────────────────────────
// ID ヘルパー
// ─────────────────────────────────────────────────────────────────

uintptr_t ParticleEditor::GetOrCreateEmitterNodeId(const std::string& name)
{
    auto it = emitterNodeIds_.find(name);
    if (it != emitterNodeIds_.end()) return it->second;

    uintptr_t id = nextNodeId_++;
    emitterNodeIds_[name]       = id;
    nodeIdToEmitterName_[id]    = name;
    return id;
}

uintptr_t ParticleEditor::GetOrCreateParticleNodeId(const std::string& name)
{
    auto it = particleNodeIds_.find(name);
    if (it != particleNodeIds_.end()) return it->second;

    uintptr_t id = nextNodeId_++;
    particleNodeIds_[name]       = id;
    nodeIdToParticleName_[id]    = name;
    return id;
}

uintptr_t ParticleEditor::GetOrCreateLinkId(const std::string& emitterName, const std::string& particleName)
{
    auto key = std::make_pair(emitterName, particleName);
    auto it  = linkIds_.find(key);
    if (it != linkIds_.end()) return it->second;

    uintptr_t id = nextLinkId_++;
    linkIds_[key] = id;
    return id;
}

std::string ParticleEditor::EmitterNameFromOutputPin(uintptr_t pinId) const
{
    uintptr_t nodeId = pinId - 100000;
    auto it = nodeIdToEmitterName_.find(nodeId);
    return (it != nodeIdToEmitterName_.end()) ? it->second : "";
}

std::string ParticleEditor::ParticleNameFromInputPin(uintptr_t pinId) const
{
    uintptr_t nodeId = pinId - 200000;
    auto it = nodeIdToParticleName_.find(nodeId);
    return (it != nodeIdToParticleName_.end()) ? it->second : "";
}

#endif
