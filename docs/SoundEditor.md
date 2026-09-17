# SE 制作機能（Sound Editor）

`SEEditor.md` の設計書に沿って、エンジン内で効果音を合成・試聴・保存できるようにしたもの。
WAV 素材を用意しなくても、波形を組み合わせて SE を作れる。

**まず触るなら**: エディタの `Window > Engine > Sound Editor`。
初回に開くと `Resource/Sounds/` へプリセットが 14 個書き出されるので、どれか選んで「再生」。

---

## 全体の流れ

```
Sound Editor (ImGui)
        ↓  編集
SoundDefinition          ← Resource/Sounds/<名前>.sound
        ↓  SoundSynth::Render（オフライン合成）
AudioBuffer (float)
        ↓  ToPCM16
Audio::RegisterGeneratedSound
        ↓
SoundManager::PlaySE("名前")
```

**合成は一度きり**。ゲーム実行中に DSP を回すことはしないので、
ImGui スレッドから焼いても音が途切れないし、音声スレッドを別に立てる必要もない
（設計書 §6「リアルタイム処理とオフライン生成を分ける」）。

---

## ファイル

| ファイル | 役割 |
|---|---|
| `Engine/Audio/SE/AudioBuffer.{h,cpp}` | float の波形バッファ。PCM16 化と .wav 書き出し |
| `Engine/Audio/SE/SoundDSP.{h,cpp}` | 発振器・エンベロープ・フィルタ・歪み・ディレイ・リバーブ・FFT |
| `Engine/Audio/SE/SoundDefinition.{h,cpp}` | `.sound` のデータ構造と JSON 読み書き（`SoundFile` 名前空間） |
| `Engine/Audio/SE/SoundSynthesizer.{h,cpp}` | 定義 → 波形。処理順はここ1か所 |
| `Engine/Audio/SE/SoundPresets.{h,cpp}` | 組み込みプリセット16種 |
| `Engine/Audio/SE/SoundAssetLibrary.{h,cpp}` | `.sound` を焼いて `Audio` へ登録する層 |
| `Engine/Editor/Windows/SoundEditorWindow.{h,cpp}` | ImGui エディタ本体 |

---

## 音の構造

1つの SE は **レイヤー（`SELayer`）の重ね合わせ**。レイヤー1本ごとに:

```
発振器 → ピッチエンベロープ → 音量エンベロープ → 専用フィルタ → 定位・開始遅延
```

全レイヤーを混ぜたあと、マスター側で:

```
歪み → ディレイ → リバーブ → マスターフィルタ → 直流除去 → 正規化 → 音量
```

波形は Sine / Square / Triangle / Saw / WhiteNoise / PinkNoise。
フィルタは LowPass / HighPass / BandPass（RBJ の双2次）。

---

## 押さえておくこと

### ノイズにも「周波数」がある

白色ノイズを毎サンプル乱数で作ると **ピッチスイープが一切効かない**。
設計書の Dodge（Noise → Pitch Down）が作れないので、
指定の周波数で乱数を引き直し、その間を線形補間している（バリューノイズ）。
周波数がそのまま音の明るさになる。

**値を保持するだけ（サンプル&ホールド、sfxr 方式）では駄目だった**。
値が飛ぶ瞬間の段差が広帯域のパチッという音になり、周波数を下げても高域が残る。
実測で 8000Hz→250Hz と 32 倍動かしてもスペクトル重心が 6480→3366Hz しか下がらず、
ハイパスを通すと重心がほぼ動かなくなった。補間に変えて 3063→155Hz になった。

### 直流を抜かないと音量が損をする

デューティ比をずらした矩形波（Warning）や低域に寄せたノイズ（Explosion）は
波形が上下どちらかへ偏る。偏りは音として聞こえないのに振幅だけ食うので、
正規化するとそのぶん本体が小さくなる。`SoundSynth::Render` の仕上げで
20Hz の1次ハイパスを通している。

### `.sound` は `.wav` より優先される

`SoundManager::PlaySE("Dodge")` は

1. `Resource/Sounds/Dodge.sound`（あれば合成して鳴らす）
2. `Resource/sound/Dodge.wav`

の順に探す。エディタで作ったほうを優先したいため。

そのため **プリセットの一括書き出しは、同名の .wav があるものを飛ばす**
（`SwordSlash` / `SwordHit` が黙って差し替わるのを防ぐ）。
差し替えたいときは「プリセットから作る」で開いて自分で保存する。

### 生成した波形を作り直すときは先にボイスを止める

`Audio::RegisterGeneratedSound` は `std::vector<BYTE>` を作り直すので、
XAudio2 が再生中のバッファを指したままだと解放済みのメモリを読む。
内部で `StopVoicesUsing(name)` を呼んでいるが、**この仕組みを迂回して
`soundDataMap` を直接触らないこと**。

また、ボイスのスロットは使い回される。再生番号だけ持って後から止めると
別の音を止めてしまうので、`Audio::GetVoiceSoundName()` で中身を確かめる
（エディタの試聴がそうしている）。

### 焼き直しはスライダーを離した時だけ

長い SE（リバーブ付きで3秒近い）を毎フレーム焼くとフレームレートが落ちる。
`ImGui::IsAnyItemActive()` が false のときだけ焼いている。

---

## ゲームから鳴らす

