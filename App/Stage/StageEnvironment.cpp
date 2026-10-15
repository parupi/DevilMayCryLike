#include "StageEnvironment.h"
#include "Debugger/GlobalVariables.h"
#include "Graphics/Rendering/Environment/RenderEnvironment.h"
#include "Graphics/Rendering/Sky/SkySystem.h"
#include "Graphics/Rendering/PostEffect/OffScreenManager.h"
#include "Graphics/Rendering/PostEffect/ColorGradingEffect.h"
#include "Graphics/Rendering/PostEffect/BloomEffect.h"
#include <memory>
#include <string>

namespace {

constexpr const char* kDirectory = "Environment";

// 今適用している雰囲気の GlobalVariables のグループ名。空なら既定の見た目
std::string g_group;

// 霧の層のうち上部の霧（FogLayer1〜3）の数。残りの1枚は地面の霧（GroundMist_*）
constexpr int kUpperFogLayerCount = RenderEnvironment::Params::kGroundMistLayer;

// 戦闘状態への寄り具合（GameCamera の battleBlend）。地面の霧を戦闘中に薄くするのに使う
float g_battleBlend = 0.0f;

// 雰囲気ごとの色調整。ボスのブレス用（BossBreathColorGrading）とは別のパスで、常に掛けておく。
// パスはシーンをまたいで生き続ける（OffScreenManager が持つ）ので、最初の1回だけ作る
constexpr const char* kGradingEffectName = "EnvironmentColorGrading";

// ブルームは全シーン共通（MyGameTitle が常時有効で登録）なので、雰囲気の間だけ値を差し替え、
// 抜けるときに元へ戻す。元の値は最初に差し替える直前に覚える
constexpr const char* kBloomEffectName = "Bloom";
bool g_bloomOverridden = false;
bool g_bloomWasActive = true;
BloomEffect::BloomSettings g_bloomOriginal{};

float g_torchFlickerScale = 1.0f;
float g_particleSpawnRate = 1.0f;
StageEnvironment::DustSettings g_dust{};
StageEnvironment::DripSettings g_drip{};

BloomEffect* FindBloom()
{
	return dynamic_cast<BloomEffect*>(OffScreenManager::GetInstance().FindEffect(kBloomEffectName));
}

void RestoreBloom()
{
	if (!g_bloomOverridden) return;
	if (BloomEffect* bloom = FindBloom()) {
		bloom->GetSettings() = g_bloomOriginal;
		bloom->SetActive(g_bloomWasActive);
	}
	g_bloomOverridden = false;
}

ColorGradingEffect* EnsureGradingEffect()
{
	auto& offScreen = OffScreenManager::GetInstance();
	if (auto* existing = dynamic_cast<ColorGradingEffect*>(offScreen.FindEffect(kGradingEffectName))) {
		return existing;
	}
	auto grading = std::make_unique<ColorGradingEffect>(kGradingEffectName);
	grading->SetActive(false);
	offScreen.AddEffect(std::move(grading));
	return dynamic_cast<ColorGradingEffect*>(offScreen.FindEffect(kGradingEffectName));
}

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

	// 高さフォグ。壁の上端（y≒7）あたりから急に濃くなり、張り出しの岩の上は霧に消える。
	// 色は背景色と同じにする。少しでも違うと、霧に沈んだ岩と背景の境目が線になって残る
	gv.AddItem(group, "HeightFogEnabled", true);
	gv.AddItem(group, "HeightFogColor", Vector3{ 8.0f / 255.0f, 12.0f / 255.0f, 16.0f / 255.0f });
	gv.AddItem(group, "HeightFogBaseHeight", 3.0f);
	gv.AddItem(group, "HeightFogDensity", 0.02f);
	gv.AddItem(group, "HeightFogFalloff", 0.25f);
	gv.AddItem(group, "HeightFogMaxOpacity", 1.0f);

