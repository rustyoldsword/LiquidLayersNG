set_xmakever("3.0.0")

includes("lib/CommonLibSSE-NG")

set_project("LiquidLayersNG")
set_version("1.0.0")
set_license("MIT")
set_languages("c++23")
set_warnings("allextra")
set_encodings("utf-8")

add_rules("mode.debug", "mode.releasedbg")

set_runtimes("MD")
set_allowedplats("windows")
set_allowedarchs("x64")
set_defaultplat("windows")
set_defaultarchs("x64")

if is_plat("windows") then
    add_defines("_SILENCE_ALL_MS_EXT_DEPRECATION_WARNINGS", "WIN32_LEAN_AND_MEAN", "NOMINMAX", "NOGDI", "UNICODE", "_UNICODE")
    add_cxxflags("cl::/permissive-", "cl::/Zc:preprocessor", "cl::/EHsc", "cl::/utf-8")
    add_cxxflags("cl::/wd4068", "cl::/wd4201", "cl::/wd4251", "cl::/wd4275", "cl::/wd4267", "cl::/wd4244", "cl::/wd4996", "cl::/wd4100")
    -- keep build paths out of the binary (logger source locations, PDB path)
    add_cxxflags("cl::/d1trimfile:" .. os.projectdir() .. "\\", {force = true})
    add_shflags("/PDBALTPATH:%_PDB%", {force = true})
end

target("LiquidLayersNG", function()
    add_rules("commonlibsse-ng.plugin", {
        name = "LiquidLayersNG",
        description = "Animated creampie and liquid layers on SexLab P+ climaxes, timed and washed off natively"
    })

    add_files("src/**.cpp")
    add_headerfiles("src/**.h")
    add_includedirs("src")
    add_includedirs("lib/SKSE-Menu-Framework-3-API")
    set_pcxxheader("src/PCH.h")
end)
