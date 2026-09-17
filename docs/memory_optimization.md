# メモリ使用量の軽量化（2026-07-30）

「GameScene に入るとメモリ使用量が 1GB 弱から 4GB 近くまで跳ね上がる」問題の原因調査と対処の記録。

計測環境: Debug x64 / RTX 3050 Laptop (VRAM 4GB) + Intel UHD / RAM 16GB

---

## 1. 結果サマリ

| 指標 | 対処前 | GIF 縮小後 | ＋ステージング対策後 |
|---|---:|---:|---:|
| GameScene の **VRAM** | **1,876 MB** | **440 MB** | 440 MB |
| プロセスの **ピーク RAM** | **3,073 MB** | 678 MB | **563〜591 MB** |
| GameScene 定常 RAM | 339 MB | 334 MB | 346 MB |
| Title の VRAM | 172 MB | 172 MB | 172 MB |
| GameScene 初期化時間 | — | 3,883 ms | 3,803 ms |

**VRAM + ピーク RAM の合計で見ると、約 4.9GB → 約 1.0GB。**

VRAM を 1,436MB 削減したのが最大の成果。とくにこのマシンは **VRAM が 4GB しかない**ため、
チュートリアル画像だけで 1.8GB を占めていた状態は、他のリソースと合わせて容量を圧迫し、
オーバーコミット時のページングによるフレーム落ちを招きかねなかった。

---

## 2. 原因

### 2-1. 本丸: チュートリアル GIF が VRAM を 1.6GB 占有していた

`GifLoader::Load()`（`Engine/Graphics/Resource/GifLoader.cpp`）は、GIF の**全フレームを
非圧縮 RGBA8 の 1 枚の巨大なアトラステクスチャに展開**する。

そのため VRAM 使用量は

```
フレーム数 × 幅 × 高さ × 4 byte
```

だけで決まり、**GIF ファイル自体の圧縮サイズはまったく効かない**。

素材は約 55fps の画面キャプチャで、実際の表示サイズよりはるかに大きかった。

| GIF | 解像度 | フレーム数 | アトラス | VRAM |
|---|---|---:|---|---:|
| LockOn | 755×554 | 410 | 15855×11080 | **670.1 MB** |
| RoundUp | 755×554 | 173 | 15855×4986 | 301.6 MB |
| AttackA | 556×381 | 242 | 16124×3429 | 210.9 MB |
| AttackB | 556×375 | 257 | 16124×3375 | 207.6 MB |
| PlayerWalk | 556×381 | 142 | 16124×1905 | 117.2 MB |
| PlayerJump | 556×381 | 130 | 16124×1905 | 117.2 MB |
| | | | **合計** | **1,624.6 MB** |

LockOn.gif は 17.7MB のファイルが **670MB** に膨らんでいた（38 倍）。

さらに `TutorialSystem::Initialize()` が 6 本を**まとめて先読み**するため、
GameScene 入場時に一気に確保される。これが「GameScene に行くと爆発する」症状の正体。

**表示サイズとの乖離**: `App/Tutorial/Tutorial.cpp` は `SetSize({350.0f, 240.0f})` で
**350×240 px にしか描画していない**。必要な 5 倍以上のピクセルを、必要な倍以上の
フレーム数で積んでいた。

#### 時期の一致

```
LockOn.gif     2026-07-17  2a844b8  小物(Prop)・ポイントライト配置/発光演出を実装
RoundUp.gif    2026-07-17  2a844b8  (同上)
AttackA/B, PlayerJump も同じコミット
PlayerWalk.gif 2026-07-02  ddf343d
```

`2a844b8` 以前は PlayerWalk.gif の 117MB のみ。ここで +1,507MB 増えたのが「急に4倍」の起点。
作業中ブランチの他の変更（Bloom / HitPostEffect / StyleHUD 等）は合計しても数 MB で無関係だった。

### 2-2. 副次: アップロード用ステージングバッファのピーク

