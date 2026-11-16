# Ship of Harkinian (Vulkan) for the PS4 - what the console adds to Shipwright's own build.
# Derived from damiantw/PS5_Shipwright's ps5/cmake/ps5-game.cmake (MIT), with the PS4 runtime and RADV of the Wind
# Waker HD PS4 port / SMW's sdk-1 (toolchain/ps4.cmake, toolchain/ps4-app.cmake).
#
# Configured with toolchain/ps4.cmake and named as CMAKE_PROJECT_Ship_INCLUDE: CMake includes it right after the
# project() call, before the build reads its options and looks for its dependencies. It provides:
#
#   PS5      the console switch Shipwright, libultraship, Torch and SDL already have for the PlayStation 5 port:
#            no OpenGL, volk, Vulkan with shaderc, console drivers in SDL, soh.o2r made on the host
#   SDL2     SDL 2.32 with the console's drivers (ps4/sdl: VideoOut + RADV, scePad, sceAudioOut)
#   Vulkan   Khronos headers; RADV itself (Mesa, PS4 winsys) is linked whole into the eboot
#   shaderc  ps4/shaderc: shaderc's C++ API over the SDK's glslang
#   zlib     the SDK's
#   the link ld.lld with the PS4 runtime (port/), then create-fself: soh_eboot/eboot.bin
#
# SPDX-License-Identifier: MIT

foreach(required SOHVK SDL_DIR)
	if(NOT ${required})
		message(FATAL_ERROR "ps4-game.cmake: ${required} is not set")
	endif()
endforeach()
set(ps4 ${SOHVK}/ps4)
include(FetchContent)

# The PlayStation console switch of the PS5 port's changes (Shipwright, libultraship, Torch, SDL)
set(PS5 TRUE)
set(PS4_GAME TRUE)

# The PS4 runtime (heap, crash log, threads, VideoOut WSI), crt1 with the malloc replacement, the link script
include(${SOHVK}/toolchain/ps4-app.cmake)

add_compile_options(-g1 -ffunction-sections -fdata-sections -fno-omit-frame-pointer)
add_compile_definitions(PS4_DATA_DIR="/data/sohvk")

# prism replaces CMAKE_CXX_FLAGS outright (set(CMAKE_CXX_FLAGS "-Wno-narrowing")), which drops the toolchain's
# sysroot, C++ library and PS4 defines: they go back on as its own options (ps4_late_fixes; Torch's CMakeLists
# appends instead). Joined spellings (-isystem/dir), since CMake removes repeated options.
string(REGEX REPLACE "-isystem +" "-isystem" ps4_cxx_flags "${CMAKE_CXX_FLAGS_INIT}")
separate_arguments(ps4_cxx_flags UNIX_COMMAND "${ps4_cxx_flags}")
set(CMAKE_POLICY_DEFAULT_CMP0077 NEW)
set(BUILD_SHARED_LIBS OFF)
set(CMAKE_CXX_SCAN_FOR_MODULES OFF)

# The platform archive (the console layer and the PS4 runtime), linked whole: its constructors have no callers
set(ps5_platform_archive ${CMAKE_BINARY_DIR}/libps5_platform.a)
add_library(ps5_platform STATIC ${ps4}/src/platform.c ${ps4}/src/startup.c ${ps4}/src/shims.c
	${ps4}/src/devloop_shot.c ${ps4}/src/shaderc_glslang.cpp ${PS4_PORT_SOURCES})
target_include_directories(ps5_platform PUBLIC ${ps4}/src PRIVATE ${PS4_PORT_DIR} ${ps4}/shaderc/libshaderc/include
	$ENV{VULKAN_HEADERS})
