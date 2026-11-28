#!/bin/sh
# Builds the Linux release for the machine it runs on (x86_64 or aarch64), on
# Linux. Leaves the stripped plugins and app in build-release/linux-<arch>-stage/,
# where scripts/make_release_zips.sh picks them up.
#
#   scripts/package_release_linux.sh
#
# Build on the oldest Linux the release should run on: the binaries need that
# glibc or newer. The release is built on Ubuntu 22.04 (glibc 2.35). Needs
# clang, ninja, CMake 3.31 and YUP's Linux packages.
#
# Building with PHYLLOTAXIS_INSTALL_PLUGINS off leaves ~/.vst3, ~/.clap and
# ~/.lv2 alone.

set -eu

root=$(cd "$(dirname "$0")/.." && pwd)
arch=$(uname -m)
build="$root/build-release/linux-$arch"
stage="$root/build-release/linux-$arch-stage"

cmake -S "$root" -B "$build" -G Ninja \
    -DYUP_DIR="$root/yup" \
    -DCMAKE_BUILD_TYPE=Release \
    -DPHYLLOTAXIS_INSTALL_PLUGINS=OFF
cmake --build "$build"

rm -rf "$stage"
mkdir -p "$stage"
cp -R "$build/VST3/Release/Phyllotaxis.vst3" "$stage/Phyllotaxis.vst3"
cp -R "$build/LV2/Phyllotaxis.lv2" "$stage/Phyllotaxis.lv2"
cp "$build/CLAP/Phyllotaxis.clap" "$stage/Phyllotaxis.clap"
cp "$build/Phyllotaxis" "$stage/Phyllotaxis"

# Release builds keep their symbols; the release doesn't need them
strip --strip-unneeded \
    "$stage"/Phyllotaxis.vst3/Contents/*-linux/*.so \
    "$stage"/Phyllotaxis.lv2/*.so \
    "$stage/Phyllotaxis.clap" \
    "$stage/Phyllotaxis"

echo "Staged in $stage"
