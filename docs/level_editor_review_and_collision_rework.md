# レベルエディタ評価 & 回転反映・OBBめり込み排斥 対応まとめ

調査日: 2026-07-08
対象: `Externals/level_editor`(Blenderアドオン) / `App/Stage/SceneBuilder.cpp` / `Engine/World3D/Collider/*`

---

## Part 1: レベルエディタ評価で見つかった課題

`Externals/level_editor`(Blenderアドオン)が出力するJSONと、それを読み込む`App/Stage/SceneLoader.cpp`・`SceneBuilder.cpp`を突き合わせた結果、**アドオンが出力しているのにゲーム側で握りつぶされている/正しく処理されていない項目**が多数見つかった。

### 1-1. 出力とゲーム側の不整合

| # | 内容 | 詳細 | 状態(このセッション時点) |
|---|---|---|---|
| 1 | 回転が無視される | `SceneBuilder::ApplyTransform`が`translate`/`scale`のみコピーし`rotate`を無視 | **✅ Part 2で対応** |
| 2 | 親子階層(`children`)が無視される | アドオンは再帰的に`children`を出力するが、`SceneLoader::Load`は`root["objects"]`直下しか読まない。`App/`内に`"children"`参照は0件 | 未対応 |
| 3 | コライダーのY/Z軸入れ替え漏れ | `ApplyTransform`はtranslate/scaleをY/Z入れ替えするが、`ApplyCollider`のcenter/sizeは未対応だった | **✅ Part 2で対応** |
| 4 | `Event_ForceBattle`が未実装 | アドオンのメニュー・エクスポート両方に存在するが、`EventFactory::Create`が`"Event_EnemySpawn"`/`"Event_Clear"`しか認識せず、`nullptr`→`BuildEvent`で早期returnされ何も起きない | 未対応 |
| 5 | クリア条件が`DEFEAT_ENEMIES`以外未実装 | アドオンUIで`ALL_ENEMIES_DEFEATED`/`REACH_OBJECT`/`TIMER`/`DEFEAT_ENEMIES`を選択できるが、ゲーム側は`DEFEAT_ENEMIES`のみ処理 | 未対応 |
| 6 | `file_name`・`event.trigger`が未消費 | `SceneLoader`でパースされ構造体に格納されるが、`SceneBuilder.cpp`のどこからも参照されない | 未対応 |

### 1-2. アドオン単体の問題

| # | 内容 | 詳細 |
|---|---|---|
| 1 | モデルパスのハードコード（重大・移植性） | `menus/topbar_menu.py:37`が`C:\Program Files\Blender Foundation\Blender 4.5\...`という開発者PC固有の絶対パスを参照。他PC/別バージョンBlenderでは`FileNotFoundError`になる。同梱の`models/`フォルダは使われていない |
| 2 | `class_name`/`file_name`編集UIが実質存在しない | `panels/class_name_panel.py`・`file_name_panel.py`・`collider_panel.py`・`disabled.py`が`__init__.py`の`classes`タプルに未登録で完全にデッドコード。表示される`OBJECT_PT_level_editor`はCollider/Disabled欄しか出さない |
| 3 | `disabled.py`が壊れている | 存在しないプロパティ`myaddon_disabled`（実際は`disabled`）と存在しないオペレーター`myaddon.toggle_object_disabled`を参照。デッドなので実害なしだが登録すると即エラー |
| 4 | `operators/load_model.py`が未登録のデッドコード | `__init__.py`でimportがコメントアウト済みだが、`panels/level_editor_panel.py`は未使用のままこのクラスをimportし続けている |
| 5 | 親が`disabled`だと子も出力から丸ごと消える | `parse_scene_recursive_json`が`disabled`判定で即returnするため子の再帰処理がスキップされる。ドキュメント化されておらず事故りやすい（現状は課題2により実害は顕在化していない） |

---

## Part 2: 今回実施した変更

ユーザー指示: 「回転の反映」「コライダーのY/Z軸修正」の2点を対応し、それに合わせてエンジン側のコライダーをOBB(有向境界ボックス)化、既存のめり込み排斥コードをエンジン側に集約。

### 変更1: 回転の適用 (`App/Stage/SceneBuilder.cpp`)

`ApplyTransform`で、JSONの`rotation`(度数法・Blenderローカルオイラー角XYZ順)をtranslate/scaleと同じY/Z入れ替えを行った上で`EulerDegree()`(`Engine/Math/Quaternion.h`)によりQuaternionへ変換し、`WorldTransform::GetRotation()`へ反映するようにした。

```cpp
Vector3 rotate = src.rotate;
std::swap(rotate.y, rotate.z);
transform->GetRotation() = EulerDegree(rotate);
```

