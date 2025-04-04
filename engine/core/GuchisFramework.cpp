#include "GuchisFramework.h"

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

	dxManager->CreateSRVForOffScreen(srvManager.get());
	// 入力の初期化
	Input::GetInstance()->Initialize();
	// Audioの初期化
	Audio::GetInstance()->Initialize();
}

void GuchisFramework::Finalize()
{
	Input::GetInstance()->Finalize();
	Audio::GetInstance()->Finalize();
	SceneManager::GetInstance()->Finalize();
	winManager->Finalize();
	dxManager->Finalize();
}

void GuchisFramework::Update()
{
	Input::GetInstance()->Update();
	SceneManager::GetInstance()->Update();
}

void GuchisFramework::Run()
{
	Initialize();

	while (true) {
		Update();

		//Windowにメッセージが来てたら最優先で処理させる
		if (IsEndRequest()) {
			// ゲームループを抜ける
			break;
		}

		Draw();
	}
	Finalize();
}