	// 上部の霧の層（仕様書 2.2 の FogLayer_01〜03）。低い層ほど薄く、高い層ほど濃く暗い
	struct LayerDefault { float height; Vector3 color; float opacity; float noiseScale; Vector3 speed; };
	const LayerDefault layers[kUpperFogLayerCount] = {
		{ 4.0f, { 36.0f / 255.0f, 45.0f / 255.0f, 56.0f / 255.0f }, 0.30f, 0.060f, {  0.25f, 0.0f,  0.10f } },
		{ 6.5f, { 25.0f / 255.0f, 32.0f / 255.0f, 41.0f / 255.0f }, 0.50f, 0.045f, { -0.18f, 0.0f,  0.12f } },
		{ 9.0f, { 15.0f / 255.0f, 20.0f / 255.0f, 27.0f / 255.0f }, 0.70f, 0.035f, {  0.12f, 0.0f, -0.20f } },
	};
	for (int i = 0; i < kUpperFogLayerCount; ++i) {
		const std::string prefix = "FogLayer" + std::to_string(i + 1) + "_";
		gv.AddItem(group, prefix + "Enabled", true);
		gv.AddItem(group, prefix + "Height", layers[i].height);
		gv.AddItem(group, prefix + "Color", layers[i].color);
		gv.AddItem(group, prefix + "Opacity", layers[i].opacity);
		gv.AddItem(group, prefix + "NoiseScale", layers[i].noiseScale);
		gv.AddItem(group, prefix + "NoiseSpeed", layers[i].speed);
	}
	gv.AddItem(group, "FogLayerHalfSize", 30.0f);
	gv.AddItem(group, "FogLayerEdgeFade", 0.3f);
	// 下の2層は壁の上端より低く壁を横切るので、短いと板が壁に刺さる線が見える
	gv.AddItem(group, "FogLayerSoftDistance", 8.0f);

	// 地面を這う霧（仕様書 4.2）。床の上面(y≒-5.05)のすぐ上に置き、細かい模様でゆっくり流す。
	// 戦闘中は BattleOpacityScale まで薄くして、足元の攻撃予兆を見えやすくする
	gv.AddItem(group, "GroundMist_Enabled", true);
	gv.AddItem(group, "GroundMist_Height", -4.55f);
	gv.AddItem(group, "GroundMist_Color", Vector3{ 95.0f / 255.0f, 108.0f / 255.0f, 124.0f / 255.0f });
	gv.AddItem(group, "GroundMist_Opacity", 0.45f);
	gv.AddItem(group, "GroundMist_NoiseScale", 0.11f);
	gv.AddItem(group, "GroundMist_NoiseSpeed", Vector3{ 0.18f, 0.0f, 0.06f });
	gv.AddItem(group, "GroundMist_SoftDistance", 0.6f);
	gv.AddItem(group, "GroundMist_BattleOpacityScale", 0.45f);

	// キャラの見やすさ（仕様書 9.2）。平行光を弱めた洞窟では、縦の面が多いキャラはほぼ光を受けず黒い影になる
	gv.AddItem(group, "CharacterAmbient", Vector3{ 0.22f, 0.24f, 0.28f });
	gv.AddItem(group, "RimColor", Vector3{ 0.06f, 0.08f, 0.11f });
	gv.AddItem(group, "RimPower", 3.0f);

	// 距離フォグ（仕様書 3.1）。遠くの壁を暗がりに沈める。強すぎると敵が見えなくなるので上限を置く
	gv.AddItem(group, "DistanceFogEnabled", true);
	gv.AddItem(group, "DistanceFogColor", Vector3{ 8.0f / 255.0f, 12.0f / 255.0f, 16.0f / 255.0f });
	gv.AddItem(group, "DistanceFogDensity", 0.015f);
	gv.AddItem(group, "DistanceFogStart", 8.0f);
	gv.AddItem(group, "DistanceFogMaxOpacity", 0.7f);

	// カラーグレーディング（仕様書 3.3）。やや寒色・彩度やや低め・コントラスト中程度、
	// 暗部は青緑、ハイライトは松明の暖色を残す
	gv.AddItem(group, "GradingEnabled", true);
	gv.AddItem(group, "GradingStrength", 1.0f);
	gv.AddItem(group, "GradingGain", Vector3{ 0.97f, 1.0f, 1.04f });
	gv.AddItem(group, "GradingLift", Vector3{ 0.0f, 0.0f, 0.0f });
	// 彩度は下げすぎるとプレイヤーの赤や攻撃予兆の色が沈むので控えめに
	gv.AddItem(group, "GradingSaturation", 0.92f);
	gv.AddItem(group, "GradingContrast", 1.06f);
	// コントラストの中心。洞窟の絵はほとんどが暗いので、0.5 のままだと暗部（とキャラ）が真っ黒に潰れる
	gv.AddItem(group, "GradingContrastPivot", 0.2f);
	gv.AddItem(group, "GradingSplitBalance", 0.45f);
	gv.AddItem(group, "GradingShadowTint", Vector3{ 0.0f, 0.02f, 0.03f });
	gv.AddItem(group, "GradingShadowTintStrength", 1.0f);
	gv.AddItem(group, "GradingHighlightTint", Vector3{ 1.08f, 1.0f, 0.86f });
	gv.AddItem(group, "GradingHighlightTintStrength", 0.6f);

