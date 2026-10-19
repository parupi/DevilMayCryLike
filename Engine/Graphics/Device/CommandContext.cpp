#include "CommandContext.h"
#include <Utility/Logger.h>
#include <Utility/ScopeProfiler.h>
#include <stdexcept>
#include <format>
#include <sstream>
#include <Windows.h> // OutputDebugString に必要

CommandContext::~CommandContext()
{
	// --- GPUの完了を確実に待つ処理 ---
	if (commandQueue_ && fence_) {
		fenceValue_++;
		//
		Logger::CheckHR(commandQueue_->Signal(fence_.Get(), fenceValue_), "");
		//
		if (fence_->GetCompletedValue() < fenceValue_) {
			if (!fenceEvent_) {
				fenceEvent_ = CreateEvent(nullptr, FALSE, FALSE, nullptr);
			}
			//
			Logger::CheckHR(fence_->SetEventOnCompletion(fenceValue_, fenceEvent_), "");

			WaitForSingleObject(fenceEvent_, INFINITE);
		}
	}

	// --- フェンスイベント解放 ---
	if (fenceEvent_) {
		CloseHandle(fenceEvent_);
		fenceEvent_ = nullptr;
	}

	device_ = nullptr;

	commandList_.Reset();
	for (auto& allocator : commandAllocators_) {
		allocator.Reset();
	}
	commandQueue_.Reset();

	fence_.Reset();
}

bool CommandContext::Initialize(ID3D12Device* device)
{
	// デバイスがnullだったら生成失敗
	if (device == nullptr) return false;
	// 保持しておく
	device_ = device;

	HRESULT hr;
	// コマンドキューを生成する
	D3D12_COMMAND_QUEUE_DESC commandQueueDesc{};
	hr = device_->CreateCommandQueue(&commandQueueDesc, IID_PPV_ARGS(&commandQueue_));
	if (FAILED(hr)) return false;
	commandQueue_->SetName(L"CommandQueue");
	Logger::Log("Complete create ID3D12CommandQueue!!!\n");// コマンドキュー生成完了のログを出す

	// コマンドアロケータを生成する。GPU が前のフレームのコマンドを読んでいる間に次のフレームを積めるよう2つ持つ
	for (uint32_t i = 0; i < kAllocatorCount; ++i) {
		hr = device_->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&commandAllocators_[i]));
		// コマンドアロケータの生成がうまくいかなかったので起動できない
		if (FAILED(hr)) return false;
		commandAllocators_[i]->SetName(i == 0 ? L"CommandAllocator0" : L"CommandAllocator1");
	}
	allocatorIndex_ = 0;
	Logger::Log("Complete create ID3D12CommandAllocator!!!\n");// コマンドアロケータ生成完了のログを出す

	// コマンドリストを生成する
	hr = device_->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, commandAllocators_[allocatorIndex_].Get(), nullptr, IID_PPV_ARGS(&commandList_));
	// コマンドリストの生成がうまくいかなかったので起動できない
	if (FAILED(hr)) return false;
	commandList_->SetName(L"CommandList");
	Logger::Log("Complete create ID3D12GraphicsCommandList!!!\n");// コマンドリスト生成完了のログを出す

	return true;
}

void CommandContext::WaitForFenceValue(uint64_t value)
{
	if (value == 0 || fence_->GetCompletedValue() >= value) return;
	Logger::CheckHR(fence_->SetEventOnCompletion(value, fenceEvent_), "Failed to set fence event on completion");
	WaitForSingleObject(fenceEvent_, INFINITE);
}

void CommandContext::WaitForLastSubmit()
{
	WaitForFenceValue(fenceValue_);
}

void CommandContext::Begin()
{
	// 次のアロケータへ切り替える。今提出したほうは GPU がまだ読んでいるかもしれないので触らない
	allocatorIndex_ = (allocatorIndex_ + 1) % kAllocatorCount;

	// このアロケータで積んだコマンドを GPU が読み終えていればリセットしてよい。
	// 提出（Flush）のたびに前の提出の完了を待っているので、ふつうはもう終わっている
	WaitForFenceValue(allocatorFenceValues_[allocatorIndex_]);

	ID3D12CommandAllocator* allocator = commandAllocators_[allocatorIndex_].Get();
	Logger::CheckHR(allocator->Reset(), "Failed to reset command allocator");
	Logger::CheckHR(commandList_->Reset(allocator, nullptr), "Failed to reset command list.");
}

