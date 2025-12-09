#pragma once
#include <cstdint>
#include <wrl.h>
#include <d3d12.h>

class DirectXManager;
class PSOManager;

class ForwardRenderPath
{
public:
	ForwardRenderPath() = default;
	~ForwardRenderPath() = default;
	// 初期化
	void Initialize(DirectXManager* dxManager, PSOManager* psoManager);
	// 描画開始
	void BeginDraw();
	// 描画終了
	void EndDraw();
	// 生成したSrvの取得
	uint32_t GetSrvIndex() const { return srvIndex_; }
	uint32_t GetSrvForDepthIndex() const { return srvForDepthIndex_; }
private:
	// リソースを生成
	void CreateResource();

	DirectXManager* dxManager_;
	PSOManager* psoManager_;

	Microsoft::WRL::ComPtr<ID3D12Resource> rtvResource_;
	uint32_t rtvIndex_ = 0;

	uint32_t srvIndex_ = 0;

	Microsoft::WRL::ComPtr<ID3D12Resource> depthBuffer_;
	uint32_t dsvIndex_ = 0;
	// Depthの情報をSRVとして読めるようにする
	uint32_t srvForDepthIndex_ = 0;
};

