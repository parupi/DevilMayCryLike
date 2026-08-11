#include "AppEditorWindows.h"
#ifdef _DEBUG

#include "Editor/AppEditor.h"

#include <Debugger/GlobalVariables.h>
#include <Editor/Core/EditorHost.h>

#include "GameObject/Camera/GameCamera.h"

#include <imgui/imgui.h>
#include <string>

namespace {

constexpr float kRadToDeg = 57.2957795f;

// パラメータは GlobalVariables のカメラ名グループに入っている。
// キーを直接叩けるので、GameCamera 側の内部変数を公開しなくてよい
GlobalVariables* Global() { return &GlobalVariables::GetInstance(); }

// 直前のウィジェットに説明のツールチップを付ける。
// キー名も併記しておくと Resource/GlobalVariables/Camera/GameCamera.json と対応が取れる
void ParamTooltip(const char* key, const char* help)
{
	ImGui::SetItemTooltip("%s\n\nキー: %s", help, key);
}

// 表示は日本語、ImGuiのIDは GlobalVariables のキーで固定する（"表示名##キー"）。
// こうしておけば表示名を書き換えてもウィジェットのIDは変わらない
void DragFloatParam(const std::string& group, const char* key, const char* label, const char* help,
	float speed, float min, float max)
{
	const std::string widgetLabel = std::string(label) + "##" + key;
	ImGui::DragFloat(widgetLabel.c_str(), &Global()->GetValueRef<float>(group, key), speed, min, max);
	ParamTooltip(key, help);
}

// ラジアンで持っているパラメータ。編集はラジアンのまま、右に度数を添える
void DragRadianParam(const std::string& group, const char* key, const char* label, const char* help,
	float speed, float min, float max)
{
	float& value = Global()->GetValueRef<float>(group, key);
	const std::string widgetLabel = std::string(label) + "##" + key;
	ImGui::DragFloat(widgetLabel.c_str(), &value, speed, min, max);
	ParamTooltip(key, help);
	ImGui::SameLine();
	ImGui::TextDisabled("= %.0f°", value * kRadToDeg);
}

void CheckboxParam(const std::string& group, const char* key, const char* label, const char* help)
{
	const std::string widgetLabel = std::string(label) + "##" + key;
	ImGui::Checkbox(widgetLabel.c_str(), &Global()->GetValueRef<bool>(group, key));
	ParamTooltip(key, help);
}

// セクションの頭に置く、その機能そのものの説明
void SectionNote(const char* text)
{
	ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled));
	ImGui::TextWrapped("%s", text);
	ImGui::PopStyleColor();
	ImGui::Spacing();
}

} // namespace

