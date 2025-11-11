#include "RendererManager.h"
#include "3d/Object/Model/BaseModel.h"

RendererManager* RendererManager::instance = nullptr;
std::once_flag RendererManager::initInstanceFlag;

RendererManager* RendererManager::GetInstance()
{
	std::call_once(initInstanceFlag, []() {
		instance = new RendererManager();
		});
	return instance;
}

void RendererManager::Initialize(DirectXManager* dxManager, PSOManager* psoManager)
{
	dxManager_ = dxManager;
	srvManager_ = dxManager_->GetSrvManager();
	psoManager_ = psoManager;

}

void RendererManager::Finalize()
{
	renders_.clear();

	dxManager_ = nullptr;

	delete instance;
	instance = nullptr;
}

void RendererManager::Update()
{
	
}

void RendererManager::RenderGBufferPass()
{
	//auto cmd = dxManager_->GetCommandList();

	//// GBuffer用PSO/RSをセット
	//cmd->SetPipelineState(psoManager_->GetDeferredPSO());
	//cmd->SetGraphicsRootSignature(psoManager_->GetDeferredSignature());

	//// GBufferのパスをセット
	//gBufferPass->Begin();

	//// ----------- Model全体Draw（Forwardと別で管理）---------------
	//auto& models = renders_;
	//for (auto& r : models)
	//{
	//	r->GetModel()->DrawGBuffer();
	//}
}

void RendererManager::RemoveDeadObjects()
{
	size_t before = renders_.size();

	renders_.erase(
		std::remove_if(renders_.begin(), renders_.end(),
			[](const std::unique_ptr<BaseRenderer>& renderer) {
				return !renderer->isAlive;
			}),
		renders_.end()
	);

	size_t after = renders_.size();
	if (before != after) {
		char buf[128];
		sprintf_s(buf, "[RendererManager] Removed %zu dead renderer(s)\n", before - after);
		OutputDebugStringA(buf);
	}
}

void RendererManager::AddRenderer(std::unique_ptr<BaseRenderer> render)
{
	renders_.push_back(std::move(render));
}

BaseRenderer* RendererManager::FindRender(std::string renderName)
{
	for (auto& render : renders_) {
		if (render->name_ == renderName) {
			return render.get();
		}
	}
	Logger::Log("renderが見つかりませんでした");
	return nullptr;
}

