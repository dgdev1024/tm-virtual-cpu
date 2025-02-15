-- @file premake5.lua

-- Workspace Settings
workspace "project-tm"
    language "C"
    cdialect "C17"
    location "./generated"
    configurations { "debug", "release", "distribute" }
    filter { "configurations:debug" }
        defines { "TM_DEBUG" }
        symbols "On"
    filter { "configurations:release" }
        defines { "TM_RELEASE" }
        optimize "On"
    filter { "configurations:distribute" }
        defines { "TM_DISTRIBUTE" }
        optimize "On"
    filter { "system:linux" }
        defines { "TM_LINUX" }
        cdialect "gnu17"
    filter {}

    -- TM Virtual CPU Backend Library
    project "tm"
        kind "SharedLib"
        location "./generated/tm"
        targetdir "./build/bin/tm/%{cfg.buildcfg}"
        objdir "./build/obj/tm/%{cfg.buildcfg}"
        includedirs {
            "./projects/tm/include"
        }
        files {
            "./projects/tm/src/tm.*.c"
        }
