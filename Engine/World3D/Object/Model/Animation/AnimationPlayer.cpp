#include "AnimationPlayer.h"
#include "AnimationClipSet.h"
#include "Skeleton.h"
#include <Utility/Logger.h>
#include <algorithm>
#include <cmath>

void AnimationPlayer::Initialize(const AnimationClipSet* clips, Skeleton* skeleton)
{
	clips_ = clips;
	skeleton_ = skeleton;

	// 既定のクリップを入れておく。何も Play しなくてもバインドポーズで固まらない
	if (clips_ && !clips_->Empty()) {
		current_ = clips_->GetDefaultClip();
		currentName_ = clips_->GetDefaultClipName();
	}
	time_ = 0.0f;
	CacheRootMotionEndpoints();
}

void AnimationPlayer::Play(const std::string& name, bool loop, float blendTime, bool forceRestart)
{
	if (!clips_) return;

	const AnimationData* next = clips_->Find(name);
	if (!next) return;

	// 同じクリップの再生要求。forceRestart が無ければループ設定だけ更新して据え置く
	if (!forceRestart && next == current_ && currentName_ == name) {
		loop_ = loop;
		return;
	}

	// 遷移する瞬間のポーズを固定してブレンド元にする。
	// 前クリップを時刻で引き直すのではなく実際のポーズを使うので、
	// ブレンド中にさらに別のクリップへ移っても繋がりが途切れない
	if (blendTime > 0.0f && skeleton_ && skeleton_->GetJointCount() > 0) {
		skeleton_->CapturePose(blendSourcePose_);
		blending_ = true;
		blendTime_ = blendTime;
		blendTimer_ = 0.0f;
	} else {
		blending_ = false;
		blendTime_ = 0.0f;
		blendTimer_ = 0.0f;
	}

	current_ = next;
	currentName_ = name;
	loop_ = loop;
	time_ = 0.0f;

	// クリップが変わったので、ルートモーションの基準を取り直す。
	// 引き継ぐと切り替えた瞬間に前クリップとの差分ぶんワープする
	rootMotionPrimed_ = false;
	CacheRootMotionEndpoints();
}

void AnimationPlayer::Update(float deltaTime)
{
	if (!skeleton_) return;

	firedEvents_.clear();

	if (!current_) {
		// 再生するものが無いならバインドポーズのまま行列だけ更新する
		skeleton_->ResetToBindPose();
		skeleton_->Update();
		return;
	}

	const float previousTime = time_;
	const float scaledDelta = deltaTime * speed_;

	bool wrapped = false;
	time_ += scaledDelta;
	if (current_->duration > 0.0f) {
		if (loop_) {
			if (time_ >= current_->duration || time_ < 0.0f) {
				wrapped = true;
			}
			time_ = std::fmod(time_, current_->duration);
			if (time_ < 0.0f) time_ += current_->duration; // 逆再生（speed_ < 0）対策
		} else {
			time_ = std::clamp(time_, 0.0f, current_->duration);
		}
	} else {
		time_ = 0.0f;
	}

	// ブレンドの進行。負のスピードで巻き戻らないよう実時間で進める
	if (blending_) {
		blendTimer_ += deltaTime;
		if (blendTime_ <= 0.0f || blendTimer_ >= blendTime_) {
			blending_ = false;
		}
	}

	// --- ベースレイヤー ---
	skeleton_->ApplyClip(current_, time_);
	if (blending_) {
		skeleton_->BlendFromPose(blendSourcePose_, blendTimer_ / blendTime_);
	}

	// --- 上半身レイヤー（ベースの上に重ねる）---
	UpdateLayer(deltaTime);

	// --- ルートモーション（ポーズから抜き取る）---
	ExtractRootMotion(previousTime, wrapped);

	skeleton_->Update();

	UpdateEvents(previousTime, wrapped);
}

// ---------------------------------------------------------------- イベント

