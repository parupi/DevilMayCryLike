<#
.SYNOPSIS
  提出用フォルダ（提出用\exe と 提出用\project）をゼロから作り直す。

.DESCRIPTION
  tools\make_submission.bat をダブルクリックすれば、これが呼ばれて以下を全部やる。

    1. Release x64 をビルドする（MSBuild は vswhere で自動検出）
    2. 提出用\exe      … GuchisEngine.exe + dxcompiler.dll + dxil.dll + Resource
    3. 提出用\project  … App / Engine / Externals / Resource + sln / vcxproj 一式
    4. 最後にサイズと「大きいファイル上位10件」を出す

  2026-09-16 に提出用が 963MB まで膨らんだ原因は、手でコピーしたときに
  ビルド中間物とキャッシュが一緒に入ってしまったこと。このスクリプトは
  $script:ExcludeDirs / $script:ExcludeFiles で最初からそれらを外して作る。
  除外の内訳と理由:

    .vs          VS の IntelliSense キャッシュ。Browse.VC.db だけで 94MB、
                 リポジトリ直下の .vs\**\ipch には 1ファイル 300MB超の .ipch が並ぶ
    generated    ビルド出力。リポジトリでは repos\generated（リポジトリの外）に出るが、
                 Externals\generated に古い残骸（DirectXTex.pch 53MB）が残ることがある
    obj / ipch   ビルド中間物
    *-md.lib     premake5.lua が staticruntime "On" なので assimp は -mt / -mtd しか
    *-mdd.lib    リンクしない。-md / -mdd（計 78MB）は入れても一度も使われない

  注意: 提出用\project の .vcxproj は出力先が ..\generated\ なので、
  提出物を受け取った人がビルドすると 提出用\generated\ が project の外
  （提出用 直下）に 290MB できる。project の中を見ても見つからないので注意。

.PARAMETER OutDir
  出力先。既定はリポジトリ直下の「提出用」。

.PARAMETER Configuration
  ビルド構成。既定 Release。提出物に Debug を入れる理由は普通ない。

.PARAMETER Force
  出力先が既にあっても確認なしで作り直す。

.PARAMETER Zip
  作ったあと 提出用.zip も作る。

.EXAMPLE
  tools\make_submission.bat
  そのままダブルクリック。

.EXAMPLE
  pwsh -File tools\make_submission.ps1 -Force -Zip
  確認なしで作り直して zip まで作る。
#>
[CmdletBinding()]
param(
    [string]$OutDir,
    [ValidateSet('Release', 'Debug')]
    [string]$Configuration = 'Release',
    [switch]$Force,
    [switch]$Zip
)

$ErrorActionPreference = 'Stop'

# tools\ の親がリポジトリルート
$repoRoot = Split-Path -Parent $PSScriptRoot
if (-not $OutDir) { $OutDir = Join-Path $repoRoot '提出用' }

# 提出物に入れないもの。理由は .DESCRIPTION 参照
$script:ExcludeDirs = @('.vs', '.git', 'generated', 'obj', 'ipch')
$script:ExcludeFiles = @(
    'assimp-vc143-md.lib', 'assimp-vc143-mdd.lib',
    '*.pdb', '*.ilk', '*.iobj', '*.ipdb', '*.tlog'
)

# project\ に入れる、フォルダではない単体ファイル
$script:LooseFiles = @(
    'GuchisEngine.sln',
    'GuchisEngine.vcxproj',
    'GuchisEngine.vcxproj.filters',
    'GuchisEngine.vcxproj.user',
    'main.cpp',
    'imgui.ini',
    'premake5.lua',
    'premake.bat',
    'README.md'
)

function Write-Step {
    param([string]$Message)
    Write-Host ''
    Write-Host "== $Message" -ForegroundColor Cyan
}

function Find-MSBuild {
    # まず vswhere。VS の版が上がってもパスを直さなくて済む
    $vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
    if (Test-Path -LiteralPath $vswhere) {
        $found = & $vswhere -latest -products * `
            -requires Microsoft.Component.MSBuild `
            -find 'MSBuild\**\Bin\MSBuild.exe' 2>$null | Select-Object -First 1
        if ($found) { return $found }
    }
    # vswhere が無い / 見つからない場合の保険
    $fallback = 'C:\Program Files\Microsoft Visual Studio\18\Community\MSBuild\Current\Bin\MSBuild.exe'
    if (Test-Path -LiteralPath $fallback) { return $fallback }
    throw 'MSBuild.exe が見つかりません。Visual Studio の C++ ワークロードが入っているか確認してください。'
}

function Copy-Tree {
    param([string]$Source, [string]$Dest)

    $rcArgs = @($Source, $Dest, '/E', '/MT:8', '/R:2', '/W:1', '/NFL', '/NDL', '/NJH', '/NJS', '/NP')
    $rcArgs += '/XD'; $rcArgs += $script:ExcludeDirs
    $rcArgs += '/XF'; $rcArgs += $script:ExcludeFiles

    & robocopy @rcArgs | Out-Null
    # robocopy は 0-7 が正常、8 以上が失敗
    if ($LASTEXITCODE -ge 8) {
        throw "コピーに失敗しました: $Source -> $Dest (robocopy exit $LASTEXITCODE)"
    }
}

function Get-SizeMB {
    param([string]$Path)
    $sum = (Get-ChildItem -LiteralPath $Path -Recurse -File -Force -ErrorAction SilentlyContinue |
        Measure-Object -Property Length -Sum).Sum
    if (-not $sum) { return 0 }
    return [math]::Round($sum / 1MB, 1)
}

# ---------------------------------------------------------------- ビルド