`DirectXManager::UploadTextureData()` はテクスチャ 1 枚ごとに専用のアップロードバッファを作り、
`ResourceManager::AddPendingUpload()` に積む。解放は `DirectXManager::EndDraw()` の
`ReleasePendingUploads()` の 1 回だけ。

`GameScene::Initialize()` は全ロードを 1 フレーム内で終えるため、
**そのフレームで作った全テクスチャ分のステージングが同時に生存**していた（実測 258MB）。

加えて、GIF 1 本につき同じ画素データが 3 箇所に同時に存在していた。

| # | 場所 | 最大 |
|---|---|---:|
| 1 | `GifLoader.cpp` の `std::vector<uint8_t> atlas` | 59 MB |
| 2 | `TextureManager::LoadTextureFromMemory` が `ScratchImage` へ memcpy | 59 MB |
| 3 | `UpdateSubresources` がアップロードバッファへ memcpy | (上記 258MB に計上) |

なお `ReleasePendingUploads()` の直前で `CommandContext::Begin()` がフェンスを待っているため、
**GPU がコピーを終える前に解放される危険は無い**。安全性の問題ではなく、純粋にピーク量の問題だった。

---

## 3. 対処

### 対処 1: GIF を表示サイズ・フレームレートまで落とす

`tools/shrink_tutorial_gifs.ps1`（ffmpeg 必要）を追加し、全 6 本を **350×240 / 20fps** へ再エンコードした。

| GIF | フレーム | 解像度 | 再生尺 | ファイル |
|---|---|---|---|---|
| LockOn | 410 → 150 | 755×554 → 350×240 | 7.51s → 7.50s | 16.9 → 3.3 MB |
| RoundUp | 173 → 64 | 755×554 → 350×240 | 3.20s → 3.20s | 9.2 → 1.9 MB |
| AttackA | 242 → 84 | 556×381 → 350×240 | 4.20s → 4.20s | 3.3 → 1.0 MB |
| AttackB | 257 → 91 | 556×375 → 350×240 | 4.66s → 4.55s | 4.7 → 1.6 MB |
| PlayerWalk | 142 → 53 | 556×381 → 350×240 | 2.63s → 2.65s | 4.1 → 1.9 MB |
| PlayerJump | 130 → 46 | 556×381 → 350×240 | 2.30s → 2.30s | 2.1 → 0.7 MB |

**効果: GIF アトラスの VRAM 1,624.6 MB → 191.6 MB（8.5 分の 1、−1,433 MB）**
（リポジトリサイズも 40.3MB → 10.4MB）

表示サイズが元々 350×240 なので**見た目は変わらない**。再生尺も維持されている。

> **注意**: 出力解像度 350×240 は `App/Tutorial/Tutorial.cpp` の `SetSize({350.0f, 240.0f})` と
> 対応している。**表示サイズを変えたら GIF を再変換すること。**
> スクリプトの `-Width` / `-Height` / `-Fps` で調整できる。

#### ハマりどころ: ffmpeg の `min_delay`

ffmpeg の GIF デマルチプレクサは既定 `min_delay=2` で、**1cs の遅延を `default_delay=10cs` に
置き換えてしまう**。素材の遅延は 1〜4cs なので、素直に変換すると LockOn の尺が
7.5s → 21s に伸び、**ゲーム中の再生が約 2.8 倍遅くなる**。

`-min_delay 0` を **`-i` より前**に付けて literal に読ませることで回避している
（エンジン側の `GifLoader` は遅延をそのまま使い、0 のときだけ 0.1 秒とする）。

スクリプトは変換前後の再生尺を出力し、15% 以上ずれたら警告を出すようにしてある。

### 対処 2: ロード中だけステージングを小分けに解放する（`UploadScope`）

`DirectXManager::UploadScope` という RAII クラスを追加。スコープ内では、貯まった
ステージングが **64MB**（`kUploadFlushBudget`）を超えそうになるたびに
`CommandContext::FlushAndWait()` → `ReleasePendingUploads()` を行う。

