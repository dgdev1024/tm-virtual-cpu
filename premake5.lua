-- @file premake5.lua

-- Workspace Settings
workspace "project-tm"
    language "C"
    cdialect "C17"
    flags { "FatalWarnings" }
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

    -- TM Virtual CPU Assembler (tmm)
    project "tmm"
        kind "ConsoleApp"
        location "./generated/tmm"
        targetdir "./build/bin/tmm/%{cfg.buildcfg}"
        objdir "./build/obj/tmm/%{cfg.buildcfg}"
        includedirs {
            "./projects/tm/include",
            "./projects/tmm/include"
        }
        files {
            "./projects/tmm/src/tmm.*.c"
        }
        libdirs {
            "./build/bin/tm/%{cfg.buildcfg}"
        }
        links {
            "tm", "m"
        }

    -- TM Virtual CPU Linker (tml)
    project "tml"
        kind "ConsoleApp"
        location "./generated/tml"
        targetdir "./build/bin/tml/%{cfg.buildcfg}"
        objdir "./build/obj/tml/%{cfg.buildcfg}"
        includedirs {
            "./projects/tm/include",
            "./projects/tml/include"
        }
        files {
            "./projects/tml/src/tml.*.c"
        }
        libdirs {
            "./build/bin/tm/%{cfg.buildcfg}"
        }
        links {
            "tm", "m"
        }

    -- TM Virtual CPU Unit Tests (tmtest)
    project "tmtest"
        kind "ConsoleApp"
        location "./generated/tmtest"
        targetdir "./build/bin/tmtest/%{cfg.buildcfg}"
        objdir "./build/obj/tmtest/%{cfg.buildcfg}"
        includedirs {
            "./projects/tm/include",
            "./projects/tmtest/include"
        }
        files {
            "./projects/tmtest/src/tmtest.*.c"
        }
        libdirs {
            "./build/bin/tm/%{cfg.buildcfg}"
        }
        links {
            "tm", "m"
        }
        