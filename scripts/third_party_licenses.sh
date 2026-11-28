#!/bin/sh
# Writes the licenses of everything built into the plugin binaries, for the
# release zips. Needs a configured build folder, for the libraries YUP fetches.
#
#   scripts/third_party_licenses.sh <build folder> <macos|windows|linux> <output file>
#
# The list follows what the plugin links: YUP's modules from yup_gui and
# yup_audio_processors down, their third-party modules, SDL3, the plugin SDKs
# and the fonts YUP's theme embeds.

set -eu

root=$(cd "$(dirname "$0")/.." && pwd)
build=$1
platform=$2
out=$3
ext="$build/externals"
third="$root/yup/thirdparty"

section() # name, license name, file...
{
    name=$1
    license=$2
    shift 2
    printf '\n================================================================================\n' >> "$out"
    printf '%s (%s)\n' "$name" "$license" >> "$out"
    printf '================================================================================\n\n' >> "$out"
    for file in "$@"; do
        # YUP fetches some modules' sources next to the build folder rather
        # than into it, which is where they land for the Linux builds
        case "$file" in "$ext"/*) [ -f "$file" ] || file="$build/../externals/${file#"$ext"/}" ;; esac
        [ -f "$file" ] || { echo "missing license file: $file" >&2; exit 1; }
        cat "$file" >> "$out"
        printf '\n' >> "$out"
    done
}

cat > "$out" <<'EOF'
Phyllotaxis is built with the libraries below. Their licenses follow.
Phyllotaxis's own code carries no copyright: see LICENSE.
EOF

section "YUP" "ISC" "$root/yup/LICENSE"
section "SDL3" "zlib" "$ext/SDL3/LICENSE.txt"
section "VST3 SDK" "MIT" "$ext/vst3sdk/LICENSE.txt"
section "CLAP" "MIT" "$ext/clap/LICENSE"
if [ "$platform" = macos ]; then
    section "AudioUnitSDK" "Apache 2.0" "$ext/AudioUnitSDK/LICENSE.txt"
fi
if [ "$platform" = linux ]; then
    section "LV2" "ISC" "$ext/lv2-sdk/COPYING"
fi
section "Rive Runtime" "MIT" "$third/rive/LICENSE"
section "Rive Decoders" "MIT" "$third/rive_decoders/LICENSE"
section "HarfBuzz" "Old MIT" "$third/harfbuzz/LICENSE"
section "SheenBidi" "Apache 2.0" "$third/sheenbidi_library/LICENSE"
section "Yoga" "MIT" "$third/yoga_library/LICENSE"
section "libhydrogen" "ISC" "$third/libhydrogen/LICENSE"
section "Luau" "MIT" "$third/luau/LICENSE"
section "glad" "MIT" "$third/glad/LICENSE"
section "Vulkan Headers and Vulkan Memory Allocator" "Apache 2.0, MIT" \
    "$third/vulkan_library/LICENSES/Vulkan-Headers.md" \
    "$third/vulkan_library/LICENSES/Apache-2.0.txt" \
    "$third/vulkan_library/LICENSES/MIT.txt" \
    "$third/vulkan_library/LICENSES/VulkanMemoryAllocator.txt"
section "Clipper2" "Boost Software License 1.0" "$third/libclipper2/LICENSE"
section "zlib" "zlib" "$third/zlib/LICENSE"
section "xsimd" "BSD 3-Clause" "$ext/xsimd/LICENSE"
section "xxHash" "BSD 2-Clause" "$ext/xxhash_library/LICENSE"
section "LZAV" "MIT" "$ext/lzav_library/LICENSE"
section "Roboto Flex font" "SIL Open Font License 1.1" "$root/scripts/licenses/RobotoFlex-OFL.txt"
section "JetBrains Mono font" "SIL Open Font License 1.1" "$root/scripts/licenses/JetBrainsMono-OFL.txt"
section "Font Awesome font" "SIL Open Font License 1.1" "$root/scripts/licenses/FontAwesome-LICENSE.txt"

echo "Wrote $out"
