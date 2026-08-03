#pragma once
#include <string>
#include <vector>
#include <nlohmann/json.hpp>
#include "Math/Vector3.h"
#include "ParticleCurves.h"
#include "ParticleEmitter.h"
#include "World3D/Object/Renderer/PrimitiveType.h"

/// <summary>
/// .vfx.json に入っているパーティクルグループ1つぶんの定義。
///
/// パラメータを `ParticleParameters` ではなく生の json で持っているのがポイント。
/// GlobalVariables のグループをそのまま出し入れするので、
/// **後からパラメータが増えてもこのファイルを触らずに保存対象へ入る**。
/// </summary>
struct VFXParticleDef
{
	std::string name;
	/// <summary>テクスチャのファイル名。これまで C++ にベタ書きしていたものがデータへ移った</summary>
	std::string texture = "white.png";
	/// <summary>形状。GPUリソースに紐づくのでグループ生成時にしか決められない</summary>
	PrimitiveType shape = PrimitiveType::Plane;
	/// <summary>GlobalVariables のグループをそのまま写したもの</summary>
	nlohmann::json params = nlohmann::json::object();
	/// <summary>時間変化カーブ。GlobalVariables では表現できないので構造体で持つ</summary>
	ParticleCurves curves;
};

/// <summary>.vfx.json のエミッター定義（発生のさせ方と、束ねるパーティクルの一覧）</summary>
struct VFXEmitterDef
{
	float frequency = 0.5f;
	/// <summary>常駐発生させるか。ワンショット専用のVFXは false のままでよい</summary>
	bool isActive = false;
	/// <summary>メッシュ表面から発生させる場合のモデル名。空なら通常の点エミット</summary>
	std::string shapeModel;
	/// <summary>親を持たないときの発生位置</summary>
	Vector3 position{};
	std::vector<EmitterParticle> particles;
};

/// <summary>1エフェクトぶんの定義。これ1つで .vfx.json 1ファイルに対応する</summary>
struct VFXDefinition
{
	std::string name;
	VFXEmitterDef emitter;
	std::vector<VFXParticleDef> particles;
};

/// <summary>
/// Resource/VFX/&lt;Name&gt;.vfx.json の読み書き（設計書 §21-22）。
///
/// これまで1エフェクトの定義は
///   Resource/GlobalVariables/Particle/&lt;Group&gt;.json（パラメータ）
///   Resource/Particle/&lt;Group&gt;.curve.json（カーブ）
///   Resource/Emitter/&lt;Emitter&gt;.json（発生設定）
///   GameScene の CreateParticleGroup 呼び出し（テクスチャと形状）
/// の4か所に散らばっていた。それを1ファイルにまとめる。
///
/// 既存の3経路はそのまま残してあるので、移行は1エフェクトずつ進められる。
/// </summary>
namespace VFXFile {

	/// <summary>Resource/VFX/&lt;vfxName&gt;.vfx.json</summary>
	std::string MakeFilePath(const std::string& vfxName);

	/// <summary>読み込む。ファイルが無い・壊れている場合は false（起動は止めない）</summary>
	bool Load(const std::string& vfxName, VFXDefinition& outDefinition);

	/// <summary>書き出す。ディレクトリが無ければ作る</summary>
	bool Save(const VFXDefinition& definition);

	/// <summary>Resource/VFX/ にある .vfx.json の名前一覧（拡張子なし）</summary>
	std::vector<std::string> ListNames();

	/// <summary>形状名（"Plane" / "Ring" / "Cylinder"）との相互変換</summary>
	const char* ToString(PrimitiveType shape);
	PrimitiveType ShapeFromString(const std::string& name);

} // namespace VFXFile