`FlushAndWait()` は元から実装されていたが**どこからも呼ばれていなかった**ものを活用している。

配線は `SceneManager::Update()` の 1 箇所だけで、**各シーンは何も呼ばなくてよい**。

```cpp
// Engine/Scene/SceneManager.cpp
{
    DirectXManager::UploadScope uploadScope(dxManager_);
    scene_->Initialize();
}
```

- しきい値判定は「今回のぶんを積む**前**」に行うため、ピークが「上限＋1枚」ではなく
  **「上限と 1 枚の大きいほう」**で収まる（実測 59MB ＝ LockOn アトラス 1 枚分ちょうど）。
- 入れ子対応（深さカウンタ）。一番外側を抜けるときに残りも解放する。
- `dxManager_` が `nullptr` でも安全に無効化される（従来どおりの挙動になるだけ）。
- スコープ外（ゲーム中のテクスチャ読み込みなど）は従来どおり `EndDraw` まで貯める。

> **重要な制約**: `FlushAndWait()` はコマンドリストを Close/Reset するため、
> 記録済みのステート（RTV・ディスクリプタヒープ・PSO など）が失われる。
> **描画中（`BeginDraw`〜`EndDraw` の間）にこのスコープを生存させてはいけない。**
> シーン初期化は `Draw()` より前に走るので現状は安全。

**効果: GameScene のステージング最大 258 MB → 59 MB**

### 対処 3: `ScratchImage` への全画素コピーを廃止

`DirectXManager::UploadTextureData()` に `DirectX::Image*` を取るオーバーロードを追加し、
`ScratchImage` 版はそこへ委譲する形にした。

`DirectX::Image` は `{width, height, format, rowPitch, slicePitch, pixels}` を持つだけの
**記述子構造体**なので、`TextureManager::LoadTextureFromMemory` は呼び出し元の `pixels` を
そのまま指させればよく、フルサイズの memcpy が 1 回まるごと消える。

**効果: GIF 1 本あたり最大 59MB の一時ピークが消滅**

---

## 4. ロード時間への影響

対処 2 はロード中に GPU 待ちを挟むため、ロードが遅くならないかを検証した。
しきい値を環境変数で切り替えて比較計測した結果：

| | しきい値 64MB（採用） | しきい値 ∞（フラッシュ無効） |
|---|---:|---:|
| ステージング最大 | 59 MB | 258 MB |
| フラッシュ回数 | 4 回 | 0 回 |
| **GameScene 初期化時間** | **3,803 ms** | **3,883 ms** |
| プロセスピーク RAM | 563 MB | 632 MB |

**ロード時間の悪化は無い**（差は計測誤差の範囲）。
このエンジンは元々毎フレーム `CommandContext::Begin()` で GPU 完了を待っており
（フレームをオーバーラップさせていない）、待ちが増えても失うものが少ない構造だった。

---

## 5. 変更ファイル

| ファイル | 内容 |
|---|---|
| `Resource/Images/Tutorial/*.gif` (6本) | 350×240 / 20fps へ再エンコード |
| `tools/shrink_tutorial_gifs.ps1` | **新規**。GIF 再変換スクリプト（再実行可能） |
| `Engine/Graphics/Device/DirectXManager.h/.cpp` | `UploadScope` / `FlushUploads()` / `UploadTextureData` オーバーロード追加 |
| `Engine/Scene/SceneManager.h/.cpp` | `SetDXManager()` 追加、`scene_->Initialize()` をスコープで包む |
| `Engine/Application/MyGameTitle.cpp` | `SceneManager::SetDXManager()` の配線 1 行 |
| `Engine/Graphics/Resource/TextureManager.cpp` | `LoadTextureFromMemory` の `ScratchImage` コピー廃止 |
| `Engine/Graphics/Rendering/PostEffect/BloomEffect.h/.cpp` | 別件のバグ修正（下記） |

### 別件で見つかったバグ（ついでに修正）

