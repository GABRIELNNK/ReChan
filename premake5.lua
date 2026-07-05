workspace "rechan"
    architecture "x86_64"
    configurations { "Debug", "Release", "ReleaseASan" }
    location "build"
    startproject "rechan"

    filter "system:windows"
        toolset "v145"
    filter {}

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

    filter "configurations:Release*"
        defines { "NDEBUG" }
        runtime "Release"
        optimize "on"

    filter { "system:windows", "configurations:Release" }
        kind "WindowedApp"
        entrypoint "mainCRTStartup"

    -- ReleaseASan: optimized (keeps the layout-sensitive bug live) + AddressSanitizer.
    -- Kept as ConsoleApp so ASan diagnostics print to a console window.
    filter "configurations:ReleaseASan"
        symbols "on"
        targetname "rechan-asan"

    filter { "system:windows", "configurations:ReleaseASan" }
        editandcontinue "off"
        buildoptions { "/fsanitize=address" }
        -- libp3d is linked without ASan; disable STL container annotations so the
        -- instrumented rechan objects match (avoids LNK2038 annotate_* mismatches).
        defines { "_DISABLE_STL_ANNOTATION" }

    filter { "system:linux", "configurations:ReleaseASan" }
        buildoptions { "-fsanitize=address" }
        linkoptions { "-fsanitize=address" }

    filter {}
