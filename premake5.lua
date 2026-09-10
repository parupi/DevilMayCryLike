-- ============================================================================
-- .sln のヘッダを VS18 のものに差し替える
--
-- premake5 (5.0.0-beta5) の VS 系アクションは vs2022 が最新で、生成される .sln には
--   # Visual Studio Version 17
-- の1行しか入らない。VS18 が書く版数3行が毎回消えるため、premake を回すたびに
-- .sln が「変更あり」になり、そのままコミットすると共有ファイルが VS17 相当に巻き戻る。
--
-- 生成後に手で書き戻す運用だと忘れた時に壊れるので、ここで出力そのものを差し替える。
-- これで premake の .sln 出力はコミット済みのものと一致し、再生成しても差分が出ない。
--
-- ※ VisualStudioVersion は VS が保存するたびにパッチ版数まで書き換える。
--    VS 側が上げてきたらこの値も合わせること（差分が出たらそれが合図）
-- ============================================================================
local p = premake
require("vstudio")

p.override(p.vstudio.sln2005, "header", function(base, wks)
   p.w('Microsoft Visual Studio Solution File, Format Version 12.00')
   p.w('# Visual Studio Version 18')
   p.w('VisualStudioVersion = 18.8.12021.73 stable')
   p.w('MinimumVisualStudioVersion = 10.0.40219.1')
end)

workspace "GuchisEngine"
   startproject "GuchisEngine"
   configurations { "Debug", "Release" }
   platforms { "x64" }

   targetdir "../generated/outputs/%{cfg.buildcfg}/%{cfg.platform}"
   objdir "../generated/obj/%{prj.name}/%{cfg.buildcfg}"

-- DirectXTex は外部プロジェクトとしてリンクでOK
externalproject "DirectXTex"
   location "Externals/DirectXTex"
   filename "DirectXTex_Desktop_2022_Win10"
   -- 参照先の .vcxproj に書いてある本物の ProjectGuid。
   -- externalproject は .vcxproj を読まずプロジェクト名から GUID を作ってしまうので、
   -- 明示しないと .sln 側だけ別の GUID になり、VS が開くたびに照合と書き戻しが走る
   uuid "371B9FA9-4C90-4AC6-A123-ACED756D6C77"
   kind "StaticLib"
   language "C++"

project "GuchisEngine"
   kind "WindowedApp"
   language "C++"
   cppdialect "C++20"

   files { 
      "*.cpp",
      "*.h",
      "Engine/**.cpp",
      "Engine/**.h",
      "Engine/**.ipp",
      "App/**.cpp",
      "App/**.h",

      -- ImGui関係を自前で含める
      "Externals/imgui/*.cpp",
      "Externals/imgui/*.h",
      "Externals/imgui-node-editor/*.cpp",
      "Externals/imgui-node-editor/*.h",
   }

   includedirs { 
      "Engine",
      "Engine/Includes",
      "App",
      "Externals",
      "Externals/assimp/include",
      "Externals/imgui",
      "Externals/imgui-node-editor",
   }

   dependson { "DirectXTex" }

   links { 
      "DirectXTex",

      -- DirectX系
      "d3d12",
      "dxgi",
      "dxguid",
      "dxcompiler",
      "dinput8",
      "xinput",
   }

   warnings "Extra"
   buildoptions { "/utf-8" }
   flags { "MultiProcessorCompile" }

   -- vendored な外部ライブラリ(imgui / imgui-node-editor)は上流のコードなので手を入れない。
   -- 自前コードの警告0を維持するため、これらのファイルだけ警告を切る
   filter "files:Externals/**"
      warnings "Off"
   filter {}

   postbuildcommands {
      'copy "$(WindowsSdkDir)bin\\$(TargetPlatformVersion)\\x64\\dxcompiler.dll" "$(TargetDir)dxcompiler.dll"',
      'copy "$(WindowsSdkDir)bin\\$(TargetPlatformVersion)\\x64\\dxil.dll" "$(TargetDir)dxil.dll"'
   }

   filter "configurations:Debug"
      defines { "DEBUG" }
      symbols "On"
      staticruntime "On"
      libdirs { "Externals/assimp/lib/Debug" }
      links { "assimp-vc143-mtd" }

   filter "configurations:Release"
      defines { "NDEBUG" }
      optimize "On"
      staticruntime "On"
      libdirs { "Externals/assimp/lib/Release" }
      links { "assimp-vc143-mt" }
