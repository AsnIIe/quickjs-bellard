local ver = "Unknow"

(function()
	local file = io.open("VERSION", "r");
	ver = file:read()
	file:close()
end)()

workspace "quickjs-bellard"
	-- Premake output folder
	location(path.join("build", _ACTION))

	platforms { "x86", "x64"  } 

	defines { "CONFIG_VERSION=\""..ver.."\"" }
	defines { "USE_WORKER" }

	-- Configuration settings
	configurations { "Debug", "Release" }

	filter "platforms:x86"
  		architecture "x86"
	filter "platforms:x64"
  		architecture "x64"  

	-- Debug configuration
	filter { "configurations:Debug" }
		defines { "DEBUG" }
		symbols "On"
		optimize "Off"
		debugdir "build/bin/%{cfg.buildcfg}/%{cfg.platform}"

	-- Release configuration
	filter { "configurations:Release" }
		defines { "NDEBUG" }
		optimize "Speed"
		inlining "Auto"
		flags { "LinkTimeOptimization" }

	filter { "language:not C#" }
		defines { "_CRT_SECURE_NO_WARNINGS" }
		buildoptions { "/std:c++14" }
		systemversion "latest"

	filter { }
		targetdir "build/bin/%{cfg.buildcfg}/%{cfg.platform}"
		exceptionhandling "Off"
		rtti "Off"
		--vectorextensions "AVX2"

    includedirs {
        "thirdparty/pthread",
    }
    libdirs {
        "thirdparty/libs/%{cfg.buildcfg}",
    }
	filter "system:windows"
        links { "libwinpthread%{cfg.platform}.lib" }
-----------------------------------------------------------------------------------------------------------------------
-- lib ECMAScript
project "libes"
	language "C"
	kind "SharedLib"
	files {
		"quickjs.h",
		"cutils.h",
		"quickjs-libc.h",
		"libregexp.h",
		"libregexp-opcode.h",
		"libunicode.h",
		"libunicode-table.h",
		"quickjs-atom.h",
		"list.h",
		"quickjs-opcode.h",
		"dtoa.h",

		"quickjspp.h",

		"platform/dirent.h",
		"platform/getopt.h",
		"platform/dynamic-resolver.h",

		"cutils.c",
		"libregexp.c",
		"libunicode.c",
		"quickjs.c",
		"quickjs-libc.c",
		"dtoa.c",

		"platform/quickjs-export.def"
	}
-----------------------------------------------------------------------------------------------------------------------
project "libquickjs"
	language "C"
	kind "StaticLib"
	files {
		"quickjs.h",
		"cutils.h",
		"quickjs-libc.h",
		"libregexp.h",
		"libregexp-opcode.h",
		"libunicode.h",
		"libunicode-table.h",
		"quickjs-atom.h",
		"list.h",
		"quickjs-opcode.h",
		"dtoa.h",

		"quickjspp.h",

		"platform/dirent.h",
		"platform/getopt.h",
		"platform/dynamic-resolver.h",

		"cutils.c",
		"libregexp.c",
		"libunicode.c",
		"quickjs.c",
		"quickjs-libc.c",
		"dtoa.c"
	}
	targetdir "build/lib/%{cfg.buildcfg}"
	targetname "%{prj.name}%{cfg.platform}"
-----------------------------------------------------------------------------------------------------------------------
project "examples"
	language "C++"
	kind "ConsoleApp"
	links { "libes" }
	exceptionhandling "On"
	rtti "On"
	files {
		"examples/main.cpp"
	}
-----------------------------------------------------------------------------------------------------------------------
project "qjsc"
	language "C"
	kind "ConsoleApp"
	links { "libes" }
	files {
		"qjsc.c"
	}
-----------------------------------------------------------------------------------------------------------------------
project "qjs"
	language "C"
	kind "ConsoleApp"
	links { "libes" }
	dependson { "qjsc" }
	files {
		"qjs.c",
		"repl.js",
		"repl.c"
	}
-- Compile repl.js and save bytecode into repl.c
prebuildcommands { "\"%{cfg.buildtarget.directory}/qjsc.exe\" -c -o \"../../repl.c\" -m \"../../repl.js\"" }