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
    targetdir "assets"
    objdir    ("%{wks.location}/obj/" .. outputdir .. "/%{prj.name}")
    debugdir  "assets"
    multiprocessorcompile "on"

    files {
        "src/**.h",
        "src/**.cpp",
    }

    filter "system:windows"
        files { "src/pc/rechan.rc" }
    filter {}

    includedirs {
        "src",
        "vendor/libp3d",
        "vendor/miniaudio",
    }

    links {
        "libp3d",
    }

    filter "system:windows"
        systemversion "latest"
        defines { "RC_PLATFORM_WINDOWS" }
        links { "opengl32" }

    filter "system:linux"
        defines { "PLATFORM_LINUX" }
        links { "GL", "X11", "pthread", "dl" }

    filter "configurations:Debug"
        runtime "Debug"
        symbols "on"
        defines { "DEBUG" }

    filter "configurations:Release"
        defines { "NDEBUG" }
        runtime "Release"
        optimize "on"
