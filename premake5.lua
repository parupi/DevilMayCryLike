workspace "GuchisEngin"
   startproject "GuchisEngin"
   configurations { "Debug", "Release" }
   platforms { "x64" }

   targetdir "../generated/outputs/%{cfg.buildcfg}/%{cfg.platform}"
   objdir "../generated/obj/%{prj.name}/%{cfg.buildcfg}"

-- DirectXTex は外部プロジェクトとしてリンクでOK
externalproject "DirectXTex"
   location "Externals/DirectXTex"
   filename "DirectXTex_Desktop_2022_Win10"
   kind "StaticLib"
   language "C++"

project "GuchisEngin"
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
