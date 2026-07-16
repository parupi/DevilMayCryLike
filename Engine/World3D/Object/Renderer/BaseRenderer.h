#pragma once
#include <World3D/WorldTransform.h>
#include <Math/Vector4.h>
#include <memory>
#include <string>
class BaseModel;

// 描画パス固有の機能は IDeferredDrawable / IShadowCaster を参照
class BaseRenderer
{
public:
	virtual ~BaseRenderer() = default;

	virtual void Update(WorldTransform* parentTransform) = 0;
	virtual void Draw() = 0;
#ifdef _DEBUG
	virtual void DebugGui(size_t index) = 0;
#endif // DEBUG

	virtual WorldTransform* GetWorldTransform() const = 0;
	virtual BaseModel* GetModel() const = 0;

	// ── レンダラー単位のDissolve上書き ──
	// モデル(マテリアル)は複数オブジェクトで共有されるため、
	// 個別オブジェクトだけを溶かす場合はこちらを使う（GBufferパスでルート定数として渡される）。
	// threshold: -1で無効(マテリアル設定を使用)、0〜1でディゾルブ量
	void SetDissolveThreshold(float t) { dissolveThreshold_ = t; }
	float GetDissolveThreshold() const { return dissolveThreshold_; }
	void SetDissolveEdgeWidth(float w) { dissolveEdgeWidth_ = w; }
	void SetDissolveEdgeColor(const Vector4& color) { dissolveEdgeColor_ = color; }

	// ── レンダラー単位のエミッシブティント ──
	// rgb = 加算する発光色、a = 強度（0で無効）。
	// スーパーアーマー中の紫発光など、個別オブジェクトの一時的な色変化に使う。
	void SetEmissiveTint(const Vector4& color) { emissiveTint_ = color; }
	const Vector4& GetEmissiveTint() const { return emissiveTint_; }

	std::string name_;
	bool isAlive = true;

protected:
	std::unique_ptr<WorldTransform> localTransform_;

	float dissolveThreshold_ = -1.0f;
	float dissolveEdgeWidth_ = 0.05f;
	Vector4 dissolveEdgeColor_ = { 1.0f, 0.3f, 0.0f, 8.0f };
	Vector4 emissiveTint_ = { 0.0f, 0.0f, 0.0f, 0.0f };
};

