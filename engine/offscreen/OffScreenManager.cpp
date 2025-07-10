#include "OffScreenManager.h"

OffScreenManager* OffScreenManager::instance = nullptr;
std::once_flag OffScreenManager::initInstanceFlag;

OffScreenManager* OffScreenManager::GetInstance()
{
	std::call_once(initInstanceFlag, []() {
		instance = new OffScreenManager();
		});
	return instance;
}

void OffScreenManager::Initialize(DirectXManager* dxManager, PSOManager* psoManager)
{
	dxManager_ = dxManager;
	psoManager_ = psoManager;
}

void OffScreenManager::Finalize()
{
	// オフスクリーンエフェクトの解放
	effects_.clear();

	dxManager_ = nullptr;
	psoManager_ = nullptr;

	delete instance;
	instance = nullptr;
}

void OffScreenManager::Update()
{
	for (size_t i = 0; i < effects_.size(); i++) {
		effects_[i]->Update();
	}
}

void OffScreenManager::Draw()
{
	bool isDraw = false;
	for (size_t i = 0; i < effects_.size(); i++) {
		if (effects_[i]->IsActive()) {
			effects_[i]->Draw();
			isDraw = true;
		}
	}
	// もし描画されていなければ
	if (!isDraw) {
		dxManager_->GetCommandList()->SetPipelineState(psoManager_->GetOffScreenPSO(OffScreenEffectType::kNone));
		dxManager_->GetCommandList()->SetGraphicsRootSignature(psoManager_->GetOffScreenSignature());
		dxManager_->GetCommandList()->SetGraphicsRootDescriptorTable(0, dxManager_->GetSrvHandle().second);
		dxManager_->GetCommandList()->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		dxManager_->GetCommandList()->DrawInstanced(3, 1, 0, 0);
	}
}

void OffScreenManager::AddEffect(std::unique_ptr<BaseOffScreen> effect)
{
	effects_.emplace_back(std::move(effect));
}
