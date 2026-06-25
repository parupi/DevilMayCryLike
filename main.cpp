#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#include "Application/MyGameTitle.h"

// Windowsアプリでのエントリーポイント(main関数)
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);

//#ifdef _DEBUG
//#include <crtdbg.h>
//	// このアロケーション番号で自動ブレーク
//	_CrtSetBreakAlloc(3584);
//	//_CrtSetBreakAlloc(94578);
//#endif


	std::unique_ptr<GuchisFramework> game = std::make_unique<MyGameTitle>();

#ifdef _DEBUG
	D3DResourceLeakChecker leakChecker;
#endif

	game->Run();

	return 0;
}
