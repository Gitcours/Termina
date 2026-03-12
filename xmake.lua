-- xmake.lua

add_rules("mode.debug", "mode.release")

-- Basics
set_languages("c++17")
set_rundir(".")
add_includedirs("Sources", "Sources/ThirdParty", { public = true })

-- Platform defines
if is_plat("windows") then
    add_defines("TRMN_WINDOWS", { public = true })
elseif is_plat("linux") then
    add_defines("TRMN_LINUX", { public = true })
    add_rpathdirs("Binaries/Linux", "$(builddir)/$(plat)/$(arch)/$(mode)/", { public = true })
    add_cflags("-fPIC", { public = true })
    add_cxxflags("-fPIC", { public = true })
elseif is_plat("macosx") then
    add_defines("TRMN_MACOS", { public = true })
    add_cflags("-x objective-c", "-fno-objc-arc", { public = true })
    add_cxxflags("-x objective-c++", "-fno-objc-arc", { public = true })
    add_mflags("-fno-objc-arc", { public = true })
    add_rpathdirs("Binaries/Mac", "$(builddir)/$(plat)/$(arch)/$(mode)/", { public = true })
    add_links("Binaries/Mac/libmetalirconverter.dylib", { public = true })
end

-- Other defines
add_defines("GLM_FORCE_DEPTH_ZERO_TO_ONE", "VK_NO_PROTOTYPES", "GLM_ENABLE_EXPERIMENTAL", { public = true })

-- Modes
if is_mode("debug") then
    add_defines("TRMN_DEBUG", { public = true })
    set_symbols("debug", { public = true })
    set_optimize("none", { public = true })
elseif is_mode("releasedbg") then
    add_defines("TRMN_RELEASE", { public = true })
    set_symbols("debug", { public = true })
    set_optimize("fastest", { public = true })
else
    add_defines("TRMN_RETAIL", { public = true })
    set_symbols("hidden", { public = true })
    set_optimize("fastest", { public = true })
    set_strip("all", { public = true })
end

-- Post link
after_link( function (target)
    if is_plat("windows") then
        os.cp("Binaries/Windows/*", "$(builddir)/$(plat)/$(arch)/$(mode)/")
    end
end)


includes("Sources")
