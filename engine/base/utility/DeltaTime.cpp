#include "DeltaTime.h"
#include <thread>

std::chrono::high_resolution_clock::time_point DeltaTime::preTime_;
float DeltaTime::deltaTime_ = 0.0f;

void DeltaTime::Initialize()
{
    preTime_ = std::chrono::high_resolution_clock::now();
	deltaTime_ = 0.0f;
}

void DeltaTime::Update()
{
    auto currentTime = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed = currentTime - preTime_;
    deltaTime_ = elapsed.count(); // 秒で保持
    preTime_ = currentTime;
}
