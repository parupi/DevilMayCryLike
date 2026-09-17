[![DebugBuild](https://github.com/parupi/DevilMayCryLike/actions/workflows/DebugBuild.yml/badge.svg)](https://github.com/parupi/DevilMayCryLike/actions/workflows/DebugBuild.yml)

[![ReleaseBuild](https://github.com/parupi/DevilMayCryLike/actions/workflows/ReleaseBuild.yml/badge.svg)](https://github.com/parupi/DevilMayCryLike/actions/workflows/ReleaseBuild.yml)

# GuchisEngine / Eclipser

DirectX12 の自作エンジン（`Engine/`）と、その上で動く DMC 風アクションゲーム（`App/`）。

## ビルドと実行

```
premake5 vs2022          # 新しいファイルを足したら必ず実行（.vcxproj はグロブ収集）
```

`GuchisEngine.sln` をビルドし、**作業ディレクトリをリポジトリルートにして**実行する。
exe と同じ場所にある `Resource/` は古い部分コピーなので、そこから起動するとシェーダーが見つからず止まる。

## ディレクトリ

| パス | 中身 |
|---|---|
| `Engine/` | エンジン本体（描画・入力・音・シーン・数学） |
| `Engine/Editor/` | エディタ基盤とエンジン機能のウィンドウ（`_DEBUG` のみ） |
| `App/` | ゲーム本体 |
| `App/Editor/` | ゲーム固有のエディタウィンドウ（`_DEBUG` のみ） |
| `Resource/` | モデル・テクスチャ・音・シェーダー・調整値のJSON |
| `docs/` | 設計ドキュメント |

## エディタ

Debug ビルドで起動すると ImGui のエディタが立ち上がる。ゲーム画面は「Game」ウィンドウの中に出る。

- **Hierarchy / Inspector** … シーン内の Object3d を一覧・生成・編集・削除
- **Asset Browser** … モデルとテクスチャの一覧、その場配置
- **Light / Render / Camera / Audio / Profiler** … 各エンジン機能
- **Player / Enemy / Attack Editor / GameCamera / Stylish / HitEffect** … ゲーム固有

Windowメニューは「エンジン」と「ゲーム」に分かれている。Ctrl+P で名前検索。
Layoutメニューに作業内容別のプリセットがある。配置が崩れたら「配置をリセット」。

**新しいエディタウィンドウの足し方**と依存関係のルールは
[`docs/EditorArchitecture.md`](docs/EditorArchitecture.md) を参照。

## ドキュメント

| ファイル | 内容 |
|---|---|
| [`docs/EditorArchitecture.md`](docs/EditorArchitecture.md) | エディタの構成と拡張のしかた |
| [`docs/EditorArchitecture_Plan.md`](docs/EditorArchitecture_Plan.md) | そこへ至った再構成の記録 |
| [`docs/memory_optimization.md`](docs/memory_optimization.md) | メモリ・VRAM の削減メモ |
