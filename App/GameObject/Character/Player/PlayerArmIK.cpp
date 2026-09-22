#include "PlayerArmIK.h"
#include "World3D/Object/Renderer/BaseRenderer.h"
#include "World3D/Object/Model/Animation/SkinnedInstance.h"
#include "World3D/WorldTransform.h"
#include "Debugger/GlobalVariables.h"
#include <algorithm>
#ifdef _DEBUG
#include "World3D/Primitive/PrimitiveLineDrawer.h"
#endif

void PlayerArmIK::Initialize(BaseRenderer* modelRenderer) {
	renderer_ = modelRenderer;
	ready_ = false;
	weight_ = 0.0f;

	GlobalVariables& global = GlobalVariables::GetInstance();
	global.LoadFile(kDirectoryName, kGroupName);
	global.AddItem(kGroupName, "Enabled", enabled_);
	global.AddItem(kGroupName, "Strength", strength_);
	global.AddItem(kGroupName, "BlendSpeed", blendSpeed_);
	global.AddItem(kGroupName, "WristWeight", wristWeight_);
	global.AddItem(kGroupName, "Iterations", iterations_);
	global.AddItem(kGroupName, "MaxAngle", maxAngleDegrees_);
	global.AddItem(kGroupName, "DrawDebug", drawDebug_);
	global.AddItem(kGroupName, "Effector", effectorJoint_);
	global.AddItem(kGroupName, "ChainCount", static_cast<int32_t>(chainJoints_.size()));
	for (int32_t i = 0; i < kMaxChainJoints; ++i) {
		const std::string key = "Chain" + std::to_string(i);
		global.AddItem(kGroupName, key, (i < static_cast<int32_t>(chainJoints_.size())) ? chainJoints_[i] : std::string(""));
	}

	LoadParams();
}

void PlayerArmIK::LoadParams() {
	GlobalVariables& global = GlobalVariables::GetInstance();
	enabled_ = global.GetValueRef<bool>(kGroupName, "Enabled");
	strength_ = std::clamp(global.GetValueRef<float>(kGroupName, "Strength"), 0.0f, 1.0f);
	blendSpeed_ = global.GetValueRef<float>(kGroupName, "BlendSpeed");
	wristWeight_ = std::clamp(global.GetValueRef<float>(kGroupName, "WristWeight"), 0.0f, 1.0f);
	iterations_ = std::clamp(global.GetValueRef<int32_t>(kGroupName, "Iterations"), 1, 32);
	maxAngleDegrees_ = global.GetValueRef<float>(kGroupName, "MaxAngle");
	drawDebug_ = global.GetValueRef<bool>(kGroupName, "DrawDebug");
	effectorJoint_ = global.GetValueRef<std::string>(kGroupName, "Effector");

	const int32_t count = std::clamp(global.GetValueRef<int32_t>(kGroupName, "ChainCount"), 0, kMaxChainJoints);
	chainJoints_.clear();
	for (int32_t i = 0; i < count; ++i) {
		const std::string& name = global.GetValueRef<std::string>(kGroupName, "Chain" + std::to_string(i));
		// 空の枠は飛ばす。詰めて渡さないと IKSolver が名前解決に失敗してまるごと効かなくなる
		if (!name.empty()) chainJoints_.push_back(name);
	}
}

