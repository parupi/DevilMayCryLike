#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#include "MyGameTitle.h"

// Windowsアプリでのエントリーポイント(main関数)
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);

	std::unique_ptr<GuchisFramework> game = std::make_unique<MyGameTitle>();

	game->Run();

#ifdef _DEBUG
	//D3DResourceLeakChecker leakCheck;
#endif // _DEBUG

	return 0;
}
