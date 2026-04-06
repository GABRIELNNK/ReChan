local libp3d_root = path.getdirectory(_SCRIPT)

-- libp3d (includes glad + glfw)
project "libp3d"
    kind "StaticLib"
    language "C++"
    cppdialect "C++20"
    staticruntime "on"
    targetdir ("%{wks.location}/bin/" .. outputdir .. "/%{prj.name}")
    objdir    ("%{wks.location}/obj/" .. outputdir .. "/%{prj.name}")

    files {
        libp3d_root .. "/**.h",
        libp3d_root .. "/**.hpp",
        libp3d_root .. "/**.cpp",

        -- glad
        libp3d_root .. "/vendor/glad/src/gl.c",

        -- glfw
        libp3d_root .. "/vendor/glfw/src/context.c",
        libp3d_root .. "/vendor/glfw/src/init.c",
        libp3d_root .. "/vendor/glfw/src/input.c",
        libp3d_root .. "/vendor/glfw/src/monitor.c",
        libp3d_root .. "/vendor/glfw/src/null_init.c",
        libp3d_root .. "/vendor/glfw/src/null_joystick.c",
        libp3d_root .. "/vendor/glfw/src/null_monitor.c",
        libp3d_root .. "/vendor/glfw/src/null_window.c",
        libp3d_root .. "/vendor/glfw/src/platform.c",
        libp3d_root .. "/vendor/glfw/src/vulkan.c",
        libp3d_root .. "/vendor/glfw/src/window.c",
    }

    includedirs {
        libp3d_root,
        libp3d_root .. "/vendor/glad/include",
        libp3d_root .. "/vendor/glfw/include",
    }

    filter "system:windows"
        systemversion "latest"
        files {
            libp3d_root .. "/vendor/glfw/src/win32_init.c",
            libp3d_root .. "/vendor/glfw/src/win32_joystick.c",
            libp3d_root .. "/vendor/glfw/src/win32_module.c",
            libp3d_root .. "/vendor/glfw/src/win32_monitor.c",
            libp3d_root .. "/vendor/glfw/src/win32_thread.c",
            libp3d_root .. "/vendor/glfw/src/win32_time.c",
            libp3d_root .. "/vendor/glfw/src/win32_window.c",
            libp3d_root .. "/vendor/glfw/src/wgl_context.c",
            libp3d_root .. "/vendor/glfw/src/egl_context.c",
            libp3d_root .. "/vendor/glfw/src/osmesa_context.c",
        }
        defines { "_GLFW_WIN32" }

    filter "configurations:Debug"
        runtime "Debug"
        symbols "on"

    filter "configurations:Release"
        runtime "Release"
        optimize "on"
