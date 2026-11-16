# sohvk Vulkan test (RADV + VideoOut WSI on the PS4). Configure with -DCMAKE_TOOLCHAIN_FILE=../toolchain/ps4.cmake
cmake_minimum_required(VERSION 3.20)
project(sohvk_vktest C CXX ASM)
include(${CMAKE_CURRENT_LIST_DIR}/../toolchain/ps4-app.cmake)
set(CMAKE_CXX_STANDARD 20)
add_executable(vktest main.cpp ${PS4_PORT_SOURCES})
target_include_directories(vktest PRIVATE $ENV{VULKAN_HEADERS} ${PS4_PORT_DIR})
target_compile_definitions(vktest PRIVATE VK_NO_PROTOTYPES PS4_DATA_DIR="/data/sohvk")
target_compile_options(vktest PRIVATE -O2 -g1)
ps4_link_mesa(vktest)
target_link_libraries(vktest PRIVATE $ENV{PS4_SYSROOT}/lib/libz.a)
target_link_libraries(vktest PRIVATE -lSceSysmodule -lSceSystemService -lSceUserService -lSceVideoOut -lSceAudioOut
                      -lScePad -lSceNet)
ps4_add_eboot(vktest)
