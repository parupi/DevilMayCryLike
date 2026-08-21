#pragma once
/// <summary>
/// フレーム内の処理時間を名前ごとに集計する簡易プロファイラ。
///
/// 使い方:
///   void Foo() {
///       PROF_SCOPE("Foo");            // スコープを抜けるまでの時間を "Foo" に加算する
///       PROF_COUNT("Fooの件数", n);   // 時間ではない数値も一緒に出せる
///   }
/// 同じ名前で複数回計測した場合は1フレーム分が合算される（ループの中に置いてよい）。
/// 結果は エディタの Profiler ウィンドウ（Engine/Editor/Windows/ProfilerWindow.cpp）に出る。
/// 集計の確定は MyGameTitle::Update() の先頭が呼ぶ ScopeProfiler::EndFrame() が行う。
///
/// ヘッダオンリー（inline変数）なので、計測したいファイルでこれをincludeするだけで足りる。
/// Release ではマクロが空になり、計測コードごと消える。
/// </summary>
#include <chrono>
#include <string>
#include <vector>

#ifdef _DEBUG

class ScopeProfiler {
public:
	struct Entry {
		std::string name;
		double thisFrameMs = 0.0; // 今フレームの累計
		double avgMs = 0.0;       // 指数移動平均
		double maxMs = 0.0;       // 直近のピーク
		long long counter = 0;    // 任意のカウンタ（呼び出し回数など）
	};

	static inline std::vector<Entry> entries_;

	static Entry& Find(const char* name) {
		for (Entry& e : entries_) {
			if (e.name == name) return e;
		}
		entries_.push_back(Entry{ name });
		return entries_.back();
	}

	static void AddMs(const char* name, double ms) { Find(name).thisFrameMs += ms; }
	static void SetCounter(const char* name, long long value) { Find(name).counter = value; }

	// 毎フレーム1回、フレームの最後に呼ぶ
	static void EndFrame() {
		for (Entry& e : entries_) {
			// 60フレームぶんくらいで馴らす
			e.avgMs += (e.thisFrameMs - e.avgMs) * 0.05;
			if (e.thisFrameMs > e.maxMs) e.maxMs = e.thisFrameMs;
			e.thisFrameMs = 0.0;
		}
	}

	static void ResetPeaks() {
		for (Entry& e : entries_) e.maxMs = 0.0;
	}
};

class ScopeTimer {
public:
	explicit ScopeTimer(const char* name)
		: name_(name), start_(std::chrono::steady_clock::now()) {
	}
	~ScopeTimer() {
		const auto end = std::chrono::steady_clock::now();
		const double ms = std::chrono::duration<double, std::milli>(end - start_).count();
		ScopeProfiler::AddMs(name_, ms);
	}

private:
	const char* name_;
	std::chrono::steady_clock::time_point start_;
};

#define PROF_CONCAT_INNER(a, b) a##b
#define PROF_CONCAT(a, b) PROF_CONCAT_INNER(a, b)
#define PROF_SCOPE(name) ScopeTimer PROF_CONCAT(profScope_, __LINE__)(name)
#define PROF_COUNT(name, value) ScopeProfiler::SetCounter(name, static_cast<long long>(value))

#else

#define PROF_SCOPE(name) ((void)0)
#define PROF_COUNT(name, value) ((void)0)

#endif // _DEBUG