void PlayerArmIK::Update(bool wantIK, const Vector3& holdPositionLocal, const Quaternion& holdRotationLocal, float deltaTime) {
	if (!renderer_) return;

	SkinnedInstance* instance = renderer_->GetSkinnedInstance();
	if (!instance) return;

	// レンダラーの生成順によってはスキンインスタンスが後から出てくるので、取れてから繋ぐ
	if (!ready_) {
		space_.Initialize(renderer_, effectorJoint_);
		ready_ = true;
	}

	LoadParams();

	// 振りの出入りで滑らかに効かせる。いきなり 1 にすると構えの瞬間に腕が飛ぶ
	const float target = (enabled_ && wantIK) ? strength_ : 0.0f;
	const float rate = std::clamp(blendSpeed_ * deltaTime, 0.0f, 1.0f);
	weight_ += (target - weight_) * rate;

	IKRequest request;
	if (weight_ <= 0.001f) {
		weight_ = 0.0f;
		request.active = false;
		instance->SetIKRequest(request);
		return;
	}

	// プレイヤーのローカル空間 → スケルトン（モデル）空間。
	// モデルはレンダラー側で 0.38 倍・足元へずらして置かれているので、この変換は必須
	Matrix4x4 toModel{};
	if (!space_.GetModelSpaceMatrix(toModel)) {
		request.active = false;
		instance->SetIKRequest(request);
		return;
	}

	lastTargetLocal_ = holdPositionLocal;

	request.active = true;
	request.target = Transform(holdPositionLocal, toModel);
	request.chain.jointNames = chainJoints_;
	request.chain.effectorJoint = effectorJoint_;
	request.chain.iterations = iterations_;
	request.chain.weight = weight_;
	request.chain.maxAngleDegrees = maxAngleDegrees_;

	// 手首を剣の向きへ合わせる。
	// レンダラーに回転が入っていてもいいように、その逆回転を掛けてモデル空間へ移す
	const Quaternion rendererRotation = renderer_->GetWorldTransform()
		? renderer_->GetWorldTransform()->GetRotation() : Identity();
	request.alignJoint = effectorJoint_;
	request.alignRotation = Inverse(rendererRotation) * holdRotationLocal;
	request.alignWeight = wristWeight_ * weight_;

	instance->SetIKRequest(request);
	lastError_ = instance->GetIKError();
}

#ifdef _DEBUG

void PlayerArmIK::DrawEditor() {
	GlobalVariables& global = GlobalVariables::GetInstance();

	if (!renderer_ || !renderer_->GetSkinnedInstance()) {
		ImGui::TextColored(ImVec4(1.0f, 0.45f, 0.40f, 1.0f), "スキンモデルが見つかりません");
		return;
	}

	ImGui::Text("効き具合: %.2f   到達誤差: %.3f", weight_, lastError_);
	ImGui::TextDisabled("剣は制御点のまま。腕だけが柄の方を向きます");
	ImGui::TextDisabled("（剣は腕より遠くまで振られるので、手が柄に届くことはまずありません）");

	ImGui::Checkbox("腕IKを使う", &global.GetValueRef<bool>(kGroupName, "Enabled"));
	ImGui::SliderFloat("効き具合", &global.GetValueRef<float>(kGroupName, "Strength"), 0.0f, 1.0f);
	ImGui::SetItemTooltip("1 でIKの結果そのまま。下げるとアニメーションの腕に戻っていく");
	ImGui::SliderFloat("手首を合わせる", &global.GetValueRef<float>(kGroupName, "WristWeight"), 0.0f, 1.0f);
	ImGui::SetItemTooltip("手のひらを剣の向きへ合わせる強さ。0 で手首はアニメーションのまま");
	ImGui::DragFloat("出入りの速さ", &global.GetValueRef<float>(kGroupName, "BlendSpeed"), 0.5f, 1.0f, 60.0f);
	ImGui::SetItemTooltip("振り始め・振り終わりでIKが効き始める／抜ける速さ");
	ImGui::DragInt("反復回数", &global.GetValueRef<int32_t>(kGroupName, "Iterations"), 0.1f, 1, 32);
	ImGui::DragFloat("1ボーンの最大角(度)", &global.GetValueRef<float>(kGroupName, "MaxAngle"), 1.0f, 0.0f, 180.0f);
	ImGui::SetItemTooltip("元の姿勢からこれ以上は曲げない。小さくすると安全だが剣を向きにくくなる\n"
		"（0 にすると制限なし）");
	ImGui::Checkbox("チェーンと目標を描く", &global.GetValueRef<bool>(kGroupName, "DrawDebug"));

	ImGui::SeparatorText("チェーン（根 → 末端）");
	ImGui::TextDisabled("ジョイント名は Animation ウィンドウの「ジョイント」で調べられます");
	int32_t& chainCount = global.GetValueRef<int32_t>(kGroupName, "ChainCount");
	chainCount = std::clamp(chainCount, 0, kMaxChainJoints);

	// 名前はモデルが持っているものから選ばせる（手打ちだと綴り違いで黙って効かなくなる）
	std::vector<std::string> jointNames;
	for (const Joint& joint : renderer_->GetSkinnedInstance()->GetSkeleton()->GetSkeletonData().joints) {
		jointNames.push_back(joint.name);
	}

	auto jointCombo = [&jointNames](const char* label, std::string& value) {
		if (!ImGui::BeginCombo(label, value.empty() ? "(なし)" : value.c_str())) return;
		if (ImGui::Selectable("(なし)", value.empty())) value.clear();
		for (const std::string& name : jointNames) {
			if (ImGui::Selectable(name.c_str(), name == value)) value = name;
		}
		ImGui::EndCombo();
	};

	for (int32_t i = 0; i < chainCount; ++i) {
		ImGui::PushID(i);
		jointCombo("ボーン", global.GetValueRef<std::string>(kGroupName, "Chain" + std::to_string(i)));
		ImGui::PopID();
	}
	if (chainCount < kMaxChainJoints && ImGui::Button("ボーンを足す")) ++chainCount;
	if (chainCount > 0) {
		ImGui::SameLine();
		if (ImGui::Button("末尾を消す")) --chainCount;
	}
	jointCombo("手（目標へ寄せる）", global.GetValueRef<std::string>(kGroupName, "Effector"));

	if (ImGui::Button("Save##ArmIK")) {
		global.SaveFile(kDirectoryName, kGroupName);
	}
}

