#pragma once
#include <string>
#include <vector>
#include <optional>
#include <nlohmann/json.hpp>
#include <Math/Vector3.h>
#include <Math/Quaternion.h>

/// <summary>
/// ステージデータ（Resource/Stage/*.json）の1オブジェクト分。
///
/// 値は**すべてエンジン空間**そのまま。読み込み時の座標変換は行わない。
/// （以前は Blender のレベルエディタが吐いた Y-up 右手系を変換していたが、
///   エンジン内エディタで作るようになったので変換は廃止した）
/// </summary>

// イベント（Event_* クラス）の設定
struct EventInfo {
	// "EnemySpawn" / "ForceBattle" / "BossSpawn" / "Clear"
	std::string type;
	// 対象にするオブジェクト名。
	// EnemySpawn / ForceBattle は出現させる敵、Clear は撃破対象の敵、
	// BossSpawn は先頭の1体だけをボスとして使う
	std::vector<std::string> targets;
};

// PointLight / ライト付き Prop のポイントライト設定
struct LightInfo {
	Vector3 color = { 1.0f, 1.0f, 1.0f };
	Vector3 offset = { 0.0f, 0.0f, 0.0f };
	float intensity = 1.5f;
	float radius = 10.0f;
	float decay = 1.0f;
};

enum class ColliderShape {
	None,
	OBB,
	Sphere,
};

// コライダー設定。エンジンの OBBData / SphereData と同じ持ち方をする
struct ColliderInfo {
	ColliderShape shape = ColliderShape::None;
	Vector3 offset = { 0.0f, 0.0f, 0.0f };
	Vector3 halfExtents = { 0.5f, 0.5f, 0.5f }; // OBB用
	float radius = 0.5f;                        // Sphere用
	bool isActive = true;
};

struct SceneObject {
	std::string name;
	std::string className = "Object3d";

	Vector3 translate = { 0.0f, 0.0f, 0.0f };
	Quaternion rotate;                      // 既定は単位クォータニオン
	Vector3 scale = { 1.0f, 1.0f, 1.0f };

	std::optional<std::string> modelName;
	std::optional<ColliderInfo> collider;
	std::optional<EventInfo> eventInfo;
	std::optional<LightInfo> lightInfo;
};

class SceneLoader
{
public:
	// ステージデータのフォーマット識別子とバージョン。SceneSaver と共有する
	static constexpr const char* kFormatName = "GuchisStage";
	static constexpr int kFormatVersion = 2;

	// ゲーム本編のステージ。エディタの保存先もここ
	static constexpr const char* kDefaultStagePath = "Resource/Stage/Stage.json";

	/// <summary>ステージデータを読み込む。開けない・形式が違う場合は例外を投げる</summary>
	static std::vector<SceneObject> Load(const std::string& path);

private:
	static SceneObject ParseObject(const nlohmann::json& j);
	static ColliderInfo ParseCollider(const nlohmann::json& j);
};
