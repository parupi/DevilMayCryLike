#include "BattleAreaWallEffect.h"
#include "Graphics/Rendering/Particle/ParticleManager.h"
#include <algorithm>
#include <cmath>
#include <random>

namespace {
	// 長さ length を spacing 間隔で分割したときの分割数（両端を必ず含めるため最低1）
	int StepCount(float length, float spacing) {
		if (spacing <= 0.0f) return 1;
		return (std::max)(1, static_cast<int>(std::lround(length / spacing)));
	}
}

void BattleAreaWallEffect::Initialize(const MovementBounds& bounds) {
	BuildGridPoints(bounds);
}

void BattleAreaWallEffect::Start() {
	if (points_.empty()) return;
	isActive_ = true;
	emitTimer_ = 0.0f;
}

void BattleAreaWallEffect::Stop() {
	isActive_ = false;
}

void BattleAreaWallEffect::Update(float deltaTime, const Vector3& viewerPos) {
	if (!isActive_) return;

	emitTimer_ += deltaTime;
	while (emitTimer_ >= kEmitInterval) {
		EmitNextPoints(viewerPos);
		emitTimer_ -= kEmitInterval;
	}
}

void BattleAreaWallEffect::BuildGridPoints(const MovementBounds& bounds) {
	// エリアの箱の高さをそのまま使う。床に埋まる部分が出てもよい
	// （プレイヤーの高さを基準にすると、ジャンプで進入したとき格子が宙に浮いてしまう）
	const float height = bounds.maxY - bounds.minY;
	const float sizeU = bounds.halfU * 2.0f;
	const float sizeV = bounds.halfV * 2.0f;
	if (height <= 0.0f || sizeU <= 0.0f || sizeV <= 0.0f) return;

	float lineSpacing = kLineSpacing;
	float dotSpacing = kDotSpacing;

	// 1枚の壁を格子点で埋める。origin から axis 方向へ length 進んだ範囲が壁の横幅にあたる
	auto addWall = [&](const Vector3& origin, const Vector3& axis, float length) {
		const int lineStepsH = StepCount(length, lineSpacing); // 縦線の本数-1
		const int lineStepsV = StepCount(height, lineSpacing); // 横線の本数-1
		const int dotStepsH = StepCount(length, dotSpacing);   // 横線を描く点の数-1
		const int dotStepsV = StepCount(height, dotSpacing);   // 縦線を描く点の数-1

		// 縦線：横方向に等間隔で並べた各線を、Y方向の点で描く
		for (int i = 0; i <= lineStepsH; ++i) {
			const Vector3 base = origin + axis * (length * static_cast<float>(i) / lineStepsH);
			for (int j = 0; j <= dotStepsV; ++j) {
				const float y = bounds.minY + height * static_cast<float>(j) / dotStepsV;
				points_.push_back({ base.x, y, base.z });
			}
		}

		// 横線：高さ方向に等間隔で並べた各線を、横方向の点で描く
		for (int j = 0; j <= lineStepsV; ++j) {
			const float y = bounds.minY + height * static_cast<float>(j) / lineStepsV;
			for (int i = 0; i <= dotStepsH; ++i) {
				const Vector3 base = origin + axis * (length * static_cast<float>(i) / dotStepsH);
				points_.push_back({ base.x, y, base.z });
			}
		}
		};

	// エリアの4隅（水平面上）。壁はこの隅を結ぶ辺に沿って張る
	const Vector3 u = bounds.axisU * bounds.halfU;
	const Vector3 v = bounds.axisV * bounds.halfV;
	const Vector3 cornerNN = bounds.center - u - v;
	const Vector3 cornerNP = bounds.center - u + v;
	const Vector3 cornerPN = bounds.center + u - v;

	// 一度に見える点が多すぎるとパーティクルが描画上限に張り付いてしまうため、
	// 上限に収まるまで間隔を広げて（＝格子を粗くして）作り直す
	for (int attempt = 0; attempt < 8; ++attempt) {
		points_.clear();
		addWall(cornerNN, bounds.axisU, sizeU); // -V 側の壁
		addWall(cornerNP, bounds.axisU, sizeU); // +V 側の壁
		addWall(cornerNN, bounds.axisV, sizeV); // -U 側の壁
		addWall(cornerPN, bounds.axisV, sizeV); // +U 側の壁

		if (points_.size() <= kMaxPoints &&
			EstimateMaxVisiblePoints(bounds) <= kMaxVisiblePoints) {
			break;
		}

		lineSpacing *= 1.5f;
		dotSpacing *= 1.5f;
	}

	// 順に発生させたときに壁が均等に光るよう、あらかじめ順序をばらしておく
	// （並び順のまま発生させると、格子を舐めるように光が走ってしまう）
	std::shuffle(points_.begin(), points_.end(), rng_);

	cursor_ = 0;
}

