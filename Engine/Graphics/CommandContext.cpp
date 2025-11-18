#include "CommandContext.h"
#include <base/Logger.h>
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
	commandAllocator_.Reset();
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

	// コマンドアロケータを生成する
	hr = device_->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&commandAllocator_));
	// コマンドアロケータの生成がうまくいかなかったので起動できない
	if (FAILED(hr)) return false;
	commandAllocator_->SetName(L"CommandAllocator");
	Logger::Log("Complete create ID3D12CommandAllocator!!!\n");// コマンドアロケータ生成完了のログを出す

	// コマンドリストを生成する
	hr = device_->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, commandAllocator_.Get(), nullptr, IID_PPV_ARGS(&commandList_));
	// コマンドリストの生成がうまくいかなかったので起動できない
	if (FAILED(hr)) return false;
	commandList_->SetName(L"CommandList");
	Logger::Log("Complete create ID3D12GraphicsCommandList!!!\n");// コマンドリスト生成完了のログを出す

	return true;
}

void CommandContext::Begin()
{
	// Fenceの値を更新
	fenceValue_++;
	// GPUがここまでたどり着いたときに、Fenceの値を指定した値に代入するようにSignalを送る
	commandQueue_->Signal(fence_.Get(), fenceValue_);

	if (fence_->GetCompletedValue() < fenceValue_) {
		// 指定したSignalにたどりつけてないので、たどり着くまで待つようにイベントを設定する
		fence_->SetEventOnCompletion(fenceValue_, fenceEvent_);
		// イベントを待つ
		WaitForSingleObject(fenceEvent_, INFINITE);
	}

	// 次のフレーム用にリセットしておく
	Logger::CheckHR(commandAllocator_->Reset(), "Failed to reset command allocator");
	Logger::CheckHR(commandList_->Reset(commandAllocator_.Get(), nullptr), "Failed to reset command list.");
}


void CommandContext::Flush()
{
	// コマンドリストの内容和確定させる
	Logger::CheckHR(commandList_->Close(), "Failed to close command list");

	// コマンドリストを実行
	ID3D12CommandList* lists[] = { commandList_.Get() };
	commandQueue_->ExecuteCommandLists(1, lists);
}

void CommandContext::FlushAndWait()
{
	Logger::CheckHR(commandList_->Close(), "Failed to close command list");

	ID3D12CommandList* commandLists[] = { commandList_.Get() };
	commandQueue_->ExecuteCommandLists(1, commandLists);

	// Fence シグナル & 待機
	fenceValue_++;
	commandQueue_->Signal(fence_.Get(), fenceValue_);
	if (fence_->GetCompletedValue() < fenceValue_) {
		fence_->SetEventOnCompletion(fenceValue_, fenceEvent_);
		WaitForSingleObject(fenceEvent_, INFINITE);
	}

	// 次フレーム用にリセット
	Logger::CheckHR(commandAllocator_->Reset(), "Failed to reset allocator");
	Logger::CheckHR(commandList_->Reset(commandAllocator_.Get(), nullptr), "Failed to reset command list");
}

void CommandContext::TransitionResource(ID3D12Resource* resource, D3D12_RESOURCE_STATES before, D3D12_RESOURCE_STATES after)
{
	barrier_.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier_.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	barrier_.Transition.pResource = resource;
	barrier_.Transition.StateBefore = before;
	barrier_.Transition.StateAfter = after;
	barrier_.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

	// デバッグ出力用ログ作成
	std::wstringstream ss;
	ss << L"[Transition] Resource: " << resource
		<< L" | From: " << std::hex << before
		<< L" | To: " << std::hex << after << std::endl;

	OutputDebugStringW(ss.str().c_str());


	commandList_->ResourceBarrier(1, &barrier_);
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
		commandList_->OMSetRenderTargets(
			rtvCount,
			rtvHandles,
			FALSE,
			dsvHandle
		);
	} else {
		commandList_->OMSetRenderTargets(
			rtvCount,
			rtvHandles,
			FALSE,
			nullptr
		);
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