**符号/ハンドネスは静的解析だけでは断定できないため未検証。** translate/scaleと同じ「Y/Z単純入れ替え」パターンを踏襲したが、実機でBlender側の既知角度（例: Y軸90度）とゲーム内の向きを見比べて、ズレていれば`rotate.y`/`rotate.z`いずれかの符号反転を追加する必要がある。

### 変更2: コライダーのY/Z軸入れ替え修正 (`App/Stage/SceneBuilder.cpp`)

`ApplyCollider`で、BOXの`offsetMin`/`offsetMax`とSphereの`offset`に、スケール乗算の**前**にY/Z入れ替えを追加した。

### 変更3: OBBコライダー化 + めり込み排斥のエンジン移行

回転が反映されるようになったため、回転を無視する`AABBCollider`のままではGround等の回転オブジェクトのコライダーが正しく機能しない。既に実装済みだが`App/`側で未使用だった`OBBCollider`/`OBBData`(`Engine/World3D/Collider/OBBCollider.h/.cpp`)を実戦投入する形で対応した。

- **`ApplyCollider`のBOXケースを`OBBCollider`生成に変更**（Sphereは変更なし）。Player/Enemy/BossKnight等の自身のコライダー・武器コライダーはコード側で個別生成しておりJSON経由ではないため影響なし。
- **`PenetrationResult`構造体を追加** (`Engine/World3D/Collider/ColliderStructs.h`): `{ bool hit; Vector3 normal; float depth; }`
- **`CollisionManager::CalculatePenetration(mover, blocker)`を追加** (`Engine/World3D/Collider/CollisionManager.h/.cpp`): AABB-AABB / OBB-OBB / OBB-AABB の全組み合わせを、共通のprivateヘルパー`CalculateBoxPenetration`（15軸SAT: 両者の3軸+9本のクロス積によるMTV計算）に正規化して渡す形で一本化。
- **`AABBCollider::CalculateCollisionNormal`を削除**。旧実装は「符号付き単位法線」しか返さず、呼び出し側(`Player.cpp`/`Enemy.cpp`)が手書きでmin/maxスナップしていた（かつ押し出し距離の計算が全軸でX方向のサイズしか使っていないバグを内包していた）。
- **`Player`/`Enemy`の`OnCollisionEnter`/`OnCollisionStay`を書き換え**: 4箇所に重複していたほぼ同一の押し出し処理を、各クラスの`ResolveGroundCollision`という共通privateメソッドに統合し、`CalculatePenetration`の結果(`normal * depth`)をtranslationに加算する形に変更。MTVベースになったことで前述のX軸依存バグも解消された。

#### 変更ファイル一覧
- `App/Stage/SceneBuilder.cpp`
- `Engine/World3D/Collider/ColliderStructs.h`
- `Engine/World3D/Collider/CollisionManager.h/.cpp`
- `Engine/World3D/Collider/AABBCollider.h/.cpp`
- `App/GameObject/Character/Player/Player.h/.cpp`
- `App/GameObject/Character/Enemy/Enemy.h/.cpp`

#### ビルド確認
MSBuildでDebug|x64ビルドが通ることを確認済み（`GuchisEngine.exe`生成成功）。**画面描画・実プレイでの動作確認はこの環境から実施できないため未検証。**

---

## Part 3: 未解決の課題 & 今後の進め方

優先順位の目安と、それぞれの対応理由・依存関係。

| 優先度 | 課題 | 理由 |
|---|---|---|
| 1 | **今回の変更の実機検証** | 回転の符号/ハンドネス、回転したGroundでのOBB押し出し挙動、既存の回転なしシーンでの回帰の有無を確認しないと、他の変更の土台として信頼できない |
| 2 | `children`階層をSceneLoaderで再帰的に読む | アドオンは階層構造を前提に設計されており(親のdisabledで子ごと隠す等)、対応しない限りペアレント機能自体が無意味。実装コストも小さい(`SceneLoader::ParseObject`に再帰呼び出しを追加するだけ) |
| 3 | `Event_ForceBattle`の実装 | `EventFactory`に1エントリ追加すれば動く見込みで対応コストが低く、レベルデザイナーがメニューから作れるのに機能しないという分かりやすいギャップを埋められる |
| 4 | クリア条件`ALL_ENEMIES_DEFEATED`/`REACH_OBJECT`/`TIMER`の実装 | `ClearEvent`側の判定ロジック追加が必要でForceBattleよりやや大きい。UIで選べるのに動かない項目を減らす目的 |
| 5 | `file_name`・`event.trigger`の配線、またはアドオン側からの削除 | 使われる予定がないなら、アドオンのUI/エクスポートから削除して「設定しても意味がない項目」を減らす方が実害が小さい。ゲーム側で使う予定があるなら要件を先に固める |
| 6 | アドオン側のデッドコード整理・パス修正 | `topbar_menu.py`のモデル絶対パスをリポジトリ相対に修正、未登録の`panels/*.py`・`load_model.py`を削除または実際に配線。緊急度は低いが、他の開発者がアドオンを使い始めた瞬間に踏むバグ(絶対パス)は早めが望ましい |

