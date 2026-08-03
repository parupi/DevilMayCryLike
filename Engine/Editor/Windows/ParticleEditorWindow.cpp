#include "ParticleEditorWindow.h"
#ifdef _DEBUG

#include "Editor/Core/EditorContext.h"
#include "Editor/Windows/ParticleEditor.h"
#include "Graphics/Rendering/Particle/ParticleManager.h"

void Editor::DrawParticleEditorWindows()
{
	ParticleManager* particles = Ctx().particleManager;
	if (!particles) {
		return;
	}
	if (ParticleEditor* editor = particles->GetEditor()) {
		// ParticleEditor 自身が EditorWindow::Begin/End で4つのウィンドウを開く
		editor->Draw();
	}
}

#endif // _DEBUG
