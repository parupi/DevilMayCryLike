#include "MyGameTitle.h"

// Windowsアプリでのエントリーポイント(main関数)
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {
	std::unique_ptr<GuchisFramework> game = std::make_unique<MyGameTitle>();

	game->Run();

	return 0;
}
