#pragma once

/// <summary>
/// タイトルの OPTION から触る、プレイに関わる設定値。
///
/// 中身は GlobalVariables のグループひとつ（Resource/GlobalVariables/Settings/GameSettings.json）で、
/// このクラスは「範囲の決まった値として読み書きする」ぶんの面倒を見る。
/// 音量だけは SoundManager が同じやり方で先に持っているので、そちらをそのまま使うこと。
/// </summary>
class GameSettings
{
public:
	// カメラ感度の倍率。1.0 が GameCamera の既定値そのまま
	static constexpr float kMinSensitivity = 0.2f;
	static constexpr float kMaxSensitivity = 2.0f;
	/// <summary>OPTION で左右キー1回ぶんの変化量</summary>
	static constexpr float kSensitivityStep = 0.1f;

	static GameSettings& GetInstance();

	/// <summary>保存済みの設定を読み込む。未保存なら既定値のまま</summary>
	void Initialize();

	/// <summary>今の値をファイルへ書き出す。OPTION を閉じるときに呼ぶ</summary>
	void Save();

	/// <summary>右スティックの感度倍率</summary>
	float GetCameraSensitivity() const { return cameraSensitivity_; }
	void SetCameraSensitivity(float scale);

	/// <summary>カメラの上下を反転するか</summary>
	bool IsInvertCameraY() const { return invertCameraY_; }
	void SetInvertCameraY(bool invert);

	/// <summary>ゲーム開始時にチュートリアルを流すか。2周目以降に切れるようにするためのもの</summary>
	bool IsTutorialEnabled() const { return tutorialEnabled_; }
	void SetTutorialEnabled(bool enabled);

private:
	GameSettings() = default;
	~GameSettings() = default;
	GameSettings(const GameSettings&) = delete;
	GameSettings& operator=(const GameSettings&) = delete;

	// GlobalVariables 側の値をメンバへ取り込む
	void PullValues();

	float cameraSensitivity_ = 1.0f;
	bool invertCameraY_ = false;
	bool tutorialEnabled_ = true;

	bool isInitialized_ = false;
};