```cpp
SoundManager::GetInstance().PlaySE("JustDodge");

// ピッチ・定位・優先度まで指定する
SEPlayParams params;
params.name = "SwordHit";
params.volume = 0.8f;
params.pitch = 1.1f;     // 0.25〜2.0
params.pan = -0.3f;
SoundManager::GetInstance().PlaySE(params);

// ワールド座標で鳴らす（距離減衰・パン・ドップラー）
SoundManager::GetInstance().PlaySE3D("Explosion", enemyPosition);
```

- 聞き手は `CameraManager::Update()` が毎フレーム渡している。ゲーム側は何もしなくてよい
- 距離の範囲は `SetSEDistanceRange(min, max)`。既定は 6m〜80m
- **距離に応じたリバーブの掛け分けは入れていない**。焼いた波形を差し替えることになるため
- 同時発音は 32 本まで。溢れたら優先度の低い音から止まる（`.sound` の `Priority`）
- 初回再生時に合成が走るので、間を空けたくないシーンでは
  `SoundManager::PreloadSE("名前")` を初期化で通しておく

---

## このゲームの SE 一覧（Sound.txt 対応）

音の定義は **`App/Audio/`** にある。Engine 側はゲームの音を知らない。

| ファイル | 中身 |
|---|---|
| `GameSoundLibrary.h` | ゲームから呼ぶ名前の定数（`GameSound::kDodge` など）。**呼ぶときは必ずこれを使う** |
| `GameSoundLibrary.cpp` | 名前・説明・作る関数の対応表と `Register()` / `ExportMissing()` |
| `GameSoundBuilders.h` | 定義を短く書くための部品（`Layer` / `Pitch` / `Env` / `Filt` / `FM`） |
| `GameSoundPlayer.cpp` | プレイヤー（移動・被弾・攻撃・手応え・ロックオン） |
| `GameSoundEnemy.cpp` | 骸骨とドラゴン |
| `GameSoundSystem.cpp` | 予兆・ランク・HPバー・UI・環境音 |
| `FootstepTracker.{h,cpp}` | 足音を刻むタイミング（プレイヤーと骸骨で共用） |

`AppEditor::Register()` が全部をエディタのプリセット一覧へ載せるので、
**Sound Editor で開いて保存すれば、コードを触らずに音を差し替えられる**
（保存した `.sound` のほうが優先される）。

### 音を変えたいとき

1. `Window > Engine > Sound Editor` を開く
2. 左の一覧から名前を選ぶ（例: `HitBone`）
3. いじって「保存」
4. ゲームを動かすと新しい音で鳴る

コード側の定義に戻したいときは `Resource/Sounds/<名前>.sound` を消して
エディタの「プリセットを全部書き出す」を押す。

### 鳴らしている場所

音の鳴らし方は3通りある。

- **ステートの `Enter` / 遷移**: `PlayerStateJump`、`BossStateRoar` など
- **攻撃パラメータに書く**: `MeleeAttackParams` / `BoneAttackParams` の
  `windupSound` / `swingSound`(`strikeSound`) にセットすると、
  コンポーネントが構え・振りの瞬間に鳴らす。**敵の攻撃音はここで足すのがいちばん楽**
- **状態を見て勝手に鳴る**: `Player::UpdateMovementSound`（足音・心音）、
  `BossAttackEffect::UpdateLocomotion`（羽ばたき・着地）

材質ごとの手応えは `PlayerWeapon::OnCollisionEnter` が `Enemy::GetHitMaterial()` を見て選ぶ。
敵を増やしたときは `GetHitMaterial` / `GetSpawnSound` / `GetDeathSound` を override すればよい。

### ループする音を足すときの決まり

`SEPlayParams::loop = true` で鳴らしたら、**戻り値を持って必ず `StopSE` する**。
今あるループは 溜め（`PlayerStateAttack`）/ 突進（`BossStateRush`）/ 炎（`BossStateBreath`）/
心音（`Player`）/ 松明（`WallTorch`）/ 環境音（`GameScene`）で、どれも中断経路（被弾・崩れ・
シーン遷移）から止めている。

位置つきのループは `UpdateSE3D` を毎フレーム呼ぶこと。`PlaySE3D` は鳴らした瞬間の位置でしか
計算しないので、呼ばないと横を通り過ぎても音が動かない。

ループ用の `.sound` は **リバーブを掛けない**（尾がレイヤーの長さより後ろへ伸びて、
継ぎ目に無音が挟まる）。詳しくは `GameSoundSystem.cpp` の `AmbienceDungeon` のコメント。

## 残っているもの

- **Phase 9 のノードベースエディタ**（設計書で★1・発展扱い）。
  `Externals/imgui-node-editor` は入っているので、やるならそこから
- LFO / ランダマイズ（同じ SE に毎回わずかな揺らぎを付ける）
- ドップラーは API はあるが、音源側の速度を渡している呼び出しがまだ無い
- **BGM とボイスはこの合成器では作れない**（Sound.txt の 7. と 8.）。
  ボス戦専用曲・ゲームオーバーのジングル・トレーニングの曲は音楽なので、
  素材を用意して `Resource/sound/*.wav` に置き、`SoundManager::PlayBGM` で差し替える。
  咆哮だけは「低い唸り＋濁った倍音＋歪み」で代用してある（`DragonRoar`）
- 足音は歩いた距離で刻んでいる。Animation ウィンドウで走りクリップに `footstep` の
  イベントを打てば、`FootstepTracker` が自動でそちら（＝足が着く瞬間）に切り替わる
