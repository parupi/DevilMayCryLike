#pragma once
#include <string>

/// <summary>
/// ステージの雰囲気（背景色・空・アンビエント・平行光・高さによる暗化）を、
/// エンジンの RenderEnvironment と SkySystem へ流し込む。
///
/// どの雰囲気にするかはステージデータの "environment" に名前で書く（例: "Cave"）。
/// 中身は GlobalVariables の Resource/GlobalVariables/Environment/Environment_<名前>.json にあり、
/// Debug では GlobalVariables のウィンドウからその場で調整できる（毎フレーム読み直すので即反映）。
/// 名前が無いステージ（トレーニングルームなど）は既定の見た目のまま。
/// </summary>
namespace StageEnvironment {

/// <summary>シーン開始時に呼ぶ。空の名前なら既定に戻すだけ</summary>
void Apply(const std::string& environmentName);

/// <summary>毎フレーム呼ぶ。調整中の値を反映する。battleBlend は戦闘状態への寄り具合（0〜1。地面の霧を薄くする）</summary>
void Update(float battleBlend = 0.0f);

/// <summary>シーン終了時に呼ぶ。次のシーン（タイトル等）へ持ち越さない</summary>
void Clear();

/// <summary>松明の揺らぎの強さの倍率（雰囲気の "TorchFlickerStrength"。雰囲気が無ければ 1）。WallTorch が読む</summary>
float GetTorchFlickerScale();

/// <summary>環境パーティクル（埃・煙）の発生頻度の倍率（"ParticleSpawnRate"。雰囲気が無ければ 1）</summary>
float GetParticleSpawnRate();

/// <summary>プレイヤーのまわりを漂う埃の設定（CaveDustEmitter が読む）。雰囲気が無ければ無効</summary>
struct DustSettings {
	bool enabled = false;
	float perSecond = 0.0f;   // 1秒に出す粒数（ParticleSpawnRate を掛けた後）
	float radius = 9.0f;      // プレイヤーから水平にこの距離の中に出す[m]
	float heightMin = -1.0f;  // プレイヤーの位置からの高さの範囲[m]
	float heightMax = 5.0f;
};
const DustSettings& GetDustSettings();

/// <summary>天井から落ちる水滴の設定（CaveDripEmitter が読む）。雰囲気が無ければ無効</summary>
struct DripSettings {
	bool enabled = false;
	float perSecond = 0.0f;      // 1秒に落とす平均の数（ParticleSpawnRate を掛けた後）
	float radius = 18.0f;        // プレイヤーからこの距離の中にある岩からだけ落とす[m]
	float ceilingHeight = 3.6f;  // 水滴が出る高さ（張り出しの岩の下面・鍾乳石の先あたり）
	float floorHeight = -5.05f;  // 床の上面。ここで波紋が出る
	float volume = 0.5f;         // 着地の音量
};
const DripSettings& GetDripSettings();

} // namespace StageEnvironment
