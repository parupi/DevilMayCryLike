#include "StageEnvironment.h"
#include "Debugger/GlobalVariables.h"
#include "Graphics/Rendering/Environment/RenderEnvironment.h"
#include "Graphics/Rendering/Sky/SkySystem.h"

namespace {

constexpr const char* kDirectory = "Environment";

// 今適用している雰囲気の GlobalVariables のグループ名。空なら既定の見た目
std::string g_group;

std::string MakeGroupName(const std::string& environmentName)
{
	return "Environment_" + environmentName;
}

// 洞窟の既定値。調整した値は Environment_Cave.json に保存され、そちらが優先される
// （AddItem は既にあるキーを上書きしない）
void AddCaveDefaults(GlobalVariables& gv, const std::string& group)
{
	// 背景は真っ黒ではなく、わずかに青緑を含んだ暗色（仕様書 2.1 の #080C10。画面に出る色で書く）
	gv.AddItem(group, "BackgroundColor", Vector3{ 8.0f / 255.0f, 12.0f / 255.0f, 16.0f / 255.0f });
	gv.AddItem(group, "SkyVisible", false);
	// 環境光は物の色に掛ける（色に関係なく足すと、暗い洞窟では全体が灰色に霞む）
	gv.AddItem(group, "AmbientColor", Vector3{ 0.06f, 0.075f, 0.095f });
	gv.AddItem(group, "AmbientAlbedoMix", 1.0f);
	// 真上からの平行光は屋外の太陽に見えるので、弱い寒色にする。床が読める程度は残す
	gv.AddItem(group, "DirectionalScale", Vector3{ 0.10f, 0.12f, 0.16f });
	// 壁の上ほど暗くする。床の上面は y≒-5、壁の上端は y≒7、張り出しの岩は y≒12 まで
	gv.AddItem(group, "HeightDarkEnabled", true);
	gv.AddItem(group, "DarkStartHeight", -1.0f);
	gv.AddItem(group, "DarkEndHeight", 9.0f);
	gv.AddItem(group, "DarkStrength", 0.9f);
}

void PushToRenderer(GlobalVariables& gv, const std::string& group)
{
	RenderEnvironment::Params params;
	params.backgroundColor = gv.GetValueRef<Vector3>(group, "BackgroundColor");
	params.ambientColor = gv.GetValueRef<Vector3>(group, "AmbientColor");
	params.ambientAlbedoMix = gv.GetValueRef<float>(group, "AmbientAlbedoMix");
	params.directionalScale = gv.GetValueRef<Vector3>(group, "DirectionalScale");
	params.heightDarkEnabled = gv.GetValueRef<bool>(group, "HeightDarkEnabled");
	params.darkStartHeight = gv.GetValueRef<float>(group, "DarkStartHeight");
	params.darkEndHeight = gv.GetValueRef<float>(group, "DarkEndHeight");
	params.darkStrength = gv.GetValueRef<float>(group, "DarkStrength");
	RenderEnvironment::GetInstance().SetParams(params);

	SkySystem::GetInstance().SetVisible(gv.GetValueRef<bool>(group, "SkyVisible"));
}

} // namespace

void StageEnvironment::Apply(const std::string& environmentName)
{
	Clear();
	if (environmentName.empty()) {
		return;
	}

	GlobalVariables& gv = GlobalVariables::GetInstance();
	g_group = MakeGroupName(environmentName);
	gv.CreateGroup(g_group);
	gv.LoadFile(kDirectory, g_group);

	// 既定値を持っているのは今のところ洞窟だけ。他の名前はファイルに全項目を書いておくこと
	if (environmentName == "Cave") {
		AddCaveDefaults(gv, g_group);
	}

	PushToRenderer(gv, g_group);
}

void StageEnvironment::Update()
{
	if (g_group.empty()) {
		return;
	}
	PushToRenderer(GlobalVariables::GetInstance(), g_group);
}

void StageEnvironment::Clear()
{
	g_group.clear();
	RenderEnvironment::GetInstance().ResetToDefault();
	SkySystem::GetInstance().SetVisible(true);
}
