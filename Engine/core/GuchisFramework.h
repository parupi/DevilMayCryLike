#pragma once
#include "Platform/WindowManager.h"
#include "Graphics/Device/DirectXManager.h"
#include "Input/Input.h"
#ifdef _DEBUG
#include "Debugger/LeakChecker.h"
#endif // _DEBUG

#include <Scene/AbstractSceneFactory.h>
#include <Audio/Audio.h>
#include "Graphics/Rendering/PSO/PSOManager.h"
#include "Graphics/Rendering/RenderPath/RenderPipeline.h"
#include "Core/EngineContext.h"

class GuchisFramework
{
public:
	virtual ~GuchisFramework() = default;

public:
	// 初期化
	virtual void Initialize();
	// 終了
	virtual void Finalize();
	// 毎フレーム更新
	virtual void Update();
	// 描画
	virtual void Draw() = 0;
	// オブジェクト削除
	virtual void RemoveObjects() = 0;

	// 終了チェック
	virtual bool IsEndRequest() { return winManager->ProcessMessage(); }
	// デバッグ用に取得できるようにしておく
	DirectXManager* GetDXManager() const { return dxManager.get(); }

public:
	// 実行
	void Run();

protected:
	std::unique_ptr<WindowManager> winManager = nullptr;
	std::unique_ptr<DirectXManager> dxManager = nullptr;
	std::unique_ptr<PSOManager> psoManager = nullptr;
	std::unique_ptr<AbstractSceneFactory> sceneFactory_ = nullptr;
	std::unique_ptr<RenderPipeline> renderPipeline_;

	// 全サービスへのビュー。MyGameTitle::Initialize() で完全に設定される
	EngineContext ctx_;
};

