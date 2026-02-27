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
	void BeginDraw(uint32_t rtvIndex, uint32_t dsvIndex);
	// 描画終了
	void EndDraw();
private:
	DirectXManager* dxManager_;
	PSOManager* psoManager_;
};

