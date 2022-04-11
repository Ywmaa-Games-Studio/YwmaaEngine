workspace "AMRT"
    architecture "x64"
    startproject "Sandbox"

    configurations
    {
        "Debug",
        "Release",
        "Dist"
    }

outputdir = "%{cfg.buildcfg}-%{cfg.system}"

project "AMRT"
    location "AMRT"
    kind "SharedLib"
    language "C++"

    targetdir ("bin/" .. outputdir .. "/%{prj.name}")
    objdir ("bin-intermediate/" .. outputdir .. "/%{prj.name}")

    files
    {
        "%{prj.name}/src/**.h",
        "%{prj.name}/src/**.cpp",
    }

--[[     includedirs
    {
        "%{prj.name}/vendor/spdlog/include"
    } ]]

    filter "system:windows"
        cppdialect "C++17"
        staticruntime "On"
        systemversion "latest"

        defines
        {
            "AMRT_PLATFORM_WINDOWS",
            "AMRT_BUILD_DLL"
        }

        postbuildcommands
        {
            ("{COPY} %{cfg.buildtarget.relpath} ../bin/" .. outputdir .. "/Sandbox")
        }
    filter "configurations:Debug"
        defines "AMRT_DEBUG"
        symbols "On"

    filter "configurations:Release"
        defines "AMRT_RELEASE"
        optimize "On"

    filter "configurations:Dist"
        defines "AMRT_DIST"
        optimize "On"


project "Sandbox"
    location "Sandbox"
    kind "ConsoleApp"
    language "C++"

    targetdir ("bin/" .. outputdir .. "/%{prj.name}")
    objdir ("bin-intermediate/" .. outputdir .. "/%{prj.name}")

    files
    {
        "%{prj.name}/src/**.h",
        "%{prj.name}/src/**.cpp",
    }

    includedirs
    {
--[[         "%{prj.name}/vendor/spdlog/include", ]]
        "AMRT/src",
    }

    links 
    {
        "AMRT"
    }

    filter "system:windows"
        cppdialect "C++17"
        staticruntime "On"
        systemversion "latest"

        defines
        {
            "AMRT_PLATFORM_WINDOWS",
        }

    filter "configurations:Debug"
        defines "AMRT_DEBUG"
        symbols "On"

    filter "configurations:Release"
        defines "AMRT_RELEASE"
        optimize "On"

    filter "configurations:Dist"
        defines "AMRT_DIST"
        optimize "On"
