#include "GpuProfiler.h"
#include "Graphics/Device/DirectXManager.h"
#include <algorithm>
#include <cassert>
#include <cstring>

GpuProfiler& GpuProfiler::GetInstance()
{
	static GpuProfiler instance;
	return instance;
}

void GpuProfiler::Initialize(DirectXManager* dxManager)
{
	dxManager_ = dxManager;
	ID3D12Device* device = dxManager_->GetDevice();

	D3D12_QUERY_HEAP_DESC heapDesc{};
	heapDesc.Type = D3D12_QUERY_HEAP_TYPE_TIMESTAMP;
	heapDesc.Count = kMaxScopes * 2;
	HRESULT hr = device->CreateQueryHeap(&heapDesc, IID_PPV_ARGS(&queryHeap_));
	assert(SUCCEEDED(hr));

	D3D12_HEAP_PROPERTIES heapProps{};
	heapProps.Type = D3D12_HEAP_TYPE_READBACK;
	D3D12_RESOURCE_DESC desc{};
	desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	desc.Width = sizeof(uint64_t) * kMaxScopes * 2 * kSlotCount;
	desc.Height = 1;
	desc.DepthOrArraySize = 1;
	desc.MipLevels = 1;
	desc.SampleDesc.Count = 1;
	desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	hr = device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &desc,
		D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&readback_));
	assert(SUCCEEDED(hr));
	(void)hr;

	dxManager_->GetCommandContext()->GetCommandQueue()->GetTimestampFrequency(&frequency_);
}

void GpuProfiler::Finalize()
{
	queryHeap_.Reset();
	readback_.Reset();
	dxManager_ = nullptr;
}

GpuProfiler::Entry& GpuProfiler::FindOrAdd(const char* name)
{
	for (Entry& entry : entries_) {
		if (entry.name == name) {
			return entry;
		}
	}
	entries_.push_back(Entry{ name });
	return entries_.back();
}

void GpuProfiler::BeginFrame()
{
	if (!queryHeap_) return;

	// このフレームが書く枠を進める。そこに残っているのは2フレーム前の結果で、
	// その GPU 処理は前のフレームの提出前に待ち終えているので、もう書き込まれている
	slot_ = (slot_ + 1) % kSlotCount;
	const std::vector<Scope>& lastScopes = slotScopes_[slot_];
	if (slotResolved_[slot_] && frequency_ > 0 && !lastScopes.empty()) {
		const uint32_t count = kMaxScopes * 2;
		const size_t offset = sizeof(uint64_t) * count * slot_;
		D3D12_RANGE range{ offset, offset + sizeof(uint64_t) * count };
		uint8_t* mapped = nullptr;
		if (SUCCEEDED(readback_->Map(0, &range, reinterpret_cast<void**>(&mapped)))) {
			const uint64_t* data = reinterpret_cast<const uint64_t*>(mapped + offset);
			++frameCount_;
			// ピークはしばらくで忘れる（一瞬の山がずっと残らないように）
			const bool resetPeak = (frameCount_ % 120) == 0;
			for (const Scope& scope : lastScopes) {
				const uint64_t begin = data[scope.beginIndex];
				const uint64_t end = data[scope.endIndex];
				const double ms = end > begin ? double(end - begin) * 1000.0 / double(frequency_) : 0.0;
				Entry& entry = FindOrAdd(scope.name);
				entry.lastMs = ms;
				entry.avgMs = entry.avgMs <= 0.0 ? ms : entry.avgMs * 0.9 + ms * 0.1;
				entry.maxMs = resetPeak ? ms : (std::max)(entry.maxMs, ms);
			}
			D3D12_RANGE noWrite{ 0, 0 };
			readback_->Unmap(0, &noWrite);
		}
	}

	scopes_.clear();
	nextIndex_ = 0;
	slotResolved_[slot_] = false;
}

void GpuProfiler::Begin(const char* name)
{
	if (!queryHeap_ || nextIndex_ + 2 > kMaxScopes * 2) return;
	Scope scope{ name, nextIndex_, nextIndex_ + 1 };
	nextIndex_ += 2;
	dxManager_->GetCommandList()->EndQuery(queryHeap_.Get(), D3D12_QUERY_TYPE_TIMESTAMP, scope.beginIndex);
	scopes_.push_back(scope);
}

void GpuProfiler::End(const char* name)
{
	if (!queryHeap_) return;
	for (auto it = scopes_.rbegin(); it != scopes_.rend(); ++it) {
		if (std::strcmp(it->name, name) == 0) {
			dxManager_->GetCommandList()->EndQuery(queryHeap_.Get(), D3D12_QUERY_TYPE_TIMESTAMP, it->endIndex);
			return;
		}
	}
}

void GpuProfiler::EndFrame()
{
	if (!queryHeap_ || nextIndex_ == 0) return;
	const uint64_t offset = sizeof(uint64_t) * kMaxScopes * 2 * slot_;
	dxManager_->GetCommandList()->ResolveQueryData(queryHeap_.Get(), D3D12_QUERY_TYPE_TIMESTAMP, 0, nextIndex_, readback_.Get(), offset);
	slotScopes_[slot_] = scopes_;
	slotResolved_[slot_] = true;
}

double GpuProfiler::TotalMs() const
{
	double total = 0.0;
	for (const Entry& entry : entries_) {
		total += entry.avgMs;
	}
	return total;
}