	// 露出とトーンマップ（仕様書 3.2 の手順5）。暗部は素通しにして、松明の周りや炎だけを収める
	gv.AddItem(group, "ToneMapEnabled", true);
	gv.AddItem(group, "Exposure", 1.0f);
	gv.AddItem(group, "ToneMapShoulderStart", 0.6f);

	// ブルーム（仕様書 3.2）。HDR のまま光を抜き出すので、しきい値 1 以上＝炎や強い光だけが光る。
	// 控えめにして、戦闘エフェクトが白飛びしないようにする
	gv.AddItem(group, "BloomEnabled", true);
	gv.AddItem(group, "BloomThreshold", 1.0f);
	gv.AddItem(group, "BloomSoftKnee", 0.3f);
	gv.AddItem(group, "BloomBlurRadius", 1.8f);
	gv.AddItem(group, "BloomIntensity", 0.6f);

	// 松明の揺らぎの強さ（WallTorch の揺れ幅に掛ける倍率）
	gv.AddItem(group, "TorchFlickerStrength", 1.0f);

	// SSAO（仕様書 3.4）。岩と壁の隙間・床と壁の接地に陰を足す。暗い洞窟で強くすると黒く潰れるので、
	// 環境光にはしっかり、直接光（松明）には弱く掛ける
	gv.AddItem(group, "SsaoEnabled", true);
	gv.AddItem(group, "SsaoRadius", 0.9f);
	gv.AddItem(group, "SsaoBias", 0.05f);
	gv.AddItem(group, "SsaoIntensity", 1.2f);
	gv.AddItem(group, "SsaoPower", 1.0f);
	gv.AddItem(group, "SsaoAmbientStrength", 1.0f);
	gv.AddItem(group, "SsaoDirectStrength", 0.35f);

	// ボリューメトリックライト（仕様書 7章）。松明の光が空気中で散って、まわりがほのかに霞む
	gv.AddItem(group, "VolumetricEnabled", true);
	gv.AddItem(group, "VolumetricStrength", 0.012f);
	gv.AddItem(group, "VolumetricMaxDistance", 60.0f);

	// 環境パーティクル（仕様書 4章・8章）。ParticleSpawnRate は埃・松明の煙と明かりの埃に掛ける倍率
	gv.AddItem(group, "ParticleSpawnRate", 1.0f);
	gv.AddItem(group, "DustEnabled", true);
	gv.AddItem(group, "DustPerSecond", 22.0f);
	gv.AddItem(group, "DustRadius", 9.0f);
	gv.AddItem(group, "DustHeightMin", -1.0f);
	gv.AddItem(group, "DustHeightMax", 5.0f);

	// 天井から落ちる水滴と波紋（仕様書 4.4）。張り出しの岩の下面は y≒3.4〜5、床の上面は y≒-5.05
	gv.AddItem(group, "DripEnabled", true);
	gv.AddItem(group, "DripPerSecond", 0.8f);
	gv.AddItem(group, "DripRadius", 18.0f);
	gv.AddItem(group, "DripCeilingHeight", 3.6f);
	gv.AddItem(group, "DripFloorHeight", -5.05f);
	gv.AddItem(group, "DripVolume", 0.5f);
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

	params.heightFogEnabled = gv.GetValueRef<bool>(group, "HeightFogEnabled");
	params.heightFogColor = gv.GetValueRef<Vector3>(group, "HeightFogColor");
	params.heightFogBaseHeight = gv.GetValueRef<float>(group, "HeightFogBaseHeight");
	params.heightFogDensity = gv.GetValueRef<float>(group, "HeightFogDensity");
	params.heightFogFalloff = gv.GetValueRef<float>(group, "HeightFogFalloff");
	params.heightFogMaxOpacity = gv.GetValueRef<float>(group, "HeightFogMaxOpacity");

