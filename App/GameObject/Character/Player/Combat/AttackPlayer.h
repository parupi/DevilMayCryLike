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
    void DrawImGui();

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
