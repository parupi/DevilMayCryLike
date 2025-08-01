#include "GuchisFramework.h"
#include "base/utility/DeltaTime.h"

void GuchisFramework::Initialize()
{
	// WinDowsAPIの初期化
	winManager = std::make_unique<WindowManager>();
	winManager->Initialize();
	// DirectXの初期化
	dxManager = std::make_unique<DirectXManager>();
	dxManager->Initialize(winManager.get());
	// SRVマネージャーの初期化
	srvManager = std::make_unique<SrvManager>();
	srvManager->Initialize(dxManager.get());
	// PSOマネージャーの初期化
	psoManager = std::make_unique<PSOManager>();
	psoManager->Initialize(dxManager.get());

	//dxManager->CreateSRVForOffScreen(srvManager.get());
	// 入力の初期化
	Input::GetInstance()->Initialize();
	// Audioの初期化
	Audio::GetInstance()->Initialize();
	// DeltaTime
	DeltaTime::Initialize();
}

void GuchisFramework::Finalize()
{
	SceneManager::GetInstance()->Finalize();
	Input::GetInstance()->Finalize();
	Audio::GetInstance()->Finalize();
	psoManager->Finalize();
	winManager->Finalize();
	srvManager->Finalize();
	dxManager->Finalize(); // デバイスが最後
}

void GuchisFramework::Update()
{
	Input::GetInstance()->Update();
	DeltaTime::Update();
	SceneManager::GetInstance()->Update();
}

void GuchisFramework::Run() {
	try {
		Initialize();
		while (true) {
			if (IsEndRequest()) break;
			Update();
			Draw();
		}
	} catch (const std::exception& e) {
		MessageBoxA(nullptr, e.what(), "例外発生", MB_OK | MB_ICONERROR);
	}
	Finalize();
}
