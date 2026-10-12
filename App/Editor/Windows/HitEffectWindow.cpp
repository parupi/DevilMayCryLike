#include "AppEditorWindows.h"
#ifdef _DEBUG

#include "Editor/AppEditor.h"

#include <Debugger/GlobalVariables.h>
#include <Editor/Core/EditorHost.h>
#include <Graphics/Rendering/PostEffect/ChromaticAberrationEffect.h>
#include <Graphics/Rendering/PostEffect/HitFlashEffect.h>
#include <Graphics/Rendering/PostEffect/RadialBlurEffect.h>
#include <World3D/WorldTransform.h>

#include "GameObject/Character/Player/Player.h"
#include "GameObject/Effect/HitEffectSystem.h"
#include "GameObject/Effect/HitPostEffect.h"
#include "Input/GamepadRumble.h"
#include "Input/Input.h"

#include <imgui/imgui.h>

namespace {

// Test ボタンで再生するVFX。既定はゲーム中と同じ "HitImpact"。
// 敵に当てにいかなくても火花とリングの見た目を確認・調整できるようにしてある
char g_vfxName[64] = "HitImpact";

void DrawStrengthRow(const char* label, HitStopStrength strength, bool isArmorHit, Player* player)
{
	GlobalVariables& global = GlobalVariables::GetInstance();
	const char* group = HitEffectSystem::GetEditorGroupName();

	ImGui::PushID(label);
	ImGui::TextUnformatted(label);
	ImGui::DragFloat("ParticleScale",
		&global.GetValueRef<float>(group, HitEffectSystem::MakeEditorKey(strength, isArmorHit, "ParticleScale")),
		0.05f, 0.0f, 10.0f);
	ImGui::SliderFloat("ShakeTrauma",
		&global.GetValueRef<float>(group, HitEffectSystem::MakeEditorKey(strength, isArmorHit, "ShakeTrauma")),
		0.0f, 1.0f);

	if (ImGui::Button("Test")) {
		HitEffectRequest request;
		if (player) {
			request.position = player->GetWorldTransform()->GetTranslation();
			request.position.y += 1.0f;
		}
		request.direction = { 0.0f, 1.0f, 0.0f };
		request.strength = strength;
		request.hitStopTime = 0.05f;
		request.hitStopIntensity = 0.05f;
		request.isArmorHit = isArmorHit;
		request.vfxName = g_vfxName;
		HitEffectSystem::GetInstance().Play(request);
	}
	ImGui::Separator();
	ImGui::PopID();
}

} // namespace

void AppEditor::DrawHitEffectWindows()
{
	Player* player = FindPlayer();

	// --- HitEffect（もとは HitEffectSystem::DebugGui） ---
	if (EditorWindow::Begin("HitEffect", EditorWindow::Category::kVFX)) {
		HitEffectSystem& system = HitEffectSystem::GetInstance();
		if (!system.IsReady()) {
			ImGui::TextDisabled("HitEffectSystem がまだ初期化されていません");
		} else {
			ImGui::TextWrapped("攻撃の強さごとの演出量。ParticleScale はVFXの発生数倍率、"
				"ShakeTrauma はカメラに加える揺れ量です。");
			ImGui::InputText("VFX (Test用)", g_vfxName, sizeof(g_vfxName));
			ImGui::Separator();

			DrawStrengthRow("Light", HitStopStrength::Light, false, player);
			DrawStrengthRow("Medium", HitStopStrength::Medium, false, player);
			DrawStrengthRow("Heavy", HitStopStrength::Heavy, false, player);
			DrawStrengthRow("Armor (弾かれ)", HitStopStrength::Medium, true, player);

			// ── コントローラーの振動（全攻撃共通）。攻撃ごとの強さ・長さは Attack Editor の Rumble ──
			ImGui::SeparatorText("Rumble（コントローラーの振動・全攻撃共通）");
			{
				GlobalVariables& global = GlobalVariables::GetInstance();
				const char* group = HitEffectSystem::GetEditorGroupName();
				ImGui::SliderFloat("全体の倍率##RumbleGain", &global.GetValueRef<float>(group, "RumbleGain"), 0.0f, 2.0f);
				ImGui::SetItemTooltip("全攻撃の振動に掛ける倍率。OPTION の VIBRATION はさらにこの後に掛かる");
				ImGui::SliderFloat("終わりぎわに弱める割合##RumbleFadeRatio", &global.GetValueRef<float>(group, "RumbleFadeRatio"), 0.0f, 1.0f);
				ImGui::SetItemTooltip("0.4 なら長さの最後の4割で 0 まで下げる。0 だとぶつ切りで止まる");
				ImGui::TextUnformatted("弾かれたとき（スーパーアーマー）");
				ImGui::SliderFloat("低周波##ArmorRumbleLow", &global.GetValueRef<float>(group, "ArmorRumbleLow"), 0.0f, 1.0f);
				ImGui::SliderFloat("高周波##ArmorRumbleHigh", &global.GetValueRef<float>(group, "ArmorRumbleHigh"), 0.0f, 1.0f);
				ImGui::DragFloat("長さ##ArmorRumbleDuration", &global.GetValueRef<float>(group, "ArmorRumbleDuration"), 0.005f, 0.0f, 2.0f, "%.3f 秒");

				const GamepadRumble& rumble = GamepadRumble::GetInstance();
				ImGui::Text("パッド : %s   OPTION の倍率 : %.0f%%",
					Input::GetInstance().IsConnected() ? "接続中" : "未接続（振動しません）", rumble.GetMasterScale() * 100.0f);
				ImGui::ProgressBar((std::max)(rumble.GetCurrentLow(), 0.0f), ImVec2(-1.0f, 0.0f), "今の低周波");
				ImGui::ProgressBar((std::max)(rumble.GetCurrentHigh(), 0.0f), ImVec2(-1.0f, 0.0f), "今の高周波");
			}
			ImGui::Separator();

			ImGui::Text("Camera : %s", system.GetCamera() ? "connected" : "NOT CONNECTED");
			ImGui::Text("Player : %s", system.GetPlayer() ? "connected" : "NOT CONNECTED");

			if (ImGui::Button("Save##HitEffect")) {
				GlobalVariables::GetInstance().SaveFile("Effect", HitEffectSystem::GetEditorGroupName());
			}
		}
		EditorWindow::End();
	}

	// --- HitPostEffect（もとは HitPostEffect::Update） ---
	if (EditorWindow::Begin("HitPostEffect", EditorWindow::Category::kPostEffect)) {
		HitPostEffect* effect = player ? player->GetHitPostEffect() : nullptr;
		if (!effect) {
			ImGui::TextDisabled("このシーンに Player はいません");
		} else {
			ImGui::Text("Playing: %s", effect->IsPlaying() ? "true" : "false");
			if (auto* blur = effect->GetRadialBlur()) {
				ImGui::Text("Blur   : %.4f", blur->GetEffectData().strength);
			}
			if (auto* chroma = effect->GetChroma()) {
				ImGui::Text("Chroma : %.4f", chroma->GetEffectData().strength);
			}
			if (auto* flash = effect->GetFlash()) {
				ImGui::Text("Flash  : %.4f", flash->GetEffectData().flashColor.w);
			}
			if (ImGui::Button("Test (Heavy)")) {
				effect->Play(HitStopStrength::Heavy, Vector2{ 0.5f, 0.5f });
			}
		}
		EditorWindow::End();
	}
}

#endif // _DEBUG
