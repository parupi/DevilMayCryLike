#pragma once
#include <vector>
#include <string>
#include <GameObject/Character/CharacterStructs.h>

enum class PlayState
{
    Stop,
    Playing,
    Paused,
};

class Player;
class PlayerStateAttack;

class AttackPlayer
{
public:
    AttackPlayer() = default;
    ~AttackPlayer() = default;

    void SetPlayer(Player* player);
    void SetAttacks(std::vector<PlayerStateAttack*> attacks);

    void Update(float deltaTime);
    void DrawEditorContents(); // ウィンドウは App/Editor/Windows/PlayerWindow.cpp が開く

    /// <summary>
    /// プレビュー再生中か。プレビューも制御点で剣を動かすので、
    /// 剣を手のボーンへ持たせる側（PlayerWeapon）がこれを見て譲る
    /// </summary>
    bool IsPlaying() const { return isPlaying_; }

private:
    void Play();
    void Stop();
    void Pause();
    void Resume();
    void Seek(float time);

private:
    Player* player_ = nullptr;
    PlayerStateAttack* currentAttack_ = nullptr;
    PlayState state_ = PlayState::Stop;

    int selectedAttackIndex_ = 0;
    bool isPlaying_ = false;

    float debugTime_ = 0.0f; // ImGui表示用
    float seekTime_ = 0.0f;      // スライダー用

    std::vector<PlayerStateAttack*> attacks_;
};
