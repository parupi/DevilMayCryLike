#pragma once
#ifdef _DEBUG

#include <vector>
#include <string>

class ParticleManager;
class GlobalVariables;

class ParticleEditor
{
public:
    void Initialize(ParticleManager* manager);
    void Draw();

private:
    void DrawParticleEditor();
    void DrawEmitterEditor();

private:
    ParticleManager* manager_ = nullptr;
    GlobalVariables* global_ = nullptr;

    int selectedParticleIndex_ = 0;
    int selectedEmitterIndex_ = 0;
};

#endif