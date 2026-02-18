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
	// PSOマネージャーの初期化
	psoManager = std::make_unique<PSOManager>();
	psoManager->Initialize(dxManager.get());

	//gBufferManager = std::make_unique<GBufferManager>();
	//gBufferManager->Initialize(dxManager.get());

	// 入力の初期化
	Input::GetInstance()->Initialize();
	// Audioの初期化
	Audio::GetInstance()->Initialize();
	// DeltaTime
	DeltaTime::Initialize();
}

void GuchisFramework::Finalize()
{
	Input::GetInstance()->Finalize();
	Audio::GetInstance()->Finalize();
	//gBufferManager->Finalize();
	//gBufferPath.reset();
	//lightingPath.reset();
	psoManager->Finalize();
	winManager->Finalize();
	
	dxManager->Finalize();
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
			// 描画が終わった後に削除
			RemoveObjects();
			Update();
			Draw();

		}
	} catch (const std::exception& e) {
		MessageBoxA(nullptr, e.what(), "例外発生", MB_OK | MB_ICONERROR);
	}
	Finalize();
}