void AnimationPlayer::FireEventsInRange(float fromTime, float toTime)
{
	if (!current_) return;

	for (const auto& event : current_->events) {
		// (from, to] の半開区間で判定する。境界で二重発火しないようにするため
		if (event.time > fromTime && event.time <= toTime) {
			firedEvents_.push_back(event.tag);
			if (eventCallback_) eventCallback_(event.tag);
		}
	}
}

void AnimationPlayer::UpdateEvents(float previousTime, bool wrapped)
{
	if (!current_ || current_->events.empty()) return;

	if (wrapped) {
		// ループで巻き戻ったフレームは「前回位置〜終端」と「先頭〜今」の2区間を見る。
		// ここを片方だけにすると、ループの継ぎ目にあるイベントが毎周落ちる
		FireEventsInRange(previousTime, current_->duration);
		FireEventsInRange(-1.0f, time_);
	} else if (time_ >= previousTime) {
		FireEventsInRange(previousTime, time_);
	} else {
		// 逆再生。順序は気にせず通過したものを拾う
		FireEventsInRange(time_ - 0.0001f, previousTime);
	}
}

bool AnimationPlayer::WasEventFired(const std::string& tag) const
{
	return std::find(firedEvents_.begin(), firedEvents_.end(), tag) != firedEvents_.end();
}

// ---------------------------------------------------------------- レイヤー

void AnimationPlayer::PlayLayer(const std::string& clipName, const std::string& maskRootJoint, bool loop, float blendTime)
{
	if (!clips_ || !skeleton_) return;

	const AnimationData* clip = clips_->Find(clipName);
	if (!clip) return;

	// マスクが作れないとレイヤーは全身に効いてしまうので、その場合は張らない
	if (maskRootJoint != layerMaskRoot_ || layerMask_.empty()) {
		if (!skeleton_->BuildSubtreeMask(maskRootJoint, layerMask_)) {
			Logger::Log("[AnimationPlayer] レイヤーのマスク基点が見つかりません: " + maskRootJoint + "\n");
			layerMask_.clear();
			return;
		}
		layerMaskRoot_ = maskRootJoint;
	}

	layerClip_ = clip;
	layerName_ = clipName;
	layerLoop_ = loop;
	layerTime_ = 0.0f;
	layerStopping_ = false;
	layerTargetWeight_ = 1.0f;
	layerBlendTime_ = blendTime;
	if (blendTime <= 0.0f) layerWeight_ = 1.0f;
}

void AnimationPlayer::StopLayer(float blendTime)
{
	if (!layerClip_) return;

	layerStopping_ = true;
	layerBlendTime_ = blendTime;
	layerTargetWeight_ = 0.0f;
	if (blendTime <= 0.0f) {
		layerWeight_ = 0.0f;
		layerClip_ = nullptr;
		layerName_.clear();
	}
}

void AnimationPlayer::UpdateLayer(float deltaTime)
{
	if (!layerClip_) return;

	// ウェイトを目標値へ寄せる
	const float target = layerStopping_ ? 0.0f : std::clamp(layerTargetWeight_, 0.0f, 1.0f);
	if (layerBlendTime_ > 0.0f) {
		const float step = deltaTime / layerBlendTime_;
		if (layerWeight_ < target) layerWeight_ = (std::min)(target, layerWeight_ + step);
		else if (layerWeight_ > target) layerWeight_ = (std::max)(target, layerWeight_ - step);
	} else {
		layerWeight_ = target;
	}

	// レイヤーの時刻を進める。ベースとは別サイクルで回る
	layerTime_ += deltaTime * speed_;
	if (layerClip_->duration > 0.0f) {
		if (layerLoop_) {
			layerTime_ = std::fmod(layerTime_, layerClip_->duration);
			if (layerTime_ < 0.0f) layerTime_ += layerClip_->duration;
		} else {
			layerTime_ = std::clamp(layerTime_, 0.0f, layerClip_->duration);
			// ワンショットのレイヤーは終わったら自動で抜ける
			if (!layerStopping_ && layerTime_ >= layerClip_->duration) {
				StopLayer(layerBlendTime_);
			}
		}
	}

	skeleton_->ApplyClipMasked(layerClip_, layerTime_, layerMask_, layerWeight_);

	// 抜けきったら解放
	if (layerStopping_ && layerWeight_ <= 0.0f) {
		layerClip_ = nullptr;
		layerName_.clear();
		layerStopping_ = false;
	}
}

