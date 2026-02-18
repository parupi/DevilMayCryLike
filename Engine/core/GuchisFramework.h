#pragma once
#include "base/WindowManager.h"
#include "Graphics/Device/DirectXManager.h"
#include "input/Input.h"
#ifdef _DEBUG
#include "debuger/LeakChecker.h"
#endif // _DEBUG

#include <scene/SceneManager.h>
#include <scene/SceneFactory.h>
#include <scene/AbstractSceneFactory.h>
#include <audio/Audio.h>
#include "Graphics/Rendering/PSO/PSOManager.h"
#include <3d/SkySystem/SkySystem.h>
//#include <base/GBufferManager.h>
//#include <base/GBufferPath.h>
//#include <base/LightingPath.h>
#include <Graphics/Rendering/RenderPath/ForwardRenderPath.h>
#include <Graphics/Rendering/RenderPath/CompositePath.h>
#include <Graphics/Rendering/Shadow/CascadedShadowMap.h>
#include <Graphics/Rendering/RenderPath/Shadow/ShadowPass.h>
#include "Graphics/Rendering/RenderPath/RenderPipeline.h"

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
	//std::unique_ptr<GBufferManager> gBufferManager = nullptr;
	//std::unique_ptr<GBufferPath> gBufferPath = nullptr;
	//std::unique_ptr<LightingPath> lightingPath = nullptr;
	//std::unique_ptr<ForwardRenderPath> forwardPath = nullptr;
	//std::unique_ptr<CompositePath> compositePath = nullptr;
	std::unique_ptr<RenderPipeline> renderPipeline_;

	std::unique_ptr<CascadedShadowMap> csm = nullptr;
	std::unique_ptr<ShadowPass> shadowPath = nullptr;
};

