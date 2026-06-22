workspace "rechan"
    architecture "x86_64"
    configurations { "Debug", "Release" }
    location "build"
    startproject "rechan"

outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"

include "vendor/libp3d"

-- rechan
project "rechan"
    kind "ConsoleApp"
    language "C++"
    cppdialect "C++20"
    staticruntime "on"
    targetdir "bin"
    objdir    ("%{wks.location}/obj/" .. outputdir .. "/%{prj.name}")
    debugdir  "bin"
    multiprocessorcompile "on"

    files {
        "src/**.h",
        "src/**.cpp",
        "vendor/cgltf/cgltf.h",
    }

    filter "system:windows"
        files { "src/pc/rechan.rc" }
    filter {}

    includedirs {
        "src",
        "vendor/libp3d",
        "vendor/libp3d/vendor/imgui",
        "vendor/miniaudio",
        "vendor/cgltf",
    }

    links {
        "libp3d",
    }
	defines { "_CRT_SECURE_NO_WARNINGS" }

    filter "system:windows"
        systemversion "latest"
        defines { "RC_PLATFORM_WINDOWS" }
        links {
            "opengl32",
            "dbghelp",
            "cfgmgr32",
            "imm32",
            "setupapi",
            "version",
            "winmm",
        }

    filter "system:linux"
        defines { "RC_PLATFORM_LINUX", "PLATFORM_LINUX" }
        linkoptions { "-rdynamic" }
        links {
            "GL", "X11", "Xcursor", "Xi", "Xinerama", "Xrandr",
            "SDL2", "pthread", "dl", "m",
        }

    filter "configurations:Debug"
        runtime "Debug"
        symbols "on"
        defines { "DEBUG" }

    filter "configurations:Release"
        defines { "NDEBUG" }
        runtime "Release"
        optimize "on"

    filter { "system:windows", "configurations:Release" }
        kind "WindowedApp"
        entrypoint "mainCRTStartup"

    filter {}
