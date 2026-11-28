#!/bin/sh
# Zips the release from the staged builds:
#   build-release/stage/       from scripts/package_release.sh (macOS)
#   build-release/win-stage/   the stage folder of scripts/package_release.ps1,
#                              copied over from the Windows machine
#   build-release/linux-<arch>-stage/   from scripts/package_release_linux.sh,
#                              for x86_64 and aarch64; each is optional
#
#   scripts/make_release_zips.sh
#
# Writes build-release/dist/Phyllotaxis-<version>-{macOS,Windows-x64,Max}.zip and
# Phyllotaxis-<version>-Linux-<arch>.tar.gz

set -eu

root=$(cd "$(dirname "$0")/.." && pwd)
build="$root/build-release"
dist="$build/dist"
version=$(sed -n 's/^set (target_version "\(.*\)")$/\1/p' "$root/CMakeLists.txt")
[ -n "$version" ] || { echo "no version in CMakeLists.txt" >&2; exit 1; }

rm -rf "$dist"
mkdir -p "$dist"

package() # platform label, stage folder, install text, licence platform [, build folder]
{
    folder="$dist/Phyllotaxis-$version-$1"
    mkdir "$folder"
    ditto "$2" "$folder"
    sed "s/@VERSION@/$version/" "$3" > "$folder/INSTALL.txt"
    cp "$root/LICENSE" "$folder/LICENSE.txt"
    "$root/scripts/third_party_licenses.sh" "${5:-$build}" "$4" "$folder/THIRD_PARTY_LICENSES.txt"
}

package macOS "$build/stage" "$root/scripts/release/INSTALL-macOS.txt" macos
package Windows-x64 "$build/win-stage" "$root/scripts/release/INSTALL-Windows.txt" windows
linux_arches=""
for arch in x86_64 aarch64; do
    [ -d "$build/linux-$arch-stage" ] || continue
    package "Linux-$arch" "$build/linux-$arch-stage" "$root/scripts/release/INSTALL-Linux.txt" linux "$build/linux-$arch"
    linux_arches="$linux_arches $arch"
done

max="$dist/Phyllotaxis-$version-Max"
mkdir "$max"
cp "$root/phyllotaxis_moire_sequencer.maxpat" "$root/phyllotaxis_moire_sequencer.js" "$max/"
cp "$root/LICENSE" "$max/LICENSE.txt"

cd "$dist"
# ditto keeps the macOS bundles' signatures and symlinks intact
ditto -c -k --norsrc --noextattr --noqtn --keepParent "Phyllotaxis-$version-macOS" "Phyllotaxis-$version-macOS.zip"
for name in Windows-x64 Max; do
    zip -q -r -X "Phyllotaxis-$version-$name.zip" "Phyllotaxis-$version-$name"
done

# tar keeps the executable bits; no macOS metadata in it
for arch in $linux_arches; do
    COPYFILE_DISABLE=1 tar --no-xattrs --no-mac-metadata -czf "Phyllotaxis-$version-Linux-$arch.tar.gz" "Phyllotaxis-$version-Linux-$arch"
done

ls -l "$dist"/*.zip "$dist"/*.tar.gz 2>/dev/null
