#pragma once
#ifdef _DEBUG

#include <string>
#include <vector>
#include <unordered_map>
#include <map>
#include <imgui-node-editor/imgui_node_editor.h>

namespace ed = ax::NodeEditor;

class ParticleManager;
class GlobalVariables;

class ParticleEditor
{
public:
    void Initialize(ParticleManager* manager);
    void Finalize();
    void Draw();

private:
    // ─── 3 ウィンドウ ─────────────────────────────────────────────
    void DrawParticleWindow();
    void DrawEmitterWindow();
    void DrawNodeGraph();

    // ─── Node ID ─────────────────────────────────────────────────
    uintptr_t GetOrCreateEmitterNodeId(const std::string& name);
    uintptr_t GetOrCreateParticleNodeId(const std::string& name);
    uintptr_t GetOrCreateLinkId(const std::string& emitterName, const std::string& particleName);

    uintptr_t OutputPinId(uintptr_t nodeId) const { return nodeId + 100000; }
    uintptr_t InputPinId (uintptr_t nodeId) const { return nodeId + 200000; }

    bool IsOutputPin(uintptr_t id) const { return id > 100000 && id < 200000; }
    bool IsInputPin (uintptr_t id) const { return id >= 200000 && id < 300000; }

    std::string EmitterNameFromOutputPin(uintptr_t pinId) const;
    std::string ParticleNameFromInputPin(uintptr_t pinId) const;

private:
    ParticleManager* manager_ = nullptr;
    GlobalVariables* global_  = nullptr;

    int selectedParticleIndex_ = 0;
    int selectedEmitterIndex_  = 0;

    // ─── Node editor ─────────────────────────────────────────────
    ed::EditorContext* nodeCtx_ = nullptr;

    std::unordered_map<std::string, uintptr_t> emitterNodeIds_;
    std::unordered_map<std::string, uintptr_t> particleNodeIds_;
    std::unordered_map<uintptr_t, std::string> nodeIdToEmitterName_;
    std::unordered_map<uintptr_t, std::string> nodeIdToParticleName_;

    // (emitterName, particleName) → link ID
    std::map<std::pair<std::string, std::string>, uintptr_t> linkIds_;

    uintptr_t nextNodeId_ = 1;
    uintptr_t nextLinkId_ = 300001;

    bool firstFrame_ = true;  // 初回フレームでノード位置を設定する
};

#endif
