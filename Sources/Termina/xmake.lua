-- xmake.lua

target("Termina")
    set_kind("shared")
    set_group("Termina")

    add_files("**.cpp")
    if is_plat("macosx") then
        add_files("**.mm")
    end

    add_headerfiles("**.hpp")
    add_includedirs(".")
    add_deps(
        "CGLTF",
        "GLFW",
        "GLM",
        "ImGui",
        "Jolt",
        "JSON",
        "MikkTSpace",
        "MiniAudio",
        "stb",
        "VMA",
        "Volk",
        "Vulkan"
    )

    if is_plat("macosx") then
        add_deps("MetalShaderConverter")
    end
