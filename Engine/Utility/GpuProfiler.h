#pragma once
#include <wrl.h>
#include <d3d12.h>
#include <string>
#include <vector>

class DirectXManager;

/// <summary>
/// 描画パスごとの GPU 処理時間（タイムスタンプのクエリ）。CPU 側の ScopeProfiler とは別に、GPU が実際に使った時間を測る。
///
/// 使い方: RenderPipeline::Execute の最初に BeginFrame、測りたい区間を Begin/End で挟み、
/// コマンドリストを閉じる前に EndFrame（結果を読み戻し用バッファへ書き出す）。
/// エンジンは毎フレーム GPU の完了を待つので、次のフレームの BeginFrame の時点で前のフレームの結果が読める。
/// 結果は Entries()（平均とピーク、ミリ秒）。Profiler / Cave Lighting ウィンドウが表示する。
/// </summary>
class GpuProfiler
{
public:
	struct Entry {
		std::string name;
		double avgMs = 0.0;  // なめらかにした平均
		double lastMs = 0.0; // 直近のフレーム
		double maxMs = 0.0;  // 直近しばらくのピーク
	};

	static GpuProfiler& GetInstance();

	void Initialize(DirectXManager* dxManager);
	void Finalize();

	/// <summary>前のフレームの結果を読み、このフレームの計測を始める</summary>
	void BeginFrame();
	/// <summary>区間の始まり。同じ名前はフレームに1回だけ</summary>
	void Begin(const char* name);
	void End(const char* name);
	/// <summary>このフレームの結果を読み戻し用バッファへ書き出す（コマンドリストを閉じる前に呼ぶ）</summary>
	void EndFrame();

	const std::vector<Entry>& Entries() const { return entries_; }
	/// <summary>計測した区間の合計（ミリ秒、平均）</summary>
	double TotalMs() const;

private:
	GpuProfiler() = default;
	GpuProfiler(const GpuProfiler&) = delete;
	GpuProfiler& operator=(const GpuProfiler&) = delete;

	static constexpr uint32_t kMaxScopes = 32;

	struct Scope {
		const char* name;
		uint32_t beginIndex;
		uint32_t endIndex;
	};

	Entry& FindOrAdd(const char* name);

	DirectXManager* dxManager_ = nullptr;
	Microsoft::WRL::ComPtr<ID3D12QueryHeap> queryHeap_;
	Microsoft::WRL::ComPtr<ID3D12Resource> readback_;
	uint64_t frequency_ = 0;

	// 読み戻し先は2枠を交互に使う。CPU と GPU が並行して走るので、前のフレームの結果は
	// まだ GPU が書いている最中かもしれない。2フレーム前の枠（＝提出前の待ちで完了が確定している）を読む
	static constexpr uint32_t kSlotCount = 2;
	std::vector<Scope> scopes_;                 // このフレームの区間
	std::vector<Scope> slotScopes_[kSlotCount]; // 枠ごとの区間（結果の読み取り用）
	bool slotResolved_[kSlotCount]{};
	uint32_t slot_ = 0;                          // このフレームが書く枠
	uint32_t nextIndex_ = 0;
	uint32_t frameCount_ = 0;

	std::vector<Entry> entries_;
};

/// <summary>区間を GPU 計測で挟む（スコープを抜けると End）</summary>
class GpuProfileScope
{
public:
	explicit GpuProfileScope(const char* name) : name_(name) { GpuProfiler::GetInstance().Begin(name_); }
	~GpuProfileScope() { GpuProfiler::GetInstance().End(name_); }
private:
	const char* name_;
};