void AppEditor::DrawCameraWorkWindow()
{
	if (!EditorWindow::Begin("GameCamera", EditorWindow::Category::kCamera)) {
		return;
	}

	GameCamera* camera = FindGameCamera();
	if (!camera) {
		ImGui::TextDisabled("このシーンに GameCamera はいません");
		EditorWindow::End();
		return;
	}

	const std::string& group = camera->name_;
	const GameCamera::EditorStatus status = camera->MakeEditorStatus();

	ImGui::Text("モード: %s    状態: %s (%.2f)",
		status.mode == GameCamera::Mode::LockOn ? "ロックオン" : "フリー",
		status.state == GameCamera::State::Battle ? "戦闘" : "探索",
		status.battleBlend);
	ImGui::SetItemTooltip(
		"モード … ロックオン中かどうか。フリーは右スティックで自由に回せる状態。\n"
		"状態  … 敵が近くに見えていると「戦闘」になる。括弧内は切り替わり具合(0〜1)。");
	ImGui::TextDisabled("各項目にカーソルを合わせると説明が出ます");
	ImGui::Separator();

	if (ImGui::CollapsingHeader("基本の追従（距離・高さ・スティック）", ImGuiTreeNodeFlags_DefaultOpen)) {
		SectionNote("プレイヤーの周りをぐるっと回る、いちばん土台の部分。まずここで画の「引き具合」と「見下ろし具合」を決める。");
		DragFloatParam(group, "Distance", "追従距離（基準）",
			"プレイヤーからカメラまでの基準の距離。\n"
			"実際の距離はこの値へ滑らかに寄っていく（攻撃中や戦闘中は下の倍率が掛かる）。\n"
			"大きい = 引きの画で周りが見える / 小さい = 寄りの画で迫力が出る",
			0.1f, 1.0f, 100.0f);
		DragFloatParam(group, "DistanceLerpSpeed", "距離の追従速度",
			"実際の距離が基準距離へ寄っていく速さ。\n"
			"大きい = すぐ目標距離になる / 小さい = ゆっくりズームしていく",
			0.05f, 0.1f, 30.0f);
		DragFloatParam(group, "BaseHeight", "カメラの高さ（基準）",
			"追従位置をプレイヤーの何m上に置くか。\n"
			"大きいほど見下ろしの画になる。",
			0.1f, 0.0f, 50.0f);
		DragFloatParam(group, "MinDistance", "最接近距離",
			"計算結果がどうであれ、プレイヤーにこれ以上は近づかない下限。\n"
			"カメラがプレイヤーにめり込むのを防ぐ保険。",
			0.1f, 0.0f, 50.0f);
		DragFloatParam(group, "SensitivityX", "右スティック感度（横）",
			"右スティックの横入力1フレームあたりの回転量(rad)。\n"
			"タイトルのOPTIONで設定した感度がさらに掛け算される。",
			0.001f, 0.0f, 1.0f);
		DragFloatParam(group, "SensitivityY", "右スティック感度（縦）",
			"右スティックの縦入力1フレームあたりの回転量(rad)。\n"
			"OPTIONの感度と上下反転の設定がここに掛かる。",
			0.001f, 0.0f, 1.0f);
		DragRadianParam(group, "PitchLimit", "見上げ／見下ろしの限界",
			"カメラの縦角度(pitch)の上限・下限(rad)。上下とも同じ角度で止まる。\n"
			"大きくすると真上・真下近くまで向けるようになる。",
			0.01f, 0.0f, 1.5f);
	}

	if (ImGui::CollapsingHeader("自動で背後に戻る（Auto Rotation）")) {
		SectionNote("右スティックから手を離したまま走っていると、カメラがプレイヤーの背中側へ自動で回り込む機能。3条件（有効・無入力の時間・移動中）が全部そろったときだけ働く。");
		CheckboxParam(group, "AutoRotateEnabled", "自動で背後に戻す",
			"オフにすると、右スティックで動かさない限りカメラの向きは変わらなくなる。");
		DragFloatParam(group, "AutoRotateDelay", "戻り始めるまでの待ち時間（秒）",
			"右スティックを離してから何秒後に回り始めるか。\n"
			"短い = すぐ背後に付く（お節介に感じやすい） / 長い = プレイヤーが向けた角度が残る",
			0.05f, 0.0f, 5.0f);
		DragFloatParam(group, "AutoRotateSpeed", "背後へ戻る速さ",
			"背中側へ回り込む補間の速さ。\n"
			"大きすぎるとカメラが勝手にぐるんと回って酔いやすい。",
			0.05f, 0.1f, 20.0f);
		DragFloatParam(group, "AutoRotateMoveSpeed", "働き始める移動速度",
			"プレイヤーの水平速度がこの値以上のときだけ回り込む。\n"
			"立ち止まって周りを見ているときに勝手に回らないための条件。",
			0.05f, 0.0f, 20.0f);
	}

	if (ImGui::CollapsingHeader("遅れ追従と注視点（Lag / LookAt）", ImGuiTreeNodeFlags_DefaultOpen)) {
		SectionNote("「カメラの位置」と「どこを見るか」をどれだけ遅らせるか。カメラの気持ちよさ・酔いにくさはほぼここで決まる。");
		DragFloatParam(group, "PositionLagSpeed", "カメラ位置の追従速度",
			"計算した理想位置へ、実際のカメラが寄っていく速さ。\n"
			"大きい = ぴったり張り付いてキビキビ / 小さい = ふわっと遅れて付いてくる",
			0.05f, 0.1f, 30.0f);
		DragFloatParam(group, "LookLagSpeed", "前方注視ずらしの追従速度",
			"下の「前方への注視ずらし」が効き始める／戻るときの速さ。\n"
			"小さいほど、向きを変えても注視点がゆっくり付いてくる。",
			0.05f, 0.1f, 30.0f);
		DragFloatParam(group, "LookForwardOffset", "前方への注視ずらし",
			"注視点をプレイヤーが向いている方向へずらす量。\n"
			"大きいほどプレイヤーが画面の手前寄りに映り、進行方向が広く見える。0で常に中央。",
			0.05f, 0.0f, 20.0f);
		DragFloatParam(group, "LookHeight", "注視点の高さ",
			"足元ではなく胸〜頭あたりを見るための高さ。\n"
			"壁との当たり判定の起点（ピボット）にもこの高さが使われる。",
			0.05f, 0.0f, 20.0f);
		DragFloatParam(group, "LookPitchDip", "縦角度による注視点の上下",
			"カメラを上げて見下ろすほど注視点を下げ、下げて見上げるほど注視点を上げる量。\n"
			"上下に振ってもプレイヤーが画面の中に残りやすくなる。0で補正なし。",
			0.05f, 0.0f, 20.0f);
		DragFloatParam(group, "LookTargetLagSpeed", "注視点そのものの追従速度",
			"注視点が急に切り替わったとき（フリー↔ロックオンの切り替えなど）の視線の飛びを吸収する。\n"
			"小さいほどゆっくり視線が移る。",
			0.05f, 0.1f, 30.0f);
	}

	if (ImGui::CollapsingHeader("壁の回り込み（Collision）")) {
		SectionNote("プレイヤーとカメラの間に壁や床（Groundカテゴリのコライダー）があると、カメラを遮蔽物の手前へ引き寄せる。");
		DragFloatParam(group, "CollisionMargin", "壁からの余裕",
			"遮蔽物のどれだけ手前でカメラを止めるか。\n"
			"小さすぎると壁が手前にめり込んで見える。",
			0.01f, 0.0f, 5.0f);
		DragFloatParam(group, "CollisionMinDist", "引き寄せ時の最小距離",
			"壁に押し込まれても、プレイヤーからこの距離は必ず空ける。\n"
			"狭い場所でカメラがプレイヤーの中に入ってしまうのを防ぐ。",
			0.01f, 0.0f, 10.0f);
	}

	if (ImGui::CollapsingHeader("ロックオン中のカメラ")) {
		SectionNote("ロックオン中は上の追従設定を使わず、プレイヤーと敵を結ぶ線の後ろにカメラを置く専用の計算になる。");
		DragFloatParam(group, "LockOnDistanceMul", "敵との距離に対する倍率",
			"（プレイヤーと敵の距離）× この値 がカメラ距離になる。\n"
			"敵が遠いほど自動で引く。下の上限・下限でクランプされる。",
			0.01f, 0.0f, 5.0f);
		DragFloatParam(group, "LockOnDistanceMin", "距離の下限",
			"敵に密着していても、これより近くには寄らない。",
			0.1f, 0.0f, 100.0f);
		DragFloatParam(group, "LockOnDistanceMax", "距離の上限",
			"敵が遠くても、これより引かない。",
			0.1f, 0.0f, 100.0f);
		DragFloatParam(group, "LockOnHeight", "カメラの高さ",
			"ロックオン中のカメラの高さ。基本の追従とは別の値。\n"
			"大きいほど2人を見下ろす画になる。",
			0.1f, 0.0f, 50.0f);
		DragFloatParam(group, "LockOnRightOffset", "横へのずらし量",
			"正で右、負で左へずらす。プレイヤーと敵が左右に並ぶ肩越しの構図になる。\n"
			"0にすると敵の真後ろからの画になり、プレイヤーが敵を隠しやすい。",
			0.1f, -20.0f, 20.0f);
		DragFloatParam(group, "LockOnLagSpeed", "カメラの追従速度",
			"ロックオン位置へ寄っていく速さ。\n"
			"大きい = キビキビ / 小さい = 敵が動いたときにゆったり追う",
			0.05f, 0.1f, 30.0f);
	}

	if (ImGui::CollapsingHeader("画角と攻撃ズーム（FOV / Action）")) {
		SectionNote("速く走るほど画角を広げて疾走感を出し、攻撃中はカメラを寄せて迫力を出す。画角は縦方向・ラジアンで持っている。");
		DragRadianParam(group, "FovNormal", "通常時の画角",
			"止まっている〜低速のときの画角（縦・rad）。\n"
			"小さい = 望遠でスピード感が減る / 大きい = 広角で歪みとスピード感が出る",
			0.005f, 0.1f, 2.0f);
		DragRadianParam(group, "FovDash", "高速移動時の画角",
			"下の「最大になる速度」に達したときの画角（縦・rad）。\n"
			"通常時より大きくすると走ったときに視界が広がって疾走感が出る。",
			0.005f, 0.1f, 2.0f);
		DragFloatParam(group, "FovSpeedMin", "画角が広がり始める速度",
			"プレイヤーの水平速度がこの値を超えると画角が広がり始める。\n"
			"歩き速度より少し上に置くと、走ったときだけ効く。",
			0.1f, 0.0f, 50.0f);
		DragFloatParam(group, "FovSpeedMax", "画角が最大になる速度",
			"この水平速度で画角が「高速移動時の画角」に到達する。\n"
			"広がり始める速度に近づけるほど、変化が急になる。",
			0.1f, 0.0f, 50.0f);
		DragFloatParam(group, "FovLerpSpeed", "画角の変化速度",
			"画角が目標値へ寄っていく速さ。\n"
			"大きすぎると加減速のたびにガクガク画角が動く。",
			0.05f, 0.1f, 30.0f);
		DragFloatParam(group, "AttackDistanceScale", "攻撃中の距離倍率",
			"プレイヤーが攻撃中の間、カメラ距離に掛ける倍率。\n"
			"1未満で寄る（0.85なら15%寄る）。1で無効。",
			0.01f, 0.3f, 1.0f);
		DragFloatParam(group, "ActionZoomSpeed", "攻撃ズームの速さ",
			"攻撃中の寄り／戻りの補間速度。\n"
			"大きいと攻撃の瞬間にガクッと寄る。",
			0.05f, 0.1f, 30.0f);
		ImGui::Separator();
		ImGui::Text("現在のズーム倍率: %.2f   画角: %.3f rad (%.0f°)",
			status.actionZoomScale, camera->GetFovY(), camera->GetFovY() * kRadToDeg);
	}

	if (ImGui::CollapsingHeader("敵を画面に入れる補正（Enemy Framing）")) {
		SectionNote("フリー中に右スティックを触っていないとき、画面の端に寄った敵を画面中央側へ入れるようカメラを少しだけ回す。プレイヤーが操作している間は働かない。");
		CheckboxParam(group, "EnemyFramingEnabled", "敵を画面に入れる補正を使う",
			"オフにすると、敵が画面外へ出てもカメラは何もしない。");
		DragFloatParam(group, "EnemyFramingEdge", "補正が始まる画面上の位置",
			"画面中央を0、左右の端を1としたとき、敵がこの値より外側に居ると補正が始まる。\n"
			"小さい = 中央寄りでもすぐ働く（お節介） / 大きい = 端に行くまで働かない",
			0.01f, 0.0f, 1.0f);
		DragFloatParam(group, "EnemyFramingYawSpeed", "補正の最大回転速度",
			"画面のいちばん外側に敵が居るときの回転速度(rad/秒)。端に近いほど強く効く。\n"
			"大きすぎるとカメラを勝手に持っていかれる感じになる。",
			0.02f, 0.0f, 5.0f);
	}

	if (ImGui::CollapsingHeader("カメラ状態（探索 / 戦闘）")) {
		SectionNote("ロックオン中、または敵が画面に見えているあいだは「戦闘」状態になり、距離と画角がまとめて変わる。切り替えは一気にではなくブレンドされる。");
		CheckboxParam(group, "BattleStateEnabled", "戦闘状態の切り替えを使う",
			"オフにすると常に「探索」扱いになり、下の2つは効かなくなる。");
		DragFloatParam(group, "BattleDistanceScale", "戦闘時の距離倍率",
			"戦闘中に追従距離へ掛ける倍率。\n"
			"1未満で寄る / 1より大きいと引く（敵の動きを見せたいなら引くのもあり）",
			0.01f, 0.3f, 1.5f);
		DragFloatParam(group, "BattleFovAdd", "戦闘時に足す画角",
			"戦闘中に画角へ加算する値(rad)。プラスで広角になり、周りの敵が見えやすくなる。",
			0.005f, -0.5f, 0.5f);
		DragFloatParam(group, "BattleBlendSpeed", "状態の切り替わり速度",
			"探索↔戦闘の変化にかかる速さ。\n"
			"小さいとゆっくり切り替わり、敵の出入りでカメラがバタつきにくい。",
			0.05f, 0.1f, 20.0f);
	}

	if (ImGui::CollapsingHeader("カメラシェイク")) {
		SectionNote("「トラウマ」という0〜1の値を溜め、それが減っていくあいだカメラを揺らす方式。揺れ幅はトラウマの2乗に比例するので、残り少ないときの小さな揺れは自然に消える。");
		DragFloatParam(group, "ShakeMaxOffset", "最大の揺れ幅",
			"トラウマが1.0のときの揺れ幅(m)。揺れの大きさの上限。",
			0.05f, 0.0f, 10.0f);
		DragFloatParam(group, "ShakeDecayRate", "揺れの収まる速さ",
			"1秒あたりに減るトラウマの量。\n"
			"大きい = 短くビシッと収まる / 小さい = 長く揺れ続ける",
			0.05f, 0.1f, 20.0f);
		DragFloatParam(group, "ShakeHitTrauma", "被弾時の揺れ量",
			"プレイヤーのHPが減ったフレームに加えるトラウマ。",
			0.01f, 0.0f, 1.0f);
		DragFloatParam(group, "ShakeLandTrauma", "着地時の揺れ量",
			"高いところから着地したときに加えるトラウマ。",
			0.01f, 0.0f, 1.0f);
		DragFloatParam(group, "ShakeLandSpeedThreshold", "揺れる落下速度のしきい値",
			"この落下速度以上で着地したときだけ揺らす。\n"
			"小さくすると軽いジャンプでも揺れる。",
			0.1f, 0.0f, 50.0f);
		ImGui::Separator();
		ImGui::Text("現在のトラウマ: %.2f", status.shakeTrauma);
		if (ImGui::Button("揺らしてみる")) {
			camera->AddShake(0.6f);
		}
	}

	ImGui::Separator();
	if (ImGui::Button("保存##Camera")) {
		Global()->SaveFile("Camera", group);
	}
	ImGui::SetItemTooltip("Resource/GlobalVariables/Camera/%s.json に書き出す。\n押さないと次回起動時に元へ戻る。", group.c_str());

	EditorWindow::End();
}

#endif // _DEBUG
