#ifdef _DEBUG

#include "ParticleEditor.h"
#include "Graphics/Rendering/Particle/ParticleManager.h"
#include "Graphics/Rendering/Particle/ParticleEmitter.h"
#include "Graphics/Rendering/Particle/ParticleGroup.h"
#include "Debugger/GlobalVariables.h"
#include "Editor/Core/EditorWindowRegistry.h"
#include <imgui.h>
#include <format>
#include <algorithm>
#include <utility>
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
    DrawVFXWindow();
    DrawNodeGraph();
}

// ─────────────────────────────────────────────────────────────────
// ウィンドウ 1 : Particle Groups
// ─────────────────────────────────────────────────────────────────

void ParticleEditor::DrawParticleWindow()
{
    ImGui::SetNextWindowSize(ImVec2(420, 600), ImGuiCond_FirstUseEver);
    if (!EditorWindow::Begin("Particle Groups", EditorWindow::Category::kVFX)) return;

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
    if (groups.empty()) { EditorWindow::End(); return; }

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
    }

    if (ImGui::CollapsingHeader("Billboard")) {
        bool& billboard = global_->GetValueRef<bool>(gName, "IsBillboard");
        ImGui::Checkbox("Billboard", &billboard);
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("オフにすると Orient To Direction / Rotate で向きが決まります");
        }

        if (billboard) {
            int& type = global_->GetValueRef<int>(gName, "BillboardType");
            const char* typeNames[] = { "Screen", "AxisY", "Velocity" };
            ImGui::Combo("Type", &type, typeNames, IM_ARRAYSIZE(typeNames));
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("Screen   : 常にカメラを向く（従来）\n"
                                  "AxisY    : Y軸まわりだけ回る。炎・光の柱向き\n"
                                  "Velocity : 進行方向に軸を合わせる。火花の尾を引く表現");
            }

            if (static_cast<BillboardType>(type) == BillboardType::Velocity) {
                float& stretch = global_->GetValueRef<float>(gName, "VelocityStretch");
                ImGui::DragFloat("Velocity Stretch", &stretch, 0.005f, 0.0f, 2.0f);
                if (ImGui::IsItemHovered()) {
                    ImGui::SetTooltip("速度1m/sあたり何倍に引き伸ばすか。0=伸ばさない。\n"
                                      "速度10m/sで0.05なら1.5倍の長さになります");
                }
                ImGui::TextDisabled("止まっている粒はScreenで描かれます");
            }
        }
    }

    if (ImGui::CollapsingHeader("Texture Animation")) {
        int& columns = global_->GetValueRef<int>(gName, "AnimColumns");
        int& rows = global_->GetValueRef<int>(gName, "AnimRows");
        ImGui::DragInt("Columns", &columns, 0.1f, 1, 32);
        ImGui::DragInt("Rows", &rows, 0.1f, 1, 32);

        const int frameCount = (columns > 0 ? columns : 1) * (rows > 0 ? rows : 1);
        if (frameCount <= 1) {
            ImGui::TextDisabled("1x1 = コマ分割なし。テクスチャ全体を使います");
        } else {
            ImGui::Text("%d コマ", frameCount);

            float& fps = global_->GetValueRef<float>(gName, "AnimFps");
            ImGui::DragFloat("Fps", &fps, 0.5f, 0.0f, 240.0f);
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("0 にすると寿命いっぱいでシートを1周します（爆発向き）");
            }
            if (fps <= 0.0f) {
                ImGui::TextDisabled("寿命いっぱいで1周します");
            }

            bool& loop = global_->GetValueRef<bool>(gName, "AnimLoop");
            ImGui::Checkbox("Loop", &loop);
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("オフにすると最後のコマで止まります");
            }
        }
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

    if (ImGui::CollapsingHeader("Physics")) {
        Vector3& gravity = global_->GetValueRef<Vector3>(gName, "Gravity");
        ImGui::DragFloat3("Gravity", &gravity.x, 0.1f);
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("毎秒加算される加速度[m/s^2]。火花なら Y=-9.8、煙なら Y=+1 など");
        }

        float& drag = global_->GetValueRef<float>(gName, "Drag");
        ImGui::SliderFloat("Drag", &drag, 0.0f, 1.0f);
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("1秒後に残る速度の割合。1.0=減衰なし / 0.1=急ブレーキ");
        }
    }

    // Curve / Gradient（GlobalVariablesでは保存できないので別ファイル管理）
    DrawCurveSection(gName);

    if (ImGui::CollapsingHeader("Direction")) {
        bool& useDir = global_->GetValueRef<bool>(gName, "UseDirectional");
        ImGui::Checkbox("Use Direction", &useDir);
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Emit に方向が渡されたとき、その方向へ飛ばす。\n"
                              "有効時は Velocity と Radial の設定は使われません。");
        }

        if (useDir) {
            float& speedMin = global_->GetValueRef<float>(gName, "SpeedMin");
            float& speedMax = global_->GetValueRef<float>(gName, "SpeedMax");
            ImGui::DragFloat("Speed Min", &speedMin, 0.1f, 0.0f, 1000.0f);
            ImGui::DragFloat("Speed Max", &speedMax, 0.1f, 0.0f, 1000.0f);

            float& spread = global_->GetValueRef<float>(gName, "SpreadDegrees");
            ImGui::SliderFloat("Spread (deg)", &spread, 0.0f, 180.0f);
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("方向を軸としたコーンの半角。0=まっすぐ / 90=半球 / 180=全方向");
            }

            bool& orient = global_->GetValueRef<bool>(gName, "OrientToDirection");
            ImGui::Checkbox("Orient To Direction", &orient);
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("メッシュ自体を方向へ向ける（インパクトリング等）。\n"
                                  "Plane は +Z 面、Ring / Cylinder は +Y 面が方向を向きます。");
            }
            if (orient && global_->GetValueRef<bool>(gName, "IsBillboard")) {
                ImGui::TextColored(ImVec4(1.0f, 0.7f, 0.2f, 1.0f),
                    "Billboard が優先されるため向き付けは効きません");
            }
        }
    }

    if (ImGui::CollapsingHeader("Radial")) {
        int& radialMode = global_->GetValueRef<int>(gName, "RadialMode");
        const char* radialNames[] = { "None", "Converge", "Diverge" };
        ImGui::Combo("Radial Mode", &radialMode, radialNames, IM_ARRAYSIZE(radialNames));
        if (radialMode != 0) {
            float& radialSpeed = global_->GetValueRef<float>(gName, "RadialSpeed");
            ImGui::DragFloat("Radial Speed", &radialSpeed, 0.1f, 0.0f, 100.0f);
        }
    }

    if (ImGui::CollapsingHeader("Blend Mode")) {
        int& blend = global_->GetValueRef<int>(gName, "BlendMode");
        const char* blendNames[] = { "None", "Normal", "Add", "Subtract", "Multiply", "Screen" };
        ImGui::Combo("Blend", &blend, blendNames, IM_ARRAYSIZE(blendNames));
    }

    ImGui::Spacing();
    // .vfx.json が定義しているグループは、そちらへ書き戻さないと次回起動で元に戻ってしまう
    if (const std::string* vfxName = manager_->GetOwningVFX(gName)) {
        if (ImGui::Button(std::format("Save VFX ({})##particle", *vfxName).c_str())) {
            vfxStatus_ = manager_->SaveVFX(*vfxName)
                ? ("saved: " + VFXFile::MakeFilePath(*vfxName))
                : ("save failed: " + *vfxName);
        }
        ImGui::TextDisabled("パラメータもカーブも %s.vfx.json にまとめて保存されます", vfxName->c_str());
    } else if (ImGui::Button("Save##particle")) {
        global_->SaveFile("Particle", gName);
    }

    EditorWindow::End();
}

