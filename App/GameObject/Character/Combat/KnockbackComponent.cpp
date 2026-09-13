#include "KnockbackComponent.h"
#include <algorithm>
#include <cmath>

namespace {
	// 速度がこれ以下になったらノックバックは終わったとみなす[m/s]
	constexpr float kRestSpeed = 0.05f;

	// KnockbackData::duration が 0 のときの既定値[秒]。
	// のけぞりは短く、吹き飛びは長く、打ち上げはその中間にしてある
	constexpr float kDefaultDuration[3] = {
		0.18f, // HitStun
		0.45f, // Knockback
		0.35f, // Launch
	};
}

float KnockbackComponent::DefaultDuration(ReactionType type) {
	const int32_t index = std::clamp(static_cast<int32_t>(type), 0, 2);
	return kDefaultDuration[index];
}

bool KnockbackComponent::HorizontalUnit(const Vector3& v, Vector3& out) {
	Vector3 horizontal{ v.x, 0.0f, v.z };
	const float length = Length(horizontal);
	if (length < 0.0001f) {
		out = {};
		return false;
	}
	out = horizontal * (1.0f / length);
	return true;
}

float KnockbackComponent::VerticalPower() const {
	// のけぞりは地上では浮かせない（仕様書 §5.1「少し後退するだけ」）。
	// 空中の相手に当てたときだけ効かせて、空中コンボで敵を落とさずに留められるようにする。
	// 攻撃データ側は種類を切り替えても UpwardRatio を付け直さなくてよい
	if (data_.type == ReactionType::HitStun && grounded_) return 0.0f;
	return data_.verticalPower;
}

float KnockbackComponent::Duration() const {
	return (data_.duration > 0.0f) ? data_.duration : DefaultDuration(data_.type);
}

float KnockbackComponent::Falloff() const {
	const float duration = Duration();
	if (duration <= 0.0f) return 0.0f;
	if (elapsed_ >= duration) return 0.0f;

	// 仕様書 §7。deceleration が 1.0 なら power * (1 - t) の直線。
	// 大きくすると初速だけ強く出てすぐ止まり、小さくすると長く滑る
	const float t = elapsed_ / duration;
	const float decel = (data_.deceleration > 0.0f) ? data_.deceleration : 1.0f;
	return std::pow(1.0f - t, decel);
}

void KnockbackComponent::ClampSpeed() {
	if (data_.maxSpeed > 0.0f && horizontalSpeed_ > data_.maxSpeed) {
		horizontalSpeed_ = data_.maxSpeed;
	}
}

void KnockbackComponent::Begin(const KnockbackData& data, const Vector3& direction, const Vector3& inheritVelocity) {
	data_ = data;
	elapsed_ = 0.0f;
	active_ = true;
	// 残っているのけぞりの方が長ければそちらを残す。
	// 弱い追撃で拘束が短くなると「割り込んで抜けられた」ように見えてしまう
	if (data_.stunTime > stunRemain_) {
		stunRemain_ = data_.stunTime;
	}

	// 向きが取れないとき（真上から潰したときなど）は水平には飛ばさない。
	// 打ち上げの verticalPower だけが効く
	if (!HorizontalUnit(direction, horizontalDir_)) {
		horizontalSpeed_ = 0.0f;
	} else {
		horizontalSpeed_ = data_.power;
	}

	float inheritUp = 0.0f;
	if (!data_.overrideVelocity) {
		// 元の速度を捨てない場合、新しい向きに沿う成分だけを初速へ足す。
		// （横向きの成分まで残すと、吹き飛ぶ向きが攻撃と食い違って見える）
		horizontalSpeed_ += Dot(Vector3{ inheritVelocity.x, 0.0f, inheritVelocity.z }, horizontalDir_);
		if (horizontalSpeed_ < 0.0f) horizontalSpeed_ = 0.0f;
		// 落下中の速度までは引き継がない（受けた瞬間に落下が加速して見えるため）
		inheritUp = (inheritVelocity.y > 0.0f) ? inheritVelocity.y : 0.0f;
	}
	ClampSpeed();

	velocity_ = horizontalDir_ * horizontalSpeed_;
	velocity_.y = VerticalPower() + inheritUp;

	// 浮かせる攻撃を受けたらその時点で接地は解除する。
	// ここで落としておかないと、最初の Update が「もう着地した」と判断して初速を消してしまう
	if (velocity_.y > 0.0f) {
		grounded_ = false;
	}
}

