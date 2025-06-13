#include "MyGameTitle.h"

// Windowsアプリでのエントリーポイント(main関数)
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {

#ifdef _DEBUG
	D3DResourceLeakChecker leakCheck;
#endif // _DEBUG

	std::unique_ptr<GuchisFramework> game = std::make_unique<MyGameTitle>();

	game->Run();

	return 0;
}
