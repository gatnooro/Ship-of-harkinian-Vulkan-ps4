#!/usr/bin/env bash
# Packs Ship of Harkinian (Vulkan) for the PS4: the eboot from build-soh plus what the title ships beside it.
#
#   tools/package-soh.sh [version]      (default 01.00) -> out-soh/SoH-Vulkan-PS4.pkg and .zip
set -euo pipefail
here=$(cd "$(dirname "$0")/.." && pwd)
[ -f "$here/env.sh" ] || { echo "no env.sh: copy env.example.sh to env.sh and set your tool paths" >&2; exit 1; }
source "$here/env.sh"
version=${1:-01.00}
eboot=$here/build-soh/soh/soh_eboot/eboot.bin
[ -f "$eboot" ] || { echo "no eboot: $eboot" >&2; exit 1; }
for f in soh.o2r assets.zip ps5_prompts.o2r; do
    [ -f "$here/pkgfiles/$f" ] || { echo "missing pkgfiles/$f" >&2; exit 1; }
done
out=$here/out-soh
mkdir -p "$out"
python3 -I "$here/tools/make_pkg.py" \
    --eboot "$eboot" \
    --title "Ship of Harkinian (Vulkan)" \
    --title-id SOHV00001 \
    --label SOHVULKANPS40000 \
    --version "$version" \
    --icon "$here/assets/sohvk-icon0.png" \
    --pic "$here/assets/sohvk-pic1.png" \
    --file "soh.o2r=$here/pkgfiles/soh.o2r" \
    --file "assets.zip=$here/pkgfiles/assets.zip" \
    --file "ps5_prompts.o2r=$here/pkgfiles/ps5_prompts.o2r" \
    --file "shader_keys.txt=$here/ps4/shader_keys.txt" \
    --out "$out" --name SoH-Vulkan-PS4.pkg
rm -f "$out/SoH-Vulkan-PS4.zip"
(cd "$out" && zip -q -0 SoH-Vulkan-PS4.zip SoH-Vulkan-PS4.pkg)
ls -la "$out"/SoH-Vulkan-PS4.*