	for (int i = 0; i < kUpperFogLayerCount; ++i) {
		const std::string prefix = "FogLayer" + std::to_string(i + 1) + "_";
		auto& layer = params.fogLayers[i];
		layer.enabled = gv.GetValueRef<bool>(group, prefix + "Enabled");
		layer.height = gv.GetValueRef<float>(group, prefix + "Height");
		layer.color = gv.GetValueRef<Vector3>(group, prefix + "Color");
		layer.opacity = gv.GetValueRef<float>(group, prefix + "Opacity");
		layer.noiseScale = gv.GetValueRef<float>(group, prefix + "NoiseScale");
		layer.noiseSpeed = gv.GetValueRef<Vector3>(group, prefix + "NoiseSpeed");
	}
	params.fogLayerHalfSize = gv.GetValueRef<float>(group, "FogLayerHalfSize");
	params.fogLayerEdgeFade = gv.GetValueRef<float>(group, "FogLayerEdgeFade");
	params.fogLayerSoftDistance = gv.GetValueRef<float>(group, "FogLayerSoftDistance");

	{
		auto& mist = params.fogLayers[RenderEnvironment::Params::kGroundMistLayer];
		mist.enabled = gv.GetValueRef<bool>(group, "GroundMist_Enabled");
		mist.height = gv.GetValueRef<float>(group, "GroundMist_Height");
		mist.color = gv.GetValueRef<Vector3>(group, "GroundMist_Color");
		const float battleScale = gv.GetValueRef<float>(group, "GroundMist_BattleOpacityScale");
		mist.opacity = gv.GetValueRef<float>(group, "GroundMist_Opacity") * (1.0f + (battleScale - 1.0f) * g_battleBlend);
		mist.noiseScale = gv.GetValueRef<float>(group, "GroundMist_NoiseScale");
		mist.noiseSpeed = gv.GetValueRef<Vector3>(group, "GroundMist_NoiseSpeed");
		mist.softDistance = gv.GetValueRef<float>(group, "GroundMist_SoftDistance");
		mist.grazingFade = false;
	}

	params.distanceFogEnabled = gv.GetValueRef<bool>(group, "DistanceFogEnabled");
	params.distanceFogColor = gv.GetValueRef<Vector3>(group, "DistanceFogColor");
	params.distanceFogDensity = gv.GetValueRef<float>(group, "DistanceFogDensity");
	params.distanceFogStart = gv.GetValueRef<float>(group, "DistanceFogStart");
	params.distanceFogMaxOpacity = gv.GetValueRef<float>(group, "DistanceFogMaxOpacity");

	params.toneMapEnabled = gv.GetValueRef<bool>(group, "ToneMapEnabled");
	params.exposure = gv.GetValueRef<float>(group, "Exposure");
	params.toneMapShoulderStart = gv.GetValueRef<float>(group, "ToneMapShoulderStart");

	params.ssaoEnabled = gv.GetValueRef<bool>(group, "SsaoEnabled");
	params.ssaoRadius = gv.GetValueRef<float>(group, "SsaoRadius");
	params.ssaoBias = gv.GetValueRef<float>(group, "SsaoBias");
	params.ssaoIntensity = gv.GetValueRef<float>(group, "SsaoIntensity");
	params.ssaoPower = gv.GetValueRef<float>(group, "SsaoPower");
	params.ssaoAmbientStrength = gv.GetValueRef<float>(group, "SsaoAmbientStrength");
	params.ssaoDirectStrength = gv.GetValueRef<float>(group, "SsaoDirectStrength");
	params.volumetricEnabled = gv.GetValueRef<bool>(group, "VolumetricEnabled");
	params.volumetricStrength = gv.GetValueRef<float>(group, "VolumetricStrength");
	params.volumetricMaxDistance = gv.GetValueRef<float>(group, "VolumetricMaxDistance");

	params.characterAmbient = gv.GetValueRef<Vector3>(group, "CharacterAmbient");
	params.rimColor = gv.GetValueRef<Vector3>(group, "RimColor");
	params.rimPower = gv.GetValueRef<float>(group, "RimPower");
	RenderEnvironment::GetInstance().SetParams(params);

	SkySystem::GetInstance().SetVisible(gv.GetValueRef<bool>(group, "SkyVisible"));

	if (BloomEffect* bloom = FindBloom()) {
		if (!g_bloomOverridden) {
			g_bloomOriginal = bloom->GetSettings();
			g_bloomWasActive = bloom->IsActive();
			g_bloomOverridden = true;
		}
		bloom->SetActive(gv.GetValueRef<bool>(group, "BloomEnabled"));
		auto& s = bloom->GetSettings();
		s.threshold = gv.GetValueRef<float>(group, "BloomThreshold");
		s.softKnee = gv.GetValueRef<float>(group, "BloomSoftKnee");
		s.blurRadius = gv.GetValueRef<float>(group, "BloomBlurRadius");
		s.intensity = gv.GetValueRef<float>(group, "BloomIntensity");
	}

