# Tool paths for the build and the packaging. Copy this file to env.sh, set the paths for your machine,
# and `source env.sh` before running cmake, ninja or tools/package-soh.sh.

# this repository
export SOHVK="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# LLVM 21.1.8: clang, ld.lld, llvm-ar
export PS4_LLVM=/opt/llvm21/bin

# OpenOrbis PS4 Toolchain v0.5.2 (PacBrew): headers, stubs, create-fself, PkgTool
export OPENORBIS=/opt/OpenOrbis/PS4Toolchain
export OO_PS4_TOOLCHAIN=$OPENORBIS

# SMW PS4 SDK sdk-1 (github.com/iHaiDeeZ/SMW): the libc++ sysroot and RADV for the PS4
export PS4_SYSROOT=/opt/smw-ps4-sdk/sysroot
export PS4_MESA=/opt/smw-ps4-sdk/build/mesa

# Vulkan-Headers v1.4.357
export VULKAN_HEADERS=/opt/Vulkan-Headers/include

export PS4_PREFIX=$SOHVK/prefix

# PkgTool is a .NET program: no ICU needed with this
export DOTNET_SYSTEM_GLOBALIZATION_INVARIANT=1
# PkgTool wants OpenSSL 1.1. On a distribution that only has OpenSSL 3, point this at a folder holding libssl.so.1.1
# and libcrypto.so.1.1; leave it empty otherwise.
export PS4_LIBSSL11=