`BloomEffect` のコンストラクタが `Update()` を呼び、その中の `ImGui::Begin()` が
ImGui のフレーム外で実行されていたため、**Debug ビルドの起動時に必ず
`imgui.cpp` の `g.WithinFrameScope` アサートで停止していた**。

定数バッファへの書き込みだけを行う `ApplyParams()` を切り出し、コンストラクタからは
そちらを呼ぶよう修正した。

---

## 6. 残っている課題

対処後のピーク RAM は定常 346MB に対して約 570MB（+約 215MB）。
この残りは**もうステージングではなく**、以下の一時バッファが占めている。

1. `GifLoader` 自身の `atlas` ベクタ（59MB）— **ステージングバッファと同時に生存する**
2. assimp によるモデル読み込みの一時領域
3. ファイル版 `TextureManager::LoadTexture` の `ScratchImage`
   （対処 3 は `LoadTextureFromMemory` のみを直したので、こちらは残っている）

さらに削るなら:

- **`GifLoader` がアトラスをマップ済みステージングバッファへ直接書き込む** → 約 59MB 削減。
  ただし `GifLoader` に「アップロード用バッファをよこす」API を渡す必要があり、レイヤをまたぐ。
- **ステージングをリングバッファ化**して使い回す → ステージングメモリが定数になり、
  アロケーション自体も減るのでロードも速くなる。1 枚が上限を超える場合の分割処理が必要。
- **チュートリアル GIF の遅延ロード**（`TutorialSystem` の一括生成をやめる）→ VRAM をさらに削減。
  ただし `TextureManager` に解放機構が無いため、実際に VRAM を返すには破棄処理の追加が要る。
  現状 192MB なら急がない。

3,073MB あったピークが約 570MB まで落ちているため、当面はここで打ち切ってよい判断。

### 非同期（スレッド）ロードについて

「ロードを裏のスレッドで走らせればカクつきが消えるのでは」という案があるが、
このエンジンでは**素直にスレッドを生やすと壊れる**箇所がある。

- `DirectXManager::UploadTextureData` は**メインのコマンドリストに積む**。
  `ID3D12GraphicsCommandList` はスレッドセーフではない。
- `TextureManager` の `textureData_`（`unordered_map`）は排他なしで書かれる。
- `SrvManager::Allocate()` は `useIndex++` するだけでアトミックでもロックでもない。
- `ReleasePendingUploads()` がメインスレッドで走るため、ワーカーが積んだコピーの
  実行前にステージングを解放しかねない。

やるなら **CPU 側（ファイル I/O・WIC デコード・フレーム合成）だけをワーカーに出し、
GPU 側（リソース生成・SRV 確保・アップロード）はメインスレッドでフレーム分割**する形にする。
`GifLoader::Load` はこの分割にほぼ理想的な形をしており、GPU に触るのは
末尾の `LoadTextureFromMemory` 一行だけ。

より安価な代替として、**既存の `FadeTransition` の暗転中にロードを走らせる**だけでも
シーン入場時のカクつきは見えなくなる。

---

## 7. 補足: 動作確認の手順

exe は `generated/outputs/Debug/x64/` に出力されるが、そこにある `Resource/` は
**古い部分コピー**で新しいシェーダーもチュートリアル GIF も入っていない。
**作業ディレクトリはリポジトリルートにすること**（VS からの実行なら既定でそうなる）。
exe のあるフォルダから起動すると `ShaderCompiler.cpp` の
`assert(SUCCEEDED(hr))`（ファイルが見つからない）で停止する。

メモリ計測:

- システム RAM … `(Get-Process GuchisEngine).WorkingSet64` / `.PeakWorkingSet64`
- VRAM … パフォーマンスカウンタ `\GPU Process Memory(pid_<PID>*)\Dedicated Usage` の合計。
  D3D12 の DEFAULT ヒープ（テクスチャ等）はここに出る。`WorkingSet64` には出ないので注意。
