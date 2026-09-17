#pragma once
#include "GameObject/Character/MovementBounds.h"
#include <Math/Vector3.h>
#include <random>
#include <vector>

/// <summary>
/// 強制戦闘エリアの範囲を、格子状の光の壁で可視化する演出。
///
/// エリアの側面4枚に縦線・横線からなる格子を敷き、その線上に等間隔で並べた
/// 「格子点」からパーティクルを発生させることで、光の格子が浮かんで見える。
/// エリアは床の向きに合わせて回転しているので、壁も MovementBounds の水平2軸に沿って張る。
/// 高さはエリアの箱そのまま（床に埋まる部分があってもよい）。
///
/// 壁全体を常時出すと画面がちらついてしまうため、発生させるのはプレイヤーの近く
/// （kVisibleRadius 以内）の格子点だけに絞ってある。境界がくっきり切れないよう、
/// kFadeStartRadius から外側は発生する確率を落として自然に散らしている。
/// 距離は高さ込みの3次元で見るので、見えるのはプレイヤーを中心とした丸い範囲になる。
/// </summary>
class BattleAreaWallEffect {
public:
	/// <summary>可視化するエリアを渡して格子を構築する</summary>
	void Initialize(const MovementBounds& bounds);

	/// <summary>壁の表示を開始する</summary>
	void Start();

	/// <summary>壁の表示を終了する（発生済みの粒子は寿命で自然に消える）</summary>
	void Stop();

	/// <param name="viewerPos">この位置の近くの壁だけを光らせる。通常はプレイヤーの位置</param>
	void Update(float deltaTime, const Vector3& viewerPos);

	bool IsActive() const { return isActive_; }

private:
	/// <summary>エリア側面4枚の格子点を作る。点が多すぎる場合は格子を粗くして作り直す</summary>
	void BuildGridPoints(const MovementBounds& bounds);

	/// <summary>格子点を一巡ぶんずつ順に、viewerPos の近くのものだけ発生させる</summary>
	void EmitNextPoints(const Vector3& viewerPos);

	/// <summary>
	/// 同時に見える格子点数の最悪値を見積もる。
	/// プレイヤーが壁際・角に張り付いたときが最悪なので、その代表位置で数えて最大を返す。
	/// </summary>
	size_t EstimateMaxVisiblePoints(const MovementBounds& bounds) const;

	// パーティクルグループ名（GameScene で CreateParticleGroup 済みであること）
	static constexpr const char* kGroupName = "BattleAreaWall";

	// 格子の線の間隔(m)。この間隔で縦線・横線を引く
	static constexpr float kLineSpacing = 1.6f;
	// 線を構成する点の間隔(m)。細かいほど線がはっきり見える
	static constexpr float kDotSpacing = 0.35f;
	// パーティクルを発生させる間隔(秒)
	static constexpr float kEmitInterval = 0.05f;
	// 全ての格子点を一巡する時間(秒)。短いほど壁が濃くなる
	static constexpr float kCycleDuration = 1.5f;
	// エリア全体の格子点数の上限。背の高いエリアだと点数が伸びるので余裕をもたせてある
	// （見える量は kVisibleRadius で決まるので、ここは主にメモリと巡回コストの歯止め）
	static constexpr size_t kMaxPoints = 30000;
	// 同時に見える格子点数の上限。実際に発生するパーティクル数はこれで決まるので、
	// これを超える場合は格子を粗くする（＝広さではなく「見える量」で密度を決める）
	static constexpr size_t kMaxVisiblePoints = 1400;

	// この距離(m)より遠い壁は出さない
	static constexpr float kVisibleRadius = 8.0f;
	// この距離(m)から外側は、遠いほど発生確率を落として薄れさせる
	static constexpr float kFadeStartRadius = 4.0f;

	// 同時生存数の上限。パーティクルグループの描画上限を超えないための保険
	static constexpr size_t kMaxAliveParticles = 1800;

	// 格子点（ワールド座標）。空間的に均等に光らせるため構築時にシャッフルしてある
	std::vector<Vector3> points_;
	// 次に発生させる格子点のインデックス
	size_t cursor_ = 0;

	float emitTimer_ = 0.0f;
	bool isActive_ = false;

	std::mt19937 rng_{ std::random_device{}() };
};
