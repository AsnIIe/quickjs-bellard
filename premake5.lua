
workspace "quickjs-bellard"
	-- Premake output folder
	location(path.join("build", _ACTION))

	platforms { "x86", "x64", "arm32", "arm64"  } 

	-- Configuration settings
	configurations { "Debug", "Release" }

	filter "platforms:x86"
  	architecture "x86"
	filter "platforms:x64"
  	architecture "x86_64"  
	filter "platforms:arm32"
  	architecture "ARM"  
	filter "platforms:arm64"
  	architecture "ARM64"  

	filter "system:windows"
  	removeplatforms { "arm32" }  

	-- Debug configuration
	filter { "configurations:Debug" }
		defines { "DEBUG" }
		symbols "On"
		optimize "Off"

	-- Release configuration
	filter { "configurations:Release" }
		defines { "NDEBUG" }
		optimize "Speed"
		inlining "Auto"

	filter { "language:not C#" }
		defines { "_CRT_SECURE_NO_WARNINGS" }
		buildoptions { "/std:c++14" }
		systemversion "latest"

	filter { }
		targetdir "build/bin/%{cfg.buildcfg}/%{cfg.platform}"
		exceptionhandling "Off"
		rtti "Off"
		--vectorextensions "AVX2"

-----------------------------------------------------------------------------------------------------------------------
project "libruntime"
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

		"platform/dirent.h",
		"platform/getopt.h",
		
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

		"platform/dirent.h",
		"platform/getopt.h",

		"cutils.c",
		"libregexp.c",
		"libunicode.c",
		"quickjs.c",
		"quickjs-libc.c",
		"dtoa.c"
	}
	targetdir "libs/"
-----------------------------------------------------------------------------------------------------------------------
project "examples"
	language "C"
	kind "ConsoleApp"
	links { "libruntime" }
	files {
		"examples/main.cpp"
	}
-----------------------------------------------------------------------------------------------------------------------
project "qjsc"
	language "C"
	kind "ConsoleApp"
	links { "libquickjs" }
	files {
		"qjsc.c"
	}
-----------------------------------------------------------------------------------------------------------------------
project "qjs"
	language "C"
	kind "ConsoleApp"
	links { "libquickjs" }
	dependson { "qjsc" }
	files {
		"qjs.c",
		"repl.js",
		"repl.c"
	}
-- Compile repl.js and save bytecode into repl.c
prebuildcommands { "\"%{cfg.buildtarget.directory}/qjsc.exe\" -c -o \"../../repl.c\" -m \"../../repl.js\"" }