### 進め方の推奨
1. まず優先度1(実機検証)を必ず先に行う。回転の符号が逆だった場合、それ以降のコライダー関連修正のテストが無意味になるため。
2. 優先度2(`children`対応)は独立して着手可能で、レベルエディタの「親子関係で隠す/まとめる」という設計意図を活かすための前提になる。
3. 優先度3・4(イベント実装)はゲームデザイン側の要件次第で実装内容が変わるため、着手前に「ForceBattleの戦闘トリガー条件」「クリア条件の各挙動」の仕様を簡単にでも書き出しておくと手戻りが少ない。
4. 優先度5・6は独立して並行に進めて良い（他の変更をブロックしない）。

---

## Part 4: コライダーサイズ不整合の修正

調査日: 2026-07-09
対応: Part 3優先度1「実機検証」で発覚した、BOXコライダー(OBBCollider)の実寸不一致。

### 症状
実機で確認したところ、レベルエディタ経由で配置したBOXコライダーが実際のオブジェクトサイズと一致しなかった(当初は大きすぎ、修正の過程で今度は小さすぎる状態を経由)。

### 原因と対応

1. **`ApplyCollider`でのスケール二重適用** (`App/Stage/SceneBuilder.cpp`)
   `SceneLoader::ParseCollider`が既に正しいローカル空間の`offsetMin`/`offsetMax`を計算済みにもかかわらず、`ApplyCollider`側で追加に`* scale * 2.0f`を掛けていた。一方`OBBCollider::Update()`はオーナーの`WorldTransform`のスケールを毎フレーム自動抽出して`halfExtents`に適用する実装だったため、スケールが二重適用され、サイズが本来より大きく膨張していた。
   → `ApplyCollider`から手動の`* scale`を削除し、スケール適用は`OBBCollider`側の自動処理に一本化。

2. **Player/敵キャラクター自コライダーの型不整合** (`Player.cpp` / `GruntMelee.cpp` / `Hellkaina.cpp` / `BossKnight.cpp`)
   各`Initialize()`で、JSON由来の自コライダー(実体は`OBBCollider`)を`static_cast<AABBCollider*>`という誤った型でキャストし、`GetColliderData()`で無関係なメモリを`AABBData`として読み書きしていた。Part 2で「BOXコライダー生成をAABBCollider→OBBColliderに変更」した際に、この4箇所のキャストが追従しておらず未定義動作になっていた(Groundはこの種のキャストをしておらず無関係だったため影響なし)。
   → 4クラスとも`static_cast<OBBCollider*>`+`OBBData::halfExtents`を使うよう修正。既存の意図的な当たり判定縮小(`* 0.5f`/BossKnightのみ`* 0.65f`)はそのまま維持。

3. **Blender側とエンジン側のコライダー単位のズレ** (`App/Stage/SceneBuilder.cpp`)
   上記2点の修正後、実機検証でGround/ゲームシーン両方のコライダーが実オブジェクトのちょうど半分のサイズになることが判明。原因を切り分けたところ、Titleシーンのみ正しく見えていたのは、そのシーンのコライダー`size`値がたまたま2倍で設定されていたためと判明し、Blenderアドオン側のcollider_size単位とエンジン側が期待する単位に定数2倍のズレがあることが分かった。
   → `ApplyCollider`のBOXケースで、`offsetMin`/`offsetMax`(Y/Z軸入れ替え後)を`* 2.0f`する補正を追加。

### 結果
Ground(地形)・Player・敵キャラクター(GruntMelee/Hellkaina/BossKnight)いずれも、コライダーサイズが実際のオブジェクトと一致することを実機で確認済み。Part 3優先度1「実機検証」のうち、コライダーサイズに関する部分はこれで解消。回転の符号/ハンドネスについては、今回の検証で明確な不具合報告は出なかったが、独立した観点での確認は未実施。

#### 変更ファイル一覧
- `App/Stage/SceneBuilder.cpp`
- `App/GameObject/Character/Player/Player.cpp`
- `App/GameObject/Character/Enemy/GruntMelee/GruntMelee.cpp`
- `App/GameObject/Character/Enemy/Hellkaina/Hellkaina.cpp`
- `App/GameObject/Character/Enemy/BossKnight/BossKnight.cpp`