size_t BattleAreaWallEffect::EstimateMaxVisiblePoints(const MovementBounds& bounds) const {
	// プレイヤーが壁の中央や角に張り付いたときが最悪ケースになる。
	// 高さは箱の中央で見る（上下どちらにも壁が続くので最も点が多くなる）
	const float midY = (bounds.minY + bounds.maxY) * 0.5f;
	const Vector3 c = { bounds.center.x, midY, bounds.center.z };
	const Vector3 u = bounds.axisU * bounds.halfU;
	const Vector3 v = bounds.axisV * bounds.halfV;

	const Vector3 samples[] = {
		c - v,     c + v,     c - u,     c + u,     // 各壁の中央
		c - u - v, c + u - v, c - u + v, c + u + v, // 4隅
	};

	const float radiusSq = kVisibleRadius * kVisibleRadius;
	size_t worst = 0;
	for (const auto& viewer : samples) {
		size_t visible = 0;
		for (const auto& point : points_) {
			const Vector3 d = point - viewer;
			if (Dot(d, d) <= radiusSq) ++visible;
		}
		worst = (std::max)(worst, visible);
	}
	return worst;
}

void BattleAreaWallEffect::EmitNextPoints(const Vector3& viewerPos) {
	if (points_.empty()) return;

	// グループが登録されていないシーンでは何もしない
	auto& particleManager = ParticleManager::GetInstance();
	const auto& groups = particleManager.GetParticleGroups();
	auto groupIt = groups.find(kGroupName);
	if (groupIt == groups.end()) return;

	// 1回の発生で進める点数。遠くの点は飛ばすだけでカーソルは進めるので、
	// 近くの点は「kCycleDuration 秒に1回光る」という間隔が保たれる
	const size_t perTick = (std::max)(
		size_t{ 1 },
		static_cast<size_t>(std::ceil(points_.size() * kEmitInterval / kCycleDuration)));

	std::uniform_real_distribution<float> dist01(0.0f, 1.0f);
	size_t aliveCount = groupIt->second.particles.size();

	for (size_t i = 0; i < perTick; ++i) {
		const Vector3& point = points_[cursor_];
		cursor_ = (cursor_ + 1) % points_.size();

		// 描画上限を超えないための保険。溢れる前に発生を止める
		if (aliveCount >= kMaxAliveParticles) break;

		// 壁全体を常時光らせると画面がちらつくので、プレイヤーの近くだけに絞る。
		// エリアの箱は縦に長いため、高さ込みの距離で見てプレイヤーの周囲だけを光らせる
		const Vector3 d = point - viewerPos;
		const float distance = std::sqrt(Dot(d, d));
		if (distance > kVisibleRadius) continue;

		// 境界がくっきり切れないよう、外側ほど発生を間引いて薄れさせる
		if (distance > kFadeStartRadius) {
			const float density = (kVisibleRadius - distance) / (kVisibleRadius - kFadeStartRadius);
			if (dist01(rng_) > density) continue;
		}

		particleManager.Emit(kGroupName, point, 1);
		++aliveCount;
	}
}
