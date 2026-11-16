#!/usr/bin/env bash
# Links a PS4 eboot: ld.lld with the arguments ps4/cmake/ps4-game.cmake gives, then create-fself.
#
#   link-eboot.sh TARGET.elf LLVM_BIN OPENORBIS <ld.lld arguments and the objects CMake hands over>
#
# CMake's link line arrives as it would go to a compiler driver: -Wl, arguments are unpacked for ld.lld, and the
# driver-only flags (threads, maths, dynamic exports: the PS4's libc and libkernel have them) are dropped. The eboot
# goes to <target>_eboot/eboot.bin next to the ELF, signed as the OpenOrbis homebrew of RetroArch's identity (the
# PAID and auth info toolchain/ps4-app.cmake names; RADV needs the game category of param.sfo, see make_pkg.py).
set -euo pipefail
target=$1 llvm=$2 oo=$3
shift 3
args=()
for a in "$@"; do
    case $a in
    -Xlinker | -pthread | -lpthread | -lm | -ldl | -lrt | -rdynamic | -E | -Wl,--export-dynamic | -Wl,-export-dynamic) ;;
    -Wl,*) IFS=, read -r -a parts <<<"${a#-Wl,}"; args+=("${parts[@]}") ;;
    # (-pie stays: ld.lld takes it as it is, and an eboot must be position independent: the PS4 loads it at
    # 0x400000, and without -pie no pointer in its data would be relocated there)
    -fPIC | -fpic | -fPIE | -O[0-9s] | -g | -g[0-9] | -march=* | -m64 | --target=* | -flto*) ;;
    *) args+=("$a") ;;
    esac
done
"$llvm/ld.lld" "${args[@]}" -o "$target"
if ! "$llvm/llvm-readelf" -h "$target" | grep -q "Type:.*DYN"; then
    echo "link-eboot.sh: $target is not position independent (no -pie): the PS4 cannot run it" >&2
    exit 1
fi

name=$(basename "${target%.elf}")
out="$(dirname "$target")/${name}_eboot"
mkdir -p "$out"
paid=0x3100000000000002
authinfo=000000000000000000000000001C004000FF000000000080000000000000000000000000000000000000008000400040000000000000008000000000000000080040FFFF000000F000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
OO_PS4_TOOLCHAIN="$oo" "$oo/bin/linux/create-fself" -in="$target" -out="$out/$name.oelf" --eboot "$out/eboot.bin" \
    --paid "$paid" --authinfo "$authinfo" > "$out/create-fself.log" 2>&1 || { cat "$out/create-fself.log" >&2; exit 1; }
# every PT_LOAD of the fake SELF must have been written (toolchain/check-oelf.cmake)
cmake -DOELF="$out/$name.oelf" -DREADELF="$llvm/llvm-readelf" -P "$(dirname "$0")/../toolchain/check-oelf.cmake"
ls -la "$out/eboot.bin"
