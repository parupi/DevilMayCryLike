#pragma once
#include <vector>
#include <Math/Vector3.h>
#include <Math/Vector4.h>

class BaseRenderer;

/// <summary>
/// 登録したレンダラーをディゾルブで溶かして消し、黒いもやを撒くエフェクト。
/// 敵の死亡演出（EnemyAppearanceEffect の Dissolve 段階）と同じ見せ方を、
/// プレイヤーなど敵以外でも使えるように切り出したもの。
///
/// ディゾルブはレンダラー単位の上書き（BaseRenderer::SetDissolveThreshold）なので、
/// 同じモデルを使う他のオブジェクトには影響しない。
/// **レンダラーはシーンをまたぐと作り直されるので、シーンの初期化で Reset() すること**
/// （溶けきった値のまま残ると、次のプレイで姿が見えなくなる）。
/// </summary>
class DissolveOutEffect {
public:
	/// <summary>溶かす対象のレンダラーを追加する（体・武器など）</summary>
	void AddRenderer(BaseRenderer* renderer);

	/// <summary>溶けていく縁の発光色（rgb + 強度）。敵は赤、プレイヤーは青白にしている</summary>
	void SetEdgeColor(const Vector4& color) { edgeColor_ = color; }
	/// <summary>溶けきるまでの秒数</summary>
	void SetDuration(float seconds) { duration_ = seconds; }

	/// <summary>溶かし始める</summary>
	void Start();

	/// <summary>
	/// 毎フレーム呼ぶ。position は黒いもやを撒く位置（倒れた体に付いてくるよう毎回渡す）
	/// </summary>
	void Update(float deltaTime, const Vector3& position);

	/// <summary>上書きを解除して元の見た目に戻す</summary>
	void Reset();

	bool IsPlaying() const { return isPlaying_; }
	bool IsFinished() const { return isFinished_; }
	/// <summary>0（そのまま）〜1（溶けきり）</summary>
	float GetProgress() const;

private:
	void Apply(float threshold);
	void Emit(int count, const Vector3& position);

	std::vector<BaseRenderer*> renderers_;

	bool isPlaying_ = false;
	bool isFinished_ = false;
	float timer_ = 0.0f;
	float emitTimer_ = 0.0f;
	float duration_ = 0.9f;
	Vector4 edgeColor_{ 0.9f, 0.15f, 0.05f, 8.0f };

	// 立ち上る黒いもや。敵の死亡演出と同じグループを共有する
	static constexpr const char* kSmokeGroup = "DeathSmoke";
	static constexpr float kEmitInterval = 0.05f; // もやの発生間隔[s]
	// カメラが寄る死亡演出で使うので、敵のもや（EnemyAppearanceEffect）より控えめにしている。
	// 濃くすると溶けていく体がもやに隠れて見えなくなる
	static constexpr int kBurstCount = 8;         // 溶け始めに一気に撒く数
	static constexpr int kEmitCount = 1;          // 1回あたりの発生数
	static constexpr float kEdgeWidth = 0.08f;    // ディゾルブ縁の太さ
};
