-- premake5.lua
project "udan_utils"
    kind "StaticLib"
    language "C++"
    cppdialect "C++17"
    staticruntime "off"

    files {
        "src/**.cpp",
        "include/udan/utils/**.h"
    }

    links { "udan_debug" }

    includedirs { 
        "include",
        "../udan_debug/include",
        "../SpdLog/include"
    }
    