void CommandContext::ExecuteClosedList()
{
	// GPU に同時に積むのは1フレームまで。前に提出したものが終わるまで待ってから、
	// CPU 側の写しを GPU のバッファへ移し（提出前コールバック）、それから実行する。
	// 待つのがここ（＝次のフレームの更新とコマンド積みの後）なので、その間 GPU は前のフレームを描いていられる
	{
		PROF_SCOPE("Gfx:前フレームのGPU待ち");
		WaitForLastSubmit();
	}
	if (preExecute_) {
		PROF_SCOPE("Gfx:アップロード転送");
		preExecute_();
	}

	ID3D12CommandList* lists[] = { commandList_.Get() };
	commandQueue_->ExecuteCommandLists(1, lists);

	fenceValue_++;
	Logger::CheckHR(commandQueue_->Signal(fence_.Get(), fenceValue_), "Failed to signal command queue");
	allocatorFenceValues_[allocatorIndex_] = fenceValue_;
}

void CommandContext::Flush()
{
	Logger::CheckHR(commandList_->Close(), "Failed to close command list");
	ExecuteClosedList();
}

void CommandContext::FlushAndWait()
{
	Logger::CheckHR(commandList_->Close(), "Failed to close command list");
	ExecuteClosedList();

	// 完了まで待つ。待ち終えたので同じアロケータをそのままリセットして積み直してよい
	WaitForLastSubmit();
	ID3D12CommandAllocator* allocator = commandAllocators_[allocatorIndex_].Get();
	Logger::CheckHR(allocator->Reset(), "Failed to reset allocator");
	Logger::CheckHR(commandList_->Reset(allocator, nullptr), "Failed to reset command list");
}

void CommandContext::TransitionResource(ID3D12Resource* resource, D3D12_RESOURCE_STATES before, D3D12_RESOURCE_STATES after)
{
    D3D12_RESOURCE_BARRIER barrier{};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    barrier.Transition.pResource = resource;
    barrier.Transition.StateBefore = before;
    barrier.Transition.StateAfter = after;
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

    commandList_->ResourceBarrier(1, &barrier);
}

void CommandContext::SetRenderTarget(D3D12_CPU_DESCRIPTOR_HANDLE rtv, D3D12_CPU_DESCRIPTOR_HANDLE dsv)
{
	commandList_->OMSetRenderTargets(1, &rtv, false, &dsv);
}

void CommandContext::SetRenderTarget(D3D12_CPU_DESCRIPTOR_HANDLE rtv)
{
	commandList_->OMSetRenderTargets(1, &rtv, FALSE, nullptr);
}

void CommandContext::SetRenderTargets(const D3D12_CPU_DESCRIPTOR_HANDLE* rtvHandles, UINT rtvCount, const D3D12_CPU_DESCRIPTOR_HANDLE* dsvHandle)
{
	if (dsvHandle) {
		commandList_->OMSetRenderTargets(rtvCount, rtvHandles, FALSE, dsvHandle);
	} else {
		commandList_->OMSetRenderTargets(rtvCount, rtvHandles, FALSE, nullptr);
	}
}

void CommandContext::ClearRenderTarget(D3D12_CPU_DESCRIPTOR_HANDLE rtv, const FLOAT clearColor[4])
{
	commandList_->ClearRenderTargetView(rtv, clearColor, 0, nullptr);
}

void CommandContext::ClearDepth(D3D12_CPU_DESCRIPTOR_HANDLE dsv)
{
	commandList_->ClearDepthStencilView(dsv, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
}

void CommandContext::SetViewportAndScissor(const D3D12_VIEWPORT& viewport, const D3D12_RECT& scissorRect)
{
	commandList_->RSSetViewports(1, &viewport);
	commandList_->RSSetScissorRects(1, &scissorRect);
}

void CommandContext::CreateFence()
{
	Logger::CheckHR(device_->CreateFence(fenceValue_, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence_)), "");

	// FenceのSignalを持つためのイベントを作成する
	fenceEvent_ = CreateEvent(NULL, FALSE, FALSE, NULL);
	Logger::CheckHR(fenceEvent_ == nullptr, "");

	fence_->SetName(L"Fence");
}