void KnockbackComponent::AddHit(const KnockbackData& data, const Vector3& direction) {
	if (!active_) {
		Begin(data, direction);
		return;
	}

	// 残っている上向きの勢い。どちらの方式でも引き継いで、
	// 空中コンボの途中で敵が急に落ちてしまわないようにする
	const float risingSpeed = (velocity_.y > 0.0f) ? velocity_.y : 0.0f;

	if (data.blend == KnockbackBlend::Override) {
		// 上書き方式（仕様書 §8）。強い攻撃を受けたときの反応が分かりやすい
		KnockbackData overridden = data;
		overridden.overrideVelocity = false; // 上向きの勢いだけ引き継ぐ
		Begin(overridden, direction, Vector3{ 0.0f, risingSpeed, 0.0f });
		return;
	}

	// ── 加算 + 上限方式（仕様書 §8）──
	// 多段ヒット攻撃で速度が際限なく伸びるのを防ぐ
	Vector3 newDir{};
	if (HorizontalUnit(direction, newDir)) {
		// 今の「減衰後」の速度に足してから、新しい向きと強さを取り直す
		const Vector3 current = horizontalDir_ * (horizontalSpeed_ * Falloff());
		const Vector3 blended = current + newDir * data.power;
		if (HorizontalUnit(blended, horizontalDir_)) {
			horizontalSpeed_ = Length(Vector3{ blended.x, 0.0f, blended.z });
		} else {
			horizontalSpeed_ = 0.0f;
		}
	}

	data_ = data;
	elapsed_ = 0.0f;
	if (data_.stunTime > stunRemain_) {
		stunRemain_ = data_.stunTime;
	}
	ClampSpeed();

	velocity_ = horizontalDir_ * horizontalSpeed_;
	velocity_.y = risingSpeed + VerticalPower();
	if (data_.maxSpeed > 0.0f && velocity_.y > data_.maxSpeed) {
		velocity_.y = data_.maxSpeed;
	}
	if (velocity_.y > 0.0f) {
		grounded_ = false;
	}
	active_ = true;
}

void KnockbackComponent::Update(float deltaTime, float gravity) {
	// のけぞりの時間は速度とは別枠。止まったあとも減らし続ける
	if (stunRemain_ > 0.0f) {
		stunRemain_ -= deltaTime;
		if (stunRemain_ < 0.0f) stunRemain_ = 0.0f;
	}

	if (!active_) return;

	elapsed_ += deltaTime;

	// ── 水平: 時間ベースの減衰（仕様書 §7）──
	// 毎フレーム係数を掛けるのではなく初速から作り直すので、フレームレートに依存しない
	const Vector3 horizontal = horizontalDir_ * (horizontalSpeed_ * Falloff());
	velocity_.x = horizontal.x;
	velocity_.z = horizontal.z;

	// ── 垂直: 重力 ──
	velocity_.y += gravity * deltaTime;
	if (grounded_ && velocity_.y < 0.0f) {
		// 接地中に下向きの速度を残すと、押し出しと落下を繰り返して上下にがくつく
		velocity_.y = 0.0f;
	}

	// ── 終了条件（仕様書 §7）──
	// 水平の減衰が終わり、かつ浮いていないなら終わり
	if (IsHorizontalFinished() && grounded_ && std::abs(velocity_.y) <= kRestSpeed) {
		Stop();
	}
}

void KnockbackComponent::Stop() {
	active_ = false;
	velocity_ = {};
	horizontalSpeed_ = 0.0f;
	// elapsed_ と stunRemain_ は戻さない。
	// elapsed_ を 0 にすると IsHorizontalFinished() が false へ逆戻りして、
	// 被弾リアクションのステートが復帰条件を満たせなくなる。
	// stunRemain_ は「速度は止まったがまだ動けない」を表すので、速度の停止とは別に減らし切る
}

void KnockbackComponent::OnLand(float keepRatio) {
	// 着地の減速。水平の初速自体を落とすので、このあとの減衰も弱いまま続く
	horizontalSpeed_ *= keepRatio;
	velocity_ = horizontalDir_ * (horizontalSpeed_ * Falloff());
	velocity_.y = 0.0f;
	grounded_ = true;
}

bool KnockbackComponent::CancelOutward(const Vector3& inwardNormal) {
	if (!active_) return false;

	const float outwardSpeed = Dot(velocity_, inwardNormal);
	if (outwardSpeed >= 0.0f) return false;

	velocity_ -= inwardNormal * outwardSpeed;
	// 初速のほうも削らないと、次のフレームに減衰カーブから同じ速度が復活する
	const float outwardInitial = Dot(horizontalDir_ * horizontalSpeed_, inwardNormal);
	if (outwardInitial < 0.0f) {
		Vector3 initial = horizontalDir_ * horizontalSpeed_;
		initial -= inwardNormal * outwardInitial;
		if (HorizontalUnit(initial, horizontalDir_)) {
			horizontalSpeed_ = Length(Vector3{ initial.x, 0.0f, initial.z });
		} else {
			horizontalSpeed_ = 0.0f;
		}
	}
	return true;
}
