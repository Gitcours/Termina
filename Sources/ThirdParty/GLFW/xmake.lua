-- xmake.lua for GLFW
-- Translated from CMakeLists.txt

target("GLFW")
    set_kind("static")
    set_languages("c99")
    add_deps("Vulkan")

    -- Public include directory (glfw3.h, glfw3native.h live here)
    add_includedirs(".", {public = true})

    -- Compiler warnings
    if is_plat("windows") then
        add_cflags("/W3")
    else
        add_cflags("-Wall")
    end

    -- -------------------------------------------------------------------------
    -- Core sources (all platforms)
    -- -------------------------------------------------------------------------
    add_files(
        "context.c",
        "init.c",
        "input.c",
        "monitor.c",
        "platform.c",
        "vulkan.c",
        "window.c",
        "egl_context.c",
        "osmesa_context.c",
        "null_init.c",
        "null_monitor.c",
        "null_window.c",
        "null_joystick.c"
    )

    -- -------------------------------------------------------------------------
    -- Time / thread / module backend (shared per-OS, including null backend)
    -- -------------------------------------------------------------------------
    if is_plat("macosx") then
        add_files(
            "macos_time.c",
            "posix_module.c",
            "posix_thread.c"
        )
    elseif is_plat("windows") then
        add_files(
            "win32_time.c",
            "win32_thread.c",
            "win32_module.c"
        )
    else
        -- Linux / BSD / other Unix
        add_files(
            "posix_time.c",
            "posix_thread.c",
            "posix_module.c"
        )
    end

    -- -------------------------------------------------------------------------
    -- macOS / Cocoa backend
    -- -------------------------------------------------------------------------
    if is_plat("macosx") then
        add_defines("_GLFW_COCOA")
        add_files(
            "cocoa_init.m",
            "cocoa_joystick.m",
            "cocoa_monitor.m",
            "cocoa_window.m",
            "nsgl_context.m"
        )
        -- Disable ARC: GLFW uses manual retain/release in its ObjC code
        add_cxflags("-fno-objc-arc")
        
        -- Frameworks
        add_frameworks("Cocoa", "IOKit", "QuartzCore", "CoreFoundation")
    end

    -- -------------------------------------------------------------------------
    -- Windows / Win32 backend
    -- -------------------------------------------------------------------------
    if is_plat("windows") then
        add_defines("_GLFW_WIN32")
        add_defines("UNICODE", "_UNICODE")
        add_files(
            "win32_init.c",
            "win32_joystick.c",
            "win32_monitor.c",
            "win32_window.c",
            "wgl_context.c"
        )
        add_syslinks("gdi32", "user32", "shell32")

        -- Suppress CRT deprecation warnings from MSVC
        add_defines("_CRT_SECURE_NO_WARNINGS")
    end

    -- -------------------------------------------------------------------------
    -- Linux / Unix — X11 backend
    -- -------------------------------------------------------------------------
    if is_plat("linux") then
        add_defines("_GLFW_WAYLAND")
        add_files(
            "wl_init.c",
            "wl_monitor.c",
            "wl_window.c"
        )
        add_packages("wayland-client", "wayland-cursor", "wayland-egl", "xkbcommon")
    end

    -- -------------------------------------------------------------------------
    -- Linux joystick + posix_poll (X11 and/or Wayland on Linux)
    -- -------------------------------------------------------------------------
    if is_plat("linux") then
        add_defines("_DEFAULT_SOURCE")
        add_files(
            "linux_joystick.c",
            "posix_poll.c"
        )
        -- rt (clock_gettime on older glibc), m, dl
        add_syslinks("rt", "m", "dl")
    end

    -- -------------------------------------------------------------------------
    -- BSD / other Unix (non-Linux, non-macOS, non-Windows)
    -- Uses posix_poll but no linux_joystick
    -- -------------------------------------------------------------------------
    if not is_plat("macosx", "windows", "linux") then
        add_files("posix_poll.c")
        add_syslinks("m", "dl")
    end

    -- -------------------------------------------------------------------------
    -- Threads (pthreads on Unix / macOS; built-in on Windows)
    -- -------------------------------------------------------------------------
    if not is_plat("windows") then
        add_syslinks("pthread")
    end
