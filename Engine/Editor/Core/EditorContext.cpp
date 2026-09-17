#include "EditorContext.h"
#ifdef _DEBUG

namespace {
// 全メンバ nullptr。SetContext 前に Ctx() が呼ばれてもこれを返す
EngineContext g_context{};
bool g_hasContext = false;
} // namespace

void Editor::SetContext(const EngineContext& context)
{
	g_context = context;
	g_hasContext = true;
}

bool Editor::HasContext()
{
	return g_hasContext;
}

const EngineContext& Editor::Ctx()
{
	return g_context;
}

#endif // _DEBUG