void PlayerArmIK::DrawDebug(const WorldTransform* playerTransform) const {
	if (!drawDebug_ || weight_ <= 0.001f || !playerTransform || !renderer_) return;

	SkinnedInstance* instance = renderer_->GetSkinnedInstance();
	if (!instance) return;

	// プレイヤーのローカル空間 → ワールド
	const Matrix4x4& playerWorld = const_cast<WorldTransform*>(playerTransform)->GetMatWorld();
	const Vector3 targetWorld = Transform(lastTargetLocal_, playerWorld);

	PrimitiveLineDrawer& lines = PrimitiveLineDrawer::GetInstance();
	lines.DrawWireSphere(targetWorld, 0.08f, { 1.0f, 0.85f, 0.2f, 1.0f }, 12);

	// 腕のチェーン。ボーンはモデル空間なので、レンダラーのローカル行列を通してからワールドへ
	WorldTransform* rendererTransform = renderer_->GetWorldTransform();
	if (!rendererTransform) return;
	const Matrix4x4 rendererLocal = MakeAffineMatrix(
		rendererTransform->GetScale(), rendererTransform->GetRotation(), rendererTransform->GetTranslation());
	const Matrix4x4 modelToWorld = rendererLocal * playerWorld;

	const Skeleton* skeleton = instance->GetSkeleton();
	std::vector<std::string> names = chainJoints_;
	names.push_back(effectorJoint_);
	for (size_t i = 0; i + 1 < names.size(); ++i) {
		const Joint* a = skeleton->FindJoint(names[i]);
		const Joint* b = skeleton->FindJoint(names[i + 1]);
		if (!a || !b) continue;
		const Vector3 pa = Transform(Vector3{ a->skeletonSpaceMatrix.m[3][0], a->skeletonSpaceMatrix.m[3][1], a->skeletonSpaceMatrix.m[3][2] }, modelToWorld);
		const Vector3 pb = Transform(Vector3{ b->skeletonSpaceMatrix.m[3][0], b->skeletonSpaceMatrix.m[3][1], b->skeletonSpaceMatrix.m[3][2] }, modelToWorld);
		lines.DrawLine(pa, pb, { 0.3f, 1.0f, 0.5f, 1.0f });
	}
}

#endif // _DEBUG