Write-Step "$Configuration x64 をビルド"
$msbuild = Find-MSBuild
Write-Host "MSBuild: $msbuild"

$sln = Join-Path $repoRoot 'GuchisEngine.sln'
if (-not (Test-Path -LiteralPath $sln)) { throw "$sln が見つかりません。" }

& $msbuild $sln -p:Configuration=$Configuration -p:Platform=x64 -t:GuchisEngine -m -v:minimal -nologo
if ($LASTEXITCODE -ne 0) { throw "ビルドに失敗しました (exit $LASTEXITCODE)。提出用フォルダは作っていません。" }

# .vcxproj の targetdir は ..\generated\outputs\<構成>\<プラットフォーム> なので
# リポジトリの1つ上に出る
$buildDir = Join-Path (Split-Path -Parent $repoRoot) "generated\outputs\$Configuration\x64"
if (-not (Test-Path -LiteralPath (Join-Path $buildDir 'GuchisEngine.exe'))) {
    # 出力先が変わっていても拾えるように探す
    $genRoot = Join-Path (Split-Path -Parent $repoRoot) 'generated'
    $newest = Get-ChildItem -LiteralPath $genRoot -Recurse -Filter 'GuchisEngine.exe' -ErrorAction SilentlyContinue |
        Sort-Object LastWriteTime -Descending | Select-Object -First 1
    if (-not $newest) { throw "ビルドは通ったのに GuchisEngine.exe が見つかりません（$buildDir を探しました）。" }
    $buildDir = $newest.DirectoryName
}
Write-Host "ビルド出力: $buildDir"

# ---------------------------------------------------------------- 出力先を作り直す

if (Test-Path -LiteralPath $OutDir) {
    if (-not $Force) {
        Write-Host ''
        Write-Host "$OutDir は既にあります。中身を消して作り直します。" -ForegroundColor Yellow
        $answer = Read-Host '続けますか? [y/N]'
        if ($answer -notmatch '^[yY]') {
            Write-Host '中止しました。'
            exit 1
        }
    }
    Write-Step '既存の出力先を削除'
    Remove-Item -LiteralPath $OutDir -Recurse -Force
}

# ---------------------------------------------------------------- 提出用\exe

Write-Step '提出用\exe を作成'
$exeOut = Join-Path $OutDir 'exe'
New-Item -ItemType Directory -Path $exeOut -Force | Out-Null

# dxcompiler / dxil はシェーダを実行時にコンパイルするので exe の隣に必須
foreach ($name in @('GuchisEngine.exe', 'dxcompiler.dll', 'dxil.dll')) {
    $src = Join-Path $buildDir $name
    if (-not (Test-Path -LiteralPath $src)) { throw "$name が $buildDir にありません。" }
    Copy-Item -LiteralPath $src -Destination (Join-Path $exeOut $name)
    Write-Host "  $name"
}
Copy-Tree (Join-Path $repoRoot 'Resource') (Join-Path $exeOut 'Resource')
Write-Host '  Resource'

# ---------------------------------------------------------------- 提出用\project

Write-Step '提出用\project を作成'
$projOut = Join-Path $OutDir 'project'
New-Item -ItemType Directory -Path $projOut -Force | Out-Null

foreach ($dir in @('App', 'Engine', 'Externals', 'Resource')) {
    $src = Join-Path $repoRoot $dir
    if (-not (Test-Path -LiteralPath $src)) { throw "$src が見つかりません。" }
    Copy-Tree $src (Join-Path $projOut $dir)
    Write-Host "  $dir"
}

foreach ($file in $script:LooseFiles) {
    $src = Join-Path $repoRoot $file
    if (Test-Path -LiteralPath $src) {
        Copy-Item -LiteralPath $src -Destination (Join-Path $projOut $file)
        Write-Host "  $file"
    }
    else {
        Write-Host "  (なし) $file" -ForegroundColor DarkGray
    }
}

# ---------------------------------------------------------------- zip

if ($Zip) {
    Write-Step 'zip を作成（サイズによっては数分かかる）'
    $zipPath = "$OutDir.zip"
    if (Test-Path -LiteralPath $zipPath) { Remove-Item -LiteralPath $zipPath -Force }
    Compress-Archive -Path (Join-Path $OutDir '*') -DestinationPath $zipPath -CompressionLevel Optimal
}

# ---------------------------------------------------------------- 結果

Write-Step '完成'
$exeMB = Get-SizeMB $exeOut
$projMB = Get-SizeMB $projOut
Write-Host ("  提出用\exe      {0,8:N1} MB" -f $exeMB)
Write-Host ("  提出用\project  {0,8:N1} MB" -f $projMB)
Write-Host ("  合計            {0,8:N1} MB" -f ($exeMB + $projMB))
if ($Zip) {
    $zipMB = [math]::Round((Get-Item -LiteralPath "$OutDir.zip").Length / 1MB, 1)
    Write-Host ("  提出用.zip      {0,8:N1} MB" -f $zipMB)
}

# 次に膨らんだときに原因をすぐ見つけられるように
Write-Step '大きいファイル上位10件'
Get-ChildItem -LiteralPath $OutDir -Recurse -File -Force |
    Sort-Object Length -Descending | Select-Object -First 10 |
    ForEach-Object {
        $rel = $_.FullName.Substring($OutDir.Length).TrimStart('\')
        Write-Host ("  {0,8:N1} MB  {1}" -f ($_.Length / 1MB), $rel)
    }

Write-Host ''
Write-Host "出力先: $OutDir"
Write-Host 'なお 提出用\project をビルドすると、中間物は project の外（提出用 直下）に generated\ として出ます。' -ForegroundColor DarkGray
