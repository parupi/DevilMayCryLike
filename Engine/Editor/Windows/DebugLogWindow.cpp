#include "DebugLogWindow.h"
#ifdef _DEBUG

#include "Editor/Core/EditorWindowRegistry.h"
#include "Utility/Logger.h"

#include <imgui/imgui.h>

void Editor::DrawDebugLogWindow()
{
	if (!EditorWindow::Begin("Debug Log", EditorWindow::Category::kEngine)) {
		return;
	}

	ImGui::TextUnformatted(Logger::GetImGuiLog().c_str());

	if (ImGui::Button("Clear")) {
		Logger::ClearImGuiLog();
	}

	EditorWindow::End();
}

#endif // _DEBUG
