#pragma once
#include <mutex>
#include <base/DirectXManager.h>
#include "base/SrvManager.h"
#include "BaseRenderer.h"
#include <base/GBufferManager.h>
#include <base/PSOManager.h>



class RendererManager
{
private:
	static RendererManager* instance;
	static std::once_flag initInstanceFlag;

	RendererManager() = default;
	~RendererManager() = default;
	RendererManager(RendererManager&) = default;
	RendererManager& operator=(RendererManager&) = default;
public:
	// インスタンスの取得
	static RendererManager* GetInstance();
	// 初期化処理
	void Initialize(DirectXManager* dxManager, PSOManager* psoManager);
	// 終了処理
	void Finalize();
	// 更新処理
	void Update();
	// GBufferに描画する準備
	void RenderGBufferPass();
	// オブジェクト削除
	void RemoveDeadObjects();
	// レンダー追加処理
	void AddRenderer(std::unique_ptr<BaseRenderer> render);

	BaseRenderer* FindRender(std::string renderName);

	DirectXManager* GetDxManager() { return dxManager_; }
	SrvManager* GetSrvManager() { return srvManager_; }
private:
	//std::unique_ptr<GBufferPass> gBufferPass = nullptr;

	DirectXManager* dxManager_ = nullptr;
	SrvManager* srvManager_ = nullptr;
	PSOManager* psoManager_ = nullptr;

	std::vector<std::unique_ptr<BaseRenderer>> renders_;
};