target_compile_definitions(ps5_platform PRIVATE "PS5_TITLE_NAME=\"Ship of Harkinian (Vulkan)\"" VK_NO_PROTOTYPES)
add_compile_definitions("PS5_TITLE_ID=\"SOHV00001\"")
set_target_properties(ps5_platform PROPERTIES ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set_source_files_properties(${ps4}/src/shaderc_glslang.cpp PROPERTIES COMPILE_OPTIONS "-O2")

# zlib: the SDK's
set(ZLIB_INCLUDE_DIR ${PS4_SYSROOT}/include CACHE PATH "" FORCE)
set(ZLIB_LIBRARY ${PS4_SYSROOT}/lib/libz.a CACHE FILEPATH "" FORCE)
target_include_directories(ps5_platform PRIVATE ${ZLIB_INCLUDE_DIR})

# SDL2: threads, time and files as a FreeBSD has them; video, input and audio from ps4/sdl
set(SDL_PS5_PORT_DIR ${ps4}/sdl CACHE INTERNAL "")
set(SDL_PS5_PLATFORM_DIR ${ps4}/src CACHE INTERNAL "")
set(SDL_STATIC ON CACHE BOOL "" FORCE)
set(SDL2_DISABLE_SDL2MAIN ON CACHE BOOL "" FORCE)
set(SDL2_DISABLE_INSTALL ON CACHE BOOL "" FORCE)
set(SDL2_DISABLE_UNINSTALL ON CACHE BOOL "" FORCE)
foreach(off SDL_SHARED SDL_TEST SDL_TESTS
		SDL_X11 SDL_WAYLAND SDL_KMSDRM SDL_RPI SDL_VIVANTE SDL_OFFSCREEN SDL_DUMMYVIDEO SDL_DIRECTFB
		SDL_OPENGL SDL_OPENGLES SDL_VULKAN
		SDL_OSS SDL_ALSA SDL_JACK SDL_ESD SDL_ARTS SDL_NAS SDL_PIPEWIRE SDL_PULSEAUDIO SDL_SNDIO SDL_FUSIONSOUND
		SDL_DISKAUDIO SDL_DUMMYAUDIO SDL_LIBSAMPLERATE
		SDL_DBUS SDL_IBUS SDL_LIBUDEV SDL_HIDAPI SDL_HIDAPI_LIBUSB SDL_HIDAPI_JOYSTICK SDL_VIRTUAL_JOYSTICK
		SDL_SYSTEM_ICONV SDL_LIBICONV SDL_RPATH SDL_SENSOR SDL_HAPTIC SDL_POWER)
	set(${off} OFF CACHE BOOL "" FORCE)
endforeach()
add_subdirectory(${SDL_DIR} ${CMAKE_BINARY_DIR}/SDL2 EXCLUDE_FROM_ALL)
target_include_directories(ps5_platform PRIVATE ${SDL_DIR}/include ${CMAKE_BINARY_DIR}/SDL2/include-config-release
	${CMAKE_BINARY_DIR}/SDL2/include-config-release/SDL2)
if(NOT TARGET SDL2::SDL2)
	add_library(SDL2::SDL2 ALIAS SDL2-static)
endif()
set(SDL2_INCLUDE_DIRS ${CMAKE_BINARY_DIR}/SDL2/include ${CMAKE_BINARY_DIR}/SDL2/include/SDL2
	${CMAKE_BINARY_DIR}/SDL2/include-config-release)
include_directories(${SDL2_INCLUDE_DIRS})

# Vulkan: the Khronos headers. The driver is RADV, linked whole by the link below.
set(Vulkan_INCLUDE_DIR $ENV{VULKAN_HEADERS} CACHE PATH "" FORCE)
set(Vulkan_LIBRARY ${ps5_platform_archive} CACHE FILEPATH "" FORCE)
include_directories(SYSTEM ${Vulkan_INCLUDE_DIR})
add_compile_definitions(VK_NO_PROTOTYPES)
add_library(volk STATIC ${ps4}/third_party/volk/volk.c)
target_include_directories(volk PUBLIC ${ps4}/third_party/volk)

# shaderc: ps4/shaderc over the SDK's glslang (libultraship links whichever of these exists)
set(SHADERC_DIR ${ps4}/shaderc CACHE PATH "" FORCE)
add_library(Vulkan::shaderc_combined INTERFACE IMPORTED GLOBAL)
target_include_directories(Vulkan::shaderc_combined INTERFACE ${ps4}/shaderc/libshaderc/include)
target_include_directories(ps5_platform PRIVATE ${PS4_SYSROOT}/include)

# libzip's checks link against the SDK's stubs, which name functions the console's C library does not declare
foreach(absent HAVE_MEMCPY_S HAVE_STRNCPY_S HAVE_STRERROR_S HAVE_STRERRORLEN_S)
	set(${absent} "" CACHE INTERNAL "" FORCE)
endforeach()

# Shipwright's dependencies, from source
set(tinyxml2_BUILD_TESTING OFF)
FetchContent_Declare(tinyxml2 GIT_REPOSITORY https://github.com/leethomason/tinyxml2.git GIT_TAG 10.0.0 GIT_SHALLOW TRUE OVERRIDE_FIND_PACKAGE)
FetchContent_Declare(nlohmann_json GIT_REPOSITORY https://github.com/nlohmann/json.git GIT_TAG v3.12.0 GIT_SHALLOW TRUE OVERRIDE_FIND_PACKAGE)
FetchContent_Declare(spdlog GIT_REPOSITORY https://github.com/gabime/spdlog.git GIT_TAG v1.16.0 GIT_SHALLOW TRUE OVERRIDE_FIND_PACKAGE)
foreach(off BUILD_TOOLS BUILD_REGRESS BUILD_EXAMPLES BUILD_DOC BUILD_OSSFUZZ ENABLE_BZIP2 ENABLE_LZMA ENABLE_ZSTD
		ENABLE_COMMONCRYPTO ENABLE_GNUTLS ENABLE_MBEDTLS ENABLE_OPENSSL ENABLE_WINDOWS_CRYPTO)
	set(${off} OFF CACHE BOOL "" FORCE)
endforeach()
FetchContent_Declare(libzip GIT_REPOSITORY https://github.com/nih-at/libzip.git GIT_TAG v1.11.4 GIT_SHALLOW TRUE OVERRIDE_FIND_PACKAGE)
FetchContent_MakeAvailable(tinyxml2 nlohmann_json spdlog libzip)
if(NOT TARGET libzip::zip)
	add_library(libzip::zip ALIAS zip)
endif()
include_directories(${libzip_SOURCE_DIR}/lib ${libzip_BINARY_DIR})

# SDL2_net (Shipwright's network features include it unconditionally)
set(SDL2NET_INSTALL OFF CACHE BOOL "" FORCE)
set(SDL2NET_SAMPLES OFF CACHE BOOL "" FORCE)
set(SDL2NET_BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
FetchContent_Declare(SDL2_net GIT_REPOSITORY https://github.com/libsdl-org/SDL_net.git GIT_TAG release-2.2.0 GIT_SHALLOW TRUE OVERRIDE_FIND_PACKAGE)
FetchContent_MakeAvailable(SDL2_net)
if(NOT TARGET SDL2_net::SDL2_net-static)
	add_library(SDL2_net::SDL2_net-static ALIAS SDL2_net)
endif()
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/sdl2net-include/SDL2)
file(COPY ${sdl2_net_SOURCE_DIR}/SDL_net.h DESTINATION ${CMAKE_BINARY_DIR}/sdl2net-include/SDL2)
include_directories(${CMAKE_BINARY_DIR}/sdl2net-include)

# The audio codecs SoH's sample importer uses (ogg, vorbis, opus, opusfile)
set(INSTALL_DOCS OFF CACHE BOOL "" FORCE)
set(BUILD_TESTING OFF CACHE BOOL "" FORCE)
set(OPUS_BUILD_PROGRAMS OFF CACHE BOOL "" FORCE)
set(OPUS_BUILD_TESTING OFF CACHE BOOL "" FORCE)
set(OP_DISABLE_HTTP ON CACHE BOOL "" FORCE)
set(OP_DISABLE_DOCS ON CACHE BOOL "" FORCE)
set(OP_DISABLE_EXAMPLES ON CACHE BOOL "" FORCE)
FetchContent_Declare(Ogg GIT_REPOSITORY https://github.com/xiph/ogg.git GIT_TAG v1.3.6 GIT_SHALLOW TRUE OVERRIDE_FIND_PACKAGE)
FetchContent_Declare(Vorbis GIT_REPOSITORY https://github.com/xiph/vorbis.git GIT_TAG v1.3.7 GIT_SHALLOW TRUE OVERRIDE_FIND_PACKAGE)
FetchContent_Declare(Opus GIT_REPOSITORY https://github.com/xiph/opus.git GIT_TAG v1.5.2 GIT_SHALLOW TRUE OVERRIDE_FIND_PACKAGE)
FetchContent_MakeAvailable(Ogg Opus)
if(NOT TARGET Ogg::ogg)
	add_library(Ogg::ogg ALIAS ogg)
endif()
if(NOT TARGET Opus::opus)
	add_library(Opus::opus ALIAS opus)
endif()
FetchContent_MakeAvailable(Vorbis)
FetchContent_Declare(OpusFile GIT_REPOSITORY https://github.com/xiph/opusfile.git GIT_TAG v0.12 GIT_SHALLOW TRUE)
FetchContent_GetProperties(OpusFile)
if(NOT opusfile_POPULATED)
	FetchContent_Populate(OpusFile)
endif()
add_library(opusfile STATIC ${opusfile_SOURCE_DIR}/src/info.c ${opusfile_SOURCE_DIR}/src/internal.c
	${opusfile_SOURCE_DIR}/src/opusfile.c ${opusfile_SOURCE_DIR}/src/stream.c)
target_include_directories(opusfile PUBLIC ${opusfile_SOURCE_DIR}/include)
target_link_libraries(opusfile PUBLIC ogg opus)
target_compile_definitions(opusfile PRIVATE OP_DISABLE_HTTP OP_DISABLE_FLOAT_API=0)
include_directories(${opusfile_SOURCE_DIR}/include ${opus_SOURCE_DIR}/include ${ogg_SOURCE_DIR}/include
	${ogg_BINARY_DIR}/include ${vorbis_SOURCE_DIR}/include)

# Shipwright's options, as the console has them
set(BUILD_REMOTE_CONTROL OFF CACHE BOOL "" FORCE)
set(USE_NETWORKING OFF CACHE BOOL "" FORCE)
set(ENABLE_SCRIPTING OFF CACHE BOOL "" FORCE)
set(DISABLE_DLL_LOADER ON CACHE BOOL "" FORCE)

# The link: every object and archive CMake hands the linker goes through tools/link-eboot.sh (ld.lld with the PS4
# runtime's crt1, link script and wraps, RADV whole, glslang and the system stubs in the group, then create-fself)
set(ps4_glslang_libs ${PS4_SYSROOT}/lib/libglslang-default-resource-limits.a ${PS4_SYSROOT}/lib/libSPIRV.a
	${PS4_SYSROOT}/lib/libglslang.a ${PS4_SYSROOT}/lib/libMachineIndependent.a ${PS4_SYSROOT}/lib/libOSDependent.a
	${PS4_SYSROOT}/lib/libGenericCodeGen.a)
list(JOIN ps4_glslang_libs " " ps4_glslang_link)
list(TRANSFORM PS4_MESA_LIBS PREPEND "${PS4_MESA_BUILD}/" OUTPUT_VARIABLE ps4_mesa_paths)
list(JOIN ps4_mesa_paths " " ps4_mesa_link)
list(JOIN PS4_WRAPS " " ps4_wraps)
set(ps4_extra_wraps --wrap=getcwd --wrap=realpath --wrap=lstat --wrap=tmpfile)
list(JOIN ps4_extra_wraps " " ps4_extra_wraps_link)
set(CMAKE_CXX_LINK_EXECUTABLE
	"bash ${SOHVK}/tools/link-eboot.sh <TARGET> ${PS4_LLVM} ${OPENORBIS} ${PS4_LDFLAGS} --script ${PS4_LINK_SCRIPT} ${ps4_wraps} ${ps4_extra_wraps_link} ${PS4_CRT1} --whole-archive ${ps5_platform_archive} ${ps4_mesa_link} --no-whole-archive <OBJECTS> --start-group <LINK_LIBRARIES> ${ps4_glslang_link} ${PS4_SYSROOT}/lib/libz.a -lSceSysmodule -lSceSystemService -lSceUserService -lSceVideoOut -lSceAudioOut -lScePad -lSceNet -lSceLibcInternal --end-group")
set(CMAKE_C_LINK_EXECUTABLE "${CMAKE_CXX_LINK_EXECUTABLE}")

# The platform archive is named by its file in the link, so the game does not depend on it by itself
function(ps4_late_fixes)
	add_dependencies(soh ps5_platform volk)
	set_property(TARGET soh APPEND PROPERTY LINK_DEPENDS ${ps5_platform_archive})
	if(TARGET prism)
		target_compile_options(prism PRIVATE ${ps4_cxx_flags})
	endif()
endfunction()
cmake_language(DEFER DIRECTORY ${CMAKE_SOURCE_DIR} CALL ps4_late_fixes)
