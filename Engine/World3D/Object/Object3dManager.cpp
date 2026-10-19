#include "Object3dManager.h"
#include "World3D/Object/Object3d.h"
#include <cassert>
#include <cmath>
#include <World3D/Light/LightManager.h>
#include <World3D/Camera/CameraManager.h>
#include <Utility/ScopeProfiler.h>

Object3dManager& Object3dManager::GetInstance() {
	static Object3dManager instance;
	return instance;
}

void Object3dManager::Initialize(DirectXManager* directXManager, PSOManager* psoManager) {
	assert(directXManager);
	dxManager_ = directXManager;
	psoManager_ = psoManager;

	instanceBatcher_ = std::make_unique<StaticInstanceBatcher>();
	instanceBatcher_->Initialize(dxManager_, psoManager_);
}

void Object3dManager::BeginRenderFrame() {
	if (instanceBatcher_) {
		instanceBatcher_->BeginFrame();
	}
}

void Object3dManager::Finalize() {
	objects_.clear();

	dxManager_ = nullptr;
	psoManager_ = nullptr;
	defaultCamera_ = nullptr;

}

void Object3dManager::Update() {
	for (auto& object : objects_) {
		if (!object) continue;
		object->Update(deltaTime_);
	}
	PROF_COUNT("Obj:オブジェクト数", objects_.size());
}

void Object3dManager::DispatchSkinning() {
	// Compute用のPSOとRootSignatureはここで1回だけ設定する
	auto* commandList = dxManager_->GetCommandList();
	commandList->SetPipelineState(psoManager_->GetSkinningPSO());
	commandList->SetComputeRootSignature(psoManager_->GetSkinningSignature());

	for (auto& object : objects_) {
		if (!object) continue;
		object->DispatchSkinning();
	}
}

void Object3dManager::DrawForward() {
	for (auto& object : objects_) {
		if (!object) continue;
		// 描画方式がForwardでなければ次
		if (object->GetOption().drawPath != DrawPath::Forward) continue;
		// ブレンドモードが違っていたら新しくPSOを設定
		if (blendMode_ != object->GetOption().blendMode) {
			// ブレンドモードを最新にしておく
			blendMode_ = object->GetOption().blendMode;

			dxManager_->GetCommandList()->SetPipelineState(psoManager_->GetObjectPSO(blendMode_));			// PSOを設定
			dxManager_->GetCommandList()->SetGraphicsRootSignature(psoManager_->GetObjectSignature());
			dxManager_->GetCommandList()->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

			// psoのセットをしたらlight,cameraもバインドしておく
			LightManager::GetInstance().BindLightsToShader();
			CameraManager::GetInstance().BindCameraToShader();
		}

		// 描画
		object->Draw();
	}

	// 次回の為にNoneに戻しておく
	blendMode_ = BlendMode::kNone;
}

void Object3dManager::DrawDeferred() {
	// 今のカメラの視錐台を作っておき、Object3d::Draw がレンダラーごとに範囲外を省く。
	// ステージの床・壁・岩が200個ほどあり、カメラの後ろの物まで描画コマンドを積んでいた
	frustumValid_ = frustumCullingEnabled_ && defaultCamera_;
	if (frustumValid_) {
		BuildViewFrustum(defaultCamera_->GetViewProjectionMatrix());
	}
	drawnCount_ = 0;
	culledCount_ = 0;

	// 全オブジェクトの描画
	for (auto& object : objects_) {
		// 描画方式がDeferredでなければ次
		if (object->GetOption().drawPath != DrawPath::Deferred) continue;
		object->Draw();
	}

	// 床・壁など、まとめて描く側へ回したもの（カリング済み）をここで描く
	if (defaultCamera_) {
		instanceBatcher_->FlushGBuffer(defaultCamera_->GetViewProjectionMatrix());
	} else {
		instanceBatcher_->DiscardGBuffer();
	}

	// GBuffer の後に別の用途で Draw が呼ばれても省かないよう、ここで切っておく
	frustumValid_ = false;
	lastDrawnCount_ = drawnCount_;
	lastCulledCount_ = culledCount_;
	PROF_COUNT("Obj:GBuffer描画", lastDrawnCount_);
	PROF_COUNT("Obj:GBufferカリング", lastCulledCount_);
}