// ---------------------------------------------------------------- ルートモーション

void AnimationPlayer::SetRootMotionJoint(const std::string& jointName)
{
	rootMotionJoint_ = jointName;
	rootMotionAccum_ = { 0.0f, 0.0f, 0.0f };
	rootMotionPrimed_ = false;
	CacheRootMotionEndpoints();
}

void AnimationPlayer::CacheRootMotionEndpoints()
{
	rootTranslateAtStart_ = { 0.0f, 0.0f, 0.0f };
	rootTranslateAtEnd_ = { 0.0f, 0.0f, 0.0f };

	if (rootMotionJoint_.empty() || !current_) return;

	auto it = current_->nodeAnimations.find(rootMotionJoint_);
	if (it == current_->nodeAnimations.end()) return;

	const auto& keyframes = it->second.translate.keyframes;
	if (keyframes.empty()) return;

	rootTranslateAtStart_ = SampleCurve(keyframes, 0.0f);
	rootTranslateAtEnd_ = SampleCurve(keyframes, current_->duration);
}

void AnimationPlayer::ExtractRootMotion(float previousTime, bool wrapped)
{
	if (rootMotionJoint_.empty() || !skeleton_) return;

	Joint* joint = skeleton_->FindJoint(rootMotionJoint_);
	if (!joint) return;

	const Vector3 currentTranslate = joint->transform.translate;

	if (!rootMotionPrimed_) {
		// 再生開始直後は差分の基準が無いので、移動量は出さずに基準だけ作る
		previousRootTranslate_ = currentTranslate;
		rootMotionPrimed_ = true;
	} else if (wrapped) {
		// ループで巻き戻った分は「終端まで」＋「先頭から今まで」で繋ぐ。
		// 素直に引き算すると1周ぶん逆向きに飛ぶ
		rootMotionAccum_ += (rootTranslateAtEnd_ - previousRootTranslate_);
		rootMotionAccum_ += (currentTranslate - rootTranslateAtStart_);
		previousRootTranslate_ = currentTranslate;
	} else {
		rootMotionAccum_ += (currentTranslate - previousRootTranslate_);
		previousRootTranslate_ = currentTranslate;
	}
	(void)previousTime;

	// 抜き取った水平移動はポーズから消す。残すとキャラが二重に動く。
	// 縦方向はジャンプ等をコード側で持っている前提でそのまま残す
	joint->transform.translate.x = joint->bindTransform.translate.x;
	joint->transform.translate.z = joint->bindTransform.translate.z;
}

Vector3 AnimationPlayer::ConsumeRootMotion()
{
	const Vector3 delta = rootMotionAccum_;
	rootMotionAccum_ = { 0.0f, 0.0f, 0.0f };
	return delta;
}

// ---------------------------------------------------------------- 問い合わせ

bool AnimationPlayer::IsFinished() const
{
	if (!current_) return true;
	if (loop_) return false;
	return time_ >= current_->duration;
}

void AnimationPlayer::SetTime(float time)
{
	const float duration = GetDuration();
	time_ = (duration > 0.0f) ? std::clamp(time, 0.0f, duration) : 0.0f;
	// 飛ばした先を基準にし直す。そうしないと次のフレームで巨大なルートモーションが出る
	rootMotionPrimed_ = false;
}

float AnimationPlayer::GetDuration() const
{
	return current_ ? current_->duration : 0.0f;
}

float AnimationPlayer::GetNormalizedTime() const
{
	if (!current_ || current_->duration <= 0.0f) return 0.0f;
	return std::clamp(time_ / current_->duration, 0.0f, 1.0f);
}
