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

    files {
        "src/**.h",
        "src/**.cpp",
    }

    includedirs {
        "src",
        "vendor/libp3d",
    }

    links {
        "libp3d",
        "glad",
        "glfw",
        "opengl32",
    }

    filter "system:windows"
        systemversion "latest"
        defines { "RC_PLATFORM_WINDOWS" }

    filter "configurations:Debug"
        runtime "Debug"
        symbols "on"
        defines { "RC_DEBUG" }

    filter "configurations:Release"
        runtime "Release"
        optimize "on"
        defines { "RC_RELEASE" }
