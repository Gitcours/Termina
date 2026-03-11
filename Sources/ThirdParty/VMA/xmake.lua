-- xmake.lua

target("VMA")
    set_kind("static")
    add_files("*.cpp")
    add_headerfiles("*.h", "*.hpp")
    add_deps("Volk", "Vulkan")
