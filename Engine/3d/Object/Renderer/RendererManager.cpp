#include "RendererManager.h"

RendererManager* RendererManager::instance = nullptr;
std::once_flag RendererManager::initInstanceFlag;

RendererManager* RendererManager::GetInstance()
{
	std::call_once(initInstanceFlag, []() {
		instance = new RendererManager();
		});
	return instance;
}

void RendererManager::Initialize(DirectXManager* dxManager, SrvManager* srvManager)
{
	dxManager_ = dxManager;
	srvManager_ = srvManager;
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

void RendererManager::RemoveDeadObjects()
{
	renders_.erase(
		std::remove_if(renders_.begin(), renders_.end(),
			[](const std::unique_ptr<BaseRenderer>& renderer) {
				return !renderer->isAlive;
			}),
		renders_.end()
	);
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