// ─────────────────────────────────────────────────────────────────
// ウィンドウ 2 : Emitters
// ─────────────────────────────────────────────────────────────────

void ParticleEditor::DrawEmitterWindow()
{
    ImGui::SetNextWindowSize(ImVec2(360, 500), ImGuiCond_FirstUseEver);
    if (!EditorWindow::Begin("Emitters", EditorWindow::Category::kVFX)) return;

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
    if (emitters.empty()) { EditorWindow::End(); return; }

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

    // ── ワンショット再生（ゲーム側の PlayVFX と同じ経路を試す）──────
    if (ImGui::CollapsingHeader("One Shot", ImGuiTreeNodeFlags_DefaultOpen)) {
        static Vector3 shotDir{ 0.0f, 1.0f, 0.0f };
        static float shotScale = 1.0f;

        ImGui::DragFloat3("Direction##shot", &shotDir.x, 0.05f, -1.0f, 1.0f);
        ImGui::DragFloat("Count Scale##shot", &shotScale, 0.05f, 0.0f, 10.0f);

        const Vector3& emitPos = global_->GetValueRef<Vector3>(eName, "EmitPosition");

        if (ImGui::Button("Play (with direction)")) {
            manager_->PlayVFX(eName, emitPos, shotDir, shotScale);
        }
        ImGui::SameLine();
        if (ImGui::Button("Play")) {
            manager_->PlayVFX(eName, emitPos, shotScale);
        }
        ImGui::TextDisabled("Position 欄の位置に1回だけ発生させます");
    }

    // ── メッシュ形状エミット（モデル表面から発生させる）──────────
    if (ImGui::CollapsingHeader("Shape (Mesh Emit)", ImGuiTreeNodeFlags_DefaultOpen)) {
        static char shapeBuf[128] = "";
        static std::string lastShapeEmitter;

        // 選択エミッターが変わったら入力バッファを現在値で更新する
        if (lastShapeEmitter != eName) {
            lastShapeEmitter = eName;
            strncpy_s(shapeBuf, emitterPtr->GetShapeModelName().c_str(), sizeof(shapeBuf) - 1);
        }

        if (ImGui::InputText("Shape Model", shapeBuf, IM_ARRAYSIZE(shapeBuf))) {
            emitterPtr->SetShapeModel(shapeBuf);
        }
        ImGui::TextDisabled("Loaded model name. Empty = point emit.");

        if (!emitterPtr->GetShapeModelName().empty()) {
            if (ImGui::SmallButton("Clear Shape")) {
                emitterPtr->ClearShapeModel();
                shapeBuf[0] = '\0';
            }
        }
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

    EditorWindow::End();
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
    if (!EditorWindow::Begin("Particle Node Graph", EditorWindow::Category::kVFX)) {
        // 非表示・折りたたみのときは ed::Begin/End を呼ばずに終了（End() も不要）
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

    EditorWindow::End();
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

// ─────────────────────────────────────────────────────────────────
// Curve / Gradient 編集
// ─────────────────────────────────────────────────────────────────

void ParticleEditor::DrawCurveSection(const std::string& groupName)
{
    ParticleCurves* curves = manager_->GetParticleCurves(groupName);
    if (!curves) return;

    if (!ImGui::CollapsingHeader("Curves")) return;

    ImGui::TextWrapped("カーブを1つでも設定すると、そのチャンネルは FadeType より優先されます。"
                       "すべて空なら従来どおり FadeType で動きます。");
    ImGui::Separator();

    // ── Size ──
    ImGui::Text("Size (x baseScale)");
    DrawCurveEditor("##sizeCurve", curves->sizeCurve, 0.0f, 2.0f);
    if (ImGui::SmallButton("消える##size")) curves->sizeCurve = Curve::Linear(1.0f, 0.0f);
    ImGui::SameLine();
    if (ImGui::SmallButton("広がる##size")) curves->sizeCurve = Curve::Linear(0.1f, 1.0f);
    ImGui::SameLine();
    if (ImGui::SmallButton("クリア##size")) curves->sizeCurve.Clear();

    ImGui::Spacing();

    // ── Alpha ──
    ImGui::Text("Alpha (x baseAlpha)");
    DrawCurveEditor("##alphaCurve", curves->alphaCurve, 0.0f, 1.0f);
    if (ImGui::SmallButton("フェードアウト##alpha")) curves->alphaCurve = Curve::Linear(1.0f, 0.0f);
    ImGui::SameLine();
    if (ImGui::SmallButton("末尾だけ消す##alpha")) {
        curves->alphaCurve.SetKeys({ {0.0f, 1.0f}, {0.7f, 1.0f}, {1.0f, 0.0f} });
    }
    ImGui::SameLine();
    if (ImGui::SmallButton("クリア##alpha")) curves->alphaCurve.Clear();

    ImGui::Spacing();

    // ── Color ──
    ImGui::Text("Color Gradient (x baseColor)");
    DrawGradientEditor("##colorGradient", curves->colorGradient);
    if (ImGui::SmallButton("火花(白→黄→橙)##grad")) {
        curves->colorGradient.SetKeys({
            { 0.0f, Vector4{ 1.0f, 1.0f, 1.0f, 1.0f } },
            { 0.4f, Vector4{ 1.0f, 0.9f, 0.35f, 1.0f } },
            { 1.0f, Vector4{ 1.0f, 0.4f, 0.05f, 0.0f } },
            });
    }
    ImGui::SameLine();
    if (ImGui::SmallButton("クリア##grad")) curves->colorGradient.Clear();

    ImGui::Spacing();
    // .vfx.json 由来のグループはカーブもそのファイルに入っている。
    // ここで .curve.json へ書き出すと、次回起動時に .vfx.json 側で上書きされて混乱する
    if (const std::string* vfxName = manager_->GetOwningVFX(groupName)) {
        ImGui::TextDisabled("カーブは %s.vfx.json に含まれます（下の Save VFX で保存）", vfxName->c_str());
    } else {
        if (ImGui::Button("Save Curves")) {
            manager_->SaveParticleCurves(groupName);
        }
        ImGui::SameLine();
        ImGui::TextDisabled("-> %s", ParticleCurves::MakeFilePath(groupName).c_str());
    }
    ImGui::Separator();
}

// ─────────────────────────────────────────────────────────────────
// ウィンドウ 3 : VFX（.vfx.json の一元管理）
// ─────────────────────────────────────────────────────────────────

void ParticleEditor::DrawVFXWindow()
{
    ImGui::SetNextWindowSize(ImVec2(380, 320), ImGuiCond_FirstUseEver);
    if (!EditorWindow::Begin("VFX", EditorWindow::Category::kVFX)) return;

    ImGui::TextWrapped("1エフェクト = Resource/VFX/<名前>.vfx.json 1ファイル。"
                       "テクスチャ・形状・パラメータ・カーブ・エミッター構成がすべて入っています。");
    ImGui::Separator();

    // ── ファイル一覧から読み込む ──────────────────────────────────
    const std::vector<std::string> vfxNames = manager_->ListVFXNames();

    if (vfxNames.empty()) {
        ImGui::TextDisabled("Resource/VFX/ に .vfx.json がありません");
    } else {
        std::vector<const char*> displayNames;
        displayNames.reserve(vfxNames.size());
        for (const std::string& name : vfxNames) displayNames.push_back(name.c_str());

        if (selectedVFXIndex_ >= static_cast<int>(vfxNames.size())) selectedVFXIndex_ = 0;
        ImGui::Combo("File", &selectedVFXIndex_, displayNames.data(), static_cast<int>(displayNames.size()));

        const std::string& selected = vfxNames[selectedVFXIndex_];

        if (ImGui::Button("Load")) {
            // 読み直すとファイルの値でパラメータとカーブが上書きされる（編集の破棄にもなる）
            vfxStatus_ = manager_->LoadVFX(selected)
                ? ("loaded: " + selected)
                : ("load failed: " + VFXFile::MakeFilePath(selected));
        }
        ImGui::SameLine();
        if (ImGui::Button("Save")) {
            vfxStatus_ = manager_->SaveVFX(selected)
                ? ("saved: " + VFXFile::MakeFilePath(selected))
                : ("save failed（未読み込みのVFXは保存できません）: " + selected);
        }
        ImGui::SameLine();
        if (ImGui::Button("Play")) {
            // 位置はエミッターの EmitPosition。方向は上向き
            const Vector3 position = global_->GetValueRef<Vector3>(selected, "EmitPosition");
            vfxStatus_ = manager_->PlayVFX(selected, position, Vector3{ 0.0f, 1.0f, 0.0f }, 1.0f)
                ? ("played: " + selected)
                : ("play failed（先にLoadしてください）: " + selected);
        }
    }

    ImGui::Separator();

    // ── 既存エミッターからの移行 ──────────────────────────────────
    ImGui::Text("既存エミッターを .vfx.json へ書き出す");
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Resource/Emitter/*.json + GlobalVariables + .curve.json に\n"
                          "散らばっている定義を1ファイルにまとめます。");
    }

    auto& emitters = manager_->GetEmitters();
    std::vector<const char*> emitterNames;
    std::vector<std::string> emitterKeys;
    for (auto& [name, _] : emitters) {
        emitterNames.push_back(name.c_str());
        emitterKeys.push_back(name);
    }

    if (emitterKeys.empty()) {
        ImGui::TextDisabled("エミッターがありません");
    } else {
        static int exportIndex = 0;
        static char exportName[64] = "";
        if (exportIndex >= static_cast<int>(emitterKeys.size())) exportIndex = 0;
        ImGui::Combo("Emitter##vfx", &exportIndex, emitterNames.data(), static_cast<int>(emitterNames.size()));
        ImGui::InputText("VFX Name##vfx", exportName, IM_ARRAYSIZE(exportName));

        if (ImGui::Button("Export") && strlen(exportName) > 0) {
            vfxStatus_ = manager_->ExportEmitterAsVFX(emitterKeys[exportIndex], exportName)
                ? ("exported: " + VFXFile::MakeFilePath(exportName))
                : ("export failed: " + emitterKeys[exportIndex]);
        }
        if (strlen(exportName) == 0) {
            ImGui::SameLine();
            ImGui::TextDisabled("VFX Name を入れてください");
        }
    }

    if (!vfxStatus_.empty()) {
        ImGui::Separator();
        ImGui::TextWrapped("%s", vfxStatus_.c_str());
    }

    EditorWindow::End();
}

bool ParticleEditor::DrawCurveEditor(const char* label, Curve& curve, float valueMin, float valueMax)
{
    auto& keys = curve.GetKeysForEdit();

    const ImVec2 canvasPos = ImGui::GetCursorScreenPos();
    ImVec2 canvasSize(ImGui::GetContentRegionAvail().x, 110.0f);
    if (canvasSize.x < 80.0f) canvasSize.x = 80.0f;
    const ImVec2 canvasEnd(canvasPos.x + canvasSize.x, canvasPos.y + canvasSize.y);

    ImGui::InvisibleButton(label, canvasSize);
    const bool hovered = ImGui::IsItemHovered();

    ImDrawList* dl = ImGui::GetWindowDrawList();
    dl->AddRectFilled(canvasPos, canvasEnd, IM_COL32(28, 28, 32, 255));
    dl->AddRect(canvasPos, canvasEnd, IM_COL32(90, 90, 100, 255));

    // 目盛り（4分割）
    for (int i = 1; i < 4; ++i) {
        const float fx = canvasPos.x + canvasSize.x * (i / 4.0f);
        const float fy = canvasPos.y + canvasSize.y * (i / 4.0f);
        dl->AddLine(ImVec2(fx, canvasPos.y), ImVec2(fx, canvasEnd.y), IM_COL32(55, 55, 60, 255));
        dl->AddLine(ImVec2(canvasPos.x, fy), ImVec2(canvasEnd.x, fy), IM_COL32(55, 55, 60, 255));
    }

    const float valueSpan = (valueMax - valueMin) > 0.0f ? (valueMax - valueMin) : 1.0f;

    // 正規化座標 <-> 画面座標
    auto toScreen = [&](float t, float v) {
        const float nx = std::clamp(t, 0.0f, 1.0f);
        const float ny = std::clamp((v - valueMin) / valueSpan, 0.0f, 1.0f);
        return ImVec2(canvasPos.x + canvasSize.x * nx, canvasEnd.y - canvasSize.y * ny);
        };
    auto toValue = [&](const ImVec2& screen) {
        const float t = std::clamp((screen.x - canvasPos.x) / canvasSize.x, 0.0f, 1.0f);
        const float v = valueMin + valueSpan * std::clamp((canvasEnd.y - screen.y) / canvasSize.y, 0.0f, 1.0f);
        return std::pair<float, float>{ t, v };
        };

    if (keys.empty()) {
        // 未設定であることが分かるように、実際に使われる値(1.0)の位置へ点線を引く
        const ImVec2 a = toScreen(0.0f, 1.0f);
        const ImVec2 b = toScreen(1.0f, 1.0f);
        dl->AddLine(a, b, IM_COL32(110, 110, 120, 160), 1.0f);
        dl->AddText(ImVec2(canvasPos.x + 6.0f, canvasPos.y + 4.0f), IM_COL32(150, 150, 160, 255),
            "empty (right-click to add key)");
    } else {
        // 折れ線
        for (size_t i = 0; i + 1 < keys.size(); ++i) {
            dl->AddLine(toScreen(keys[i].time, keys[i].value),
                toScreen(keys[i + 1].time, keys[i + 1].value),
                IM_COL32(120, 200, 255, 255), 2.0f);
        }
        // 端の外側は一定値になるので、その区間も描いておく
        dl->AddLine(toScreen(0.0f, keys.front().value), toScreen(keys.front().time, keys.front().value),
            IM_COL32(120, 200, 255, 120), 2.0f);
        dl->AddLine(toScreen(keys.back().time, keys.back().value), toScreen(1.0f, keys.back().value),
            IM_COL32(120, 200, 255, 120), 2.0f);
    }

    bool edited = false;
    const ImVec2 mouse = ImGui::GetIO().MousePos;

    // 最も近いキーを探す（掴む・消す対象の判定用）
    int nearestIndex = -1;
    float nearestDistSq = 12.0f * 12.0f;
    for (size_t i = 0; i < keys.size(); ++i) {
        const ImVec2 p = toScreen(keys[i].time, keys[i].value);
        const float dx = p.x - mouse.x;
        const float dy = p.y - mouse.y;
        const float distSq = dx * dx + dy * dy;
        if (distSq < nearestDistSq) {
            nearestDistSq = distSq;
            nearestIndex = static_cast<int>(i);
        }
    }

    // キーの描画（掴んでいるもの・近いものを強調）
    for (size_t i = 0; i < keys.size(); ++i) {
        const ImVec2 p = toScreen(keys[i].time, keys[i].value);
        const bool highlight = (static_cast<int>(i) == nearestIndex);
        dl->AddCircleFilled(p, highlight ? 6.0f : 4.0f,
            highlight ? IM_COL32(255, 220, 120, 255) : IM_COL32(255, 255, 255, 255));
    }

    if (hovered) {
        // 左ドラッグ: キーを掴んで移動
        if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) && nearestIndex >= 0) {
            draggingCurve_ = &curve;
            draggingKeyIndex_ = nearestIndex;
        }
        // 右クリック: キーを追加
        if (ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
            auto [t, v] = toValue(mouse);
            keys.push_back(CurveKey{ t, v });
            curve.SortKeys();
            edited = true;
        }
        // 中クリック: 近いキーを削除
        if (ImGui::IsMouseClicked(ImGuiMouseButton_Middle) && nearestIndex >= 0) {
            keys.erase(keys.begin() + nearestIndex);
            edited = true;
        }
    }

    // ドラッグ中の更新（キャンバス外へ出ても掴んだままにする）
    if (draggingCurve_ == &curve && draggingKeyIndex_ >= 0) {
        if (ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
            if (draggingKeyIndex_ < static_cast<int>(keys.size())) {
                auto [t, v] = toValue(mouse);
                keys[draggingKeyIndex_].time = t;
                keys[draggingKeyIndex_].value = v;
                edited = true;
            }
        } else {
            // 離した時点で時刻順に整列する（ドラッグ中に並べ替えると掴んだキーが入れ替わるため）
            curve.SortKeys();
            draggingCurve_ = nullptr;
            draggingKeyIndex_ = -1;
        }
    }

    ImGui::TextDisabled("左ドラッグ=移動 / 右クリック=追加 / 中クリック=削除   [%.2f - %.2f]",
        valueMin, valueMax);

    return edited;
}

bool ParticleEditor::DrawGradientEditor(const char* label, Gradient& gradient)
{
    auto& keys = gradient.GetKeysForEdit();
    bool edited = false;

    // ── プレビューバー ──
    const ImVec2 barPos = ImGui::GetCursorScreenPos();
    ImVec2 barSize(ImGui::GetContentRegionAvail().x, 24.0f);
    if (barSize.x < 80.0f) barSize.x = 80.0f;
    ImGui::InvisibleButton(label, barSize);

    ImDrawList* dl = ImGui::GetWindowDrawList();
    const ImVec2 barEnd(barPos.x + barSize.x, barPos.y + barSize.y);

    if (keys.empty()) {
        dl->AddRectFilled(barPos, barEnd, IM_COL32(28, 28, 32, 255));
        dl->AddText(ImVec2(barPos.x + 6.0f, barPos.y + 4.0f), IM_COL32(150, 150, 160, 255), "empty");
    } else {
        // 細かい短冊で塗ってグラデーションを再現する
        const int kSegments = 64;
        for (int i = 0; i < kSegments; ++i) {
            const float t0 = static_cast<float>(i) / kSegments;
            const float t1 = static_cast<float>(i + 1) / kSegments;
            const Vector4 c = gradient.Evaluate(t0);
            dl->AddRectFilled(
                ImVec2(barPos.x + barSize.x * t0, barPos.y),
                ImVec2(barPos.x + barSize.x * t1, barEnd.y),
                ImGui::ColorConvertFloat4ToU32(ImVec4(c.x, c.y, c.z, c.w)));
        }
        // キー位置のマーカー
        for (const GradientKey& key : keys) {
            const float x = barPos.x + barSize.x * std::clamp(key.time, 0.0f, 1.0f);
            dl->AddLine(ImVec2(x, barPos.y), ImVec2(x, barEnd.y), IM_COL32(255, 255, 255, 200), 1.5f);
        }
    }
    dl->AddRect(barPos, barEnd, IM_COL32(90, 90, 100, 255));

    // ── キー一覧 ──
    int removeIndex = -1;
    for (size_t i = 0; i < keys.size(); ++i) {
        ImGui::PushID(static_cast<int>(i));

        ImGui::SetNextItemWidth(90.0f);
        if (ImGui::DragFloat("##time", &keys[i].time, 0.01f, 0.0f, 1.0f)) {
            edited = true;
        }
        ImGui::SameLine();
        if (ImGui::ColorEdit4("##color", &keys[i].color.x,
            ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_AlphaBar | ImGuiColorEditFlags_AlphaPreview)) {
            edited = true;
        }
        ImGui::SameLine();
        if (ImGui::SmallButton("x")) {
            removeIndex = static_cast<int>(i);
        }

        ImGui::PopID();
    }

    if (removeIndex >= 0) {
        keys.erase(keys.begin() + removeIndex);
        edited = true;
    }

    if (ImGui::SmallButton("キー追加##grad")) {
        keys.push_back(GradientKey{ 1.0f, Vector4{ 1.0f, 1.0f, 1.0f, 1.0f } });
        edited = true;
    }

    if (edited) {
        gradient.SortKeys();
    }
    return edited;
}

#endif
