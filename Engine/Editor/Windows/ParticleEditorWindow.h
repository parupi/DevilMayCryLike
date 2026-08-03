#pragma once
#ifdef _DEBUG

namespace Editor {

/// <summary>
/// パーティクル／VFX のエディタ（Particle Groups / Emitters / VFX / Particle Node Graph）を回す。
/// 実体は ParticleManager が持っている ParticleEditor。
/// もとは ParticleManager::Update() の末尾から呼ばれていた。
/// </summary>
void DrawParticleEditorWindows();

} // namespace Editor

#endif // _DEBUG