void Object3dManager::BuildViewFrustum(const Matrix4x4& viewProj) {
	// 行ベクトル規約（clip = v * M）なので、clip の各成分は M の「列」との内積になる。
	// D3D の深度は 0〜w なので、ニアは z >= 0、ファーは z <= w
	auto column = [&](int c, float out[4]) {
		for (int r = 0; r < 4; ++r) out[r] = viewProj.m[r][c];
	};
	float c0[4], c1[4], c2[4], c3[4];
	column(0, c0); column(1, c1); column(2, c2); column(3, c3);
	for (int i = 0; i < 4; ++i) {
		frustumPlanes_[0][i] = c3[i] + c0[i]; // 左
		frustumPlanes_[1][i] = c3[i] - c0[i]; // 右
		frustumPlanes_[2][i] = c3[i] + c1[i]; // 下
		frustumPlanes_[3][i] = c3[i] - c1[i]; // 上
		frustumPlanes_[4][i] = c2[i];         // ニア
		frustumPlanes_[5][i] = c3[i] - c2[i]; // ファー
	}
	for (auto& plane : frustumPlanes_) {
		const float length = std::sqrt(plane[0] * plane[0] + plane[1] * plane[1] + plane[2] * plane[2]);
		if (length > 1e-6f) {
			for (float& v : plane) v /= length;
		}
	}
}

bool Object3dManager::IsInViewFrustum(const Vector3& center, float radius) const {
	if (!frustumValid_) return true;
	for (const auto& plane : frustumPlanes_) {
		if (plane[0] * center.x + plane[1] * center.y + plane[2] * center.z + plane[3] < -radius) {
			return false;
		}
	}
	return true;
}

void Object3dManager::DrawShadow(const Matrix4x4& lightViewProj) {
	// 全オブジェクトの描画
	for (auto& object : objects_) {
		// 描画方式がDeferredでなければ次
		if (object->GetOption().drawPath != DrawPath::Deferred) continue;
		object->DrawShadow(lightViewProj);
	}
	// 床・壁など、まとめて描く側へ回したものをこのカスケードに描く
	instanceBatcher_->FlushShadow(lightViewProj);
}

void Object3dManager::AddObject(std::unique_ptr<Object3d> object) {
	objects_.push_back(std::move(object));
}

void Object3dManager::DeleteObject(const std::string& name) {
	for (auto& obj : objects_) {
		if (obj && obj->name_ == name) {
			obj->isAlive = false;
			obj->ResetObject();
		}
	}
}

void Object3dManager::DeleteAllObject() {
	for (auto& obj : objects_) {
		// 全オブジェクトの生存フラグを切る
		obj->isAlive = false;
		obj->ResetObject();
	}
}

void Object3dManager::RemoveDeadObject() {
	size_t before = objects_.size();

	objects_.erase(
		std::remove_if(objects_.begin(), objects_.end(),
			[](const std::unique_ptr<Object3d>& object) {
				return !object->isAlive;
			}),
		objects_.end()
	);

	size_t after = objects_.size();
	if (before != after) {
		char buf[128];
		sprintf_s(buf, "[Object3dManager] Removed %zu dead object(s)\n", before - after);
		OutputDebugStringA(buf);
	}
}


Object3d* Object3dManager::FindObject(std::string objectName) {
	for (auto& object : objects_) {
		if (!object) continue;
		if (object->name_ == objectName) {
			return object.get();
		}
	}
	Logger::Log("objectが見つかりませんでした");
	return nullptr;
}

std::vector<Object3d*> Object3dManager::GetAllObject() {
	std::vector<Object3d*> objects;

	for (auto& object : objects_) {
		if (!object) continue;
		objects.push_back(object.get());
	}

	return objects;
}