	g_torchFlickerScale = gv.GetValueRef<float>(group, "TorchFlickerStrength");
	g_particleSpawnRate = gv.GetValueRef<float>(group, "ParticleSpawnRate");
	g_dust.enabled = gv.GetValueRef<bool>(group, "DustEnabled");
	g_dust.perSecond = gv.GetValueRef<float>(group, "DustPerSecond") * g_particleSpawnRate;
	g_dust.radius = gv.GetValueRef<float>(group, "DustRadius");
	g_dust.heightMin = gv.GetValueRef<float>(group, "DustHeightMin");
	g_dust.heightMax = gv.GetValueRef<float>(group, "DustHeightMax");
	g_drip.enabled = gv.GetValueRef<bool>(group, "DripEnabled");
	g_drip.perSecond = gv.GetValueRef<float>(group, "DripPerSecond") * g_particleSpawnRate;
	g_drip.radius = gv.GetValueRef<float>(group, "DripRadius");
	g_drip.ceilingHeight = gv.GetValueRef<float>(group, "DripCeilingHeight");
	g_drip.floorHeight = gv.GetValueRef<float>(group, "DripFloorHeight");
	g_drip.volume = gv.GetValueRef<float>(group, "DripVolume");

	if (ColorGradingEffect* grading = EnsureGradingEffect()) {
		const bool enabled = gv.GetValueRef<bool>(group, "GradingEnabled");
		grading->SetActive(enabled);
		auto& d = grading->GetEffectData();
		d.strength = enabled ? gv.GetValueRef<float>(group, "GradingStrength") : 0.0f;
		d.gain = gv.GetValueRef<Vector3>(group, "GradingGain");
		d.lift = gv.GetValueRef<Vector3>(group, "GradingLift");
		d.saturation = gv.GetValueRef<float>(group, "GradingSaturation");
		d.contrast = gv.GetValueRef<float>(group, "GradingContrast");
		d.perceptual = 1.0f;
		d.splitBalance = gv.GetValueRef<float>(group, "GradingSplitBalance");
		d.contrastPivot = gv.GetValueRef<float>(group, "GradingContrastPivot");
		d.shadowTint = gv.GetValueRef<Vector3>(group, "GradingShadowTint");
		d.shadowTintStrength = gv.GetValueRef<float>(group, "GradingShadowTintStrength");
		d.highlightTint = gv.GetValueRef<Vector3>(group, "GradingHighlightTint");
		d.highlightTintStrength = gv.GetValueRef<float>(group, "GradingHighlightTintStrength");
	}
}

} // namespace

void StageEnvironment::Apply(const std::string& environmentName)
{
	// 色調整のパスはチェーンの登録順にかかる。ボスのブレスなど、ステージを組み立てたときに
	// 足される演出より前に掛けたいので、雰囲気が無いステージでも先に（無効のまま）作っておく
	EnsureGradingEffect();
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

void StageEnvironment::Update(float battleBlend)
{
	g_battleBlend = battleBlend;
	if (g_group.empty()) {
		return;
	}
	PushToRenderer(GlobalVariables::GetInstance(), g_group);
}

void StageEnvironment::Clear()
{
	g_group.clear();
	RenderEnvironment::GetInstance().ResetToDefault();
	RestoreBloom();
	g_torchFlickerScale = 1.0f;
	g_particleSpawnRate = 1.0f;
	g_dust = StageEnvironment::DustSettings{};
	g_drip = StageEnvironment::DripSettings{};
	SkySystem::GetInstance().SetVisible(true);
	if (auto* grading = dynamic_cast<ColorGradingEffect*>(OffScreenManager::GetInstance().FindEffect(kGradingEffectName))) {
		grading->SetActive(false);
		grading->GetEffectData().strength = 0.0f;
	}
}

float StageEnvironment::GetTorchFlickerScale()
{
	return g_torchFlickerScale;
}

float StageEnvironment::GetParticleSpawnRate()
{
	return g_particleSpawnRate;
}

const StageEnvironment::DustSettings& StageEnvironment::GetDustSettings()
{
	return g_dust;
}

const StageEnvironment::DripSettings& StageEnvironment::GetDripSettings()
{
	return g_drip;
}
