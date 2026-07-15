#include "RendererManager.h"
#include "World3D/Object/Model/BaseModel.h"

RendererManager& RendererManager::GetInstance() {
	static RendererManager instance;
	return instance;
}

void RendererManager::Initialize(DirectXManager* dxManager, PSOManager* psoManager) {
	dxManager_ = dxManager;
	srvManager_ = dxManager_->GetSrvManager();
	psoManager_ = psoManager;

}

void RendererManager::Finalize() {
	renders_.clear();

	dxManager_ = nullptr;

}

void RendererManager::Update() {

}

void RendererManager::DeleteAllRenderer() {
	for (auto& renderer : renders_) {
		renderer->isAlive = false;
	}
}

void RendererManager::RemoveDeadObjects() {
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

void RendererManager::AddRenderer(std::unique_ptr<BaseRenderer> render) {
	renders_.push_back(std::move(render));
}

BaseRenderer* RendererManager::FindRender(std::string renderName) {
	for (auto& render : renders_) {
		if (render->name_ == renderName) {
			return render.get();
		}
	}
	Logger::Log("renderが見つかりませんでした");
	return nullptr;
}

