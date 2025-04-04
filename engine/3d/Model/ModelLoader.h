#pragma once
#include "DirectXManager.h"
#include <SrvManager.h>
class ModelLoader
{
public: // メンバ関数
	void Initialize(DirectXManager* dxManager, SrvManager* srvManager);

private:

	DirectXManager* dxManager_ = nullptr;
	SrvManager* srvManager_ = nullptr;

public: // ゲッター//セッター//
	DirectXManager* GetDxManager() const { return dxManager_; }
	SrvManager* GetSrvManager() const { return srvManager_; }
};

