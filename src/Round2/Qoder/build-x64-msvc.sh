#!/bin/sh
# FFmpeg v4.4.2 x64 MSVC 构建脚本（由 run-build-x64.bat 在 vcvars64 环境中调用）
# configure 已完成时自动跳过，直接增量 make。
set -e

# 以脚本自身所在目录为根，不硬编码本机绝对路径
ROOT="$(cd "$(dirname "$0")" && pwd)"
cd "$ROOT/ffmpeg-4.4.2"

# 便携版 GNU make（无 nasm，x86 asm 优化关闭，纯 C 编译）
export PATH="$ROOT/tools/make/bin:$PATH"
export SHELL=/usr/bin/bash

if [ ! -f ffbuild/config.mak ]; then
    ./configure \
        --toolchain=msvc \
        --enable-shared \
        --disable-programs \
        --disable-doc \
        --disable-avformat \
        --disable-avfilter \
        --disable-avdevice \
        --disable-swscale \
        --disable-swresample \
        --disable-postproc \
        --disable-network \
        --disable-everything \
        --disable-x86asm \
        --enable-parser=mpegaudio,sbc \
        --enable-decoder=mp3,mp3float,mp3adu,mp3adufloat,mp3on4,mp3on4float,sbc,\
pcm_sga,pcm_u16be,pcm_u16le,pcm_u24be,pcm_u24le,pcm_u32be,pcm_u32le,pcm_u8,\
pcm_vidc,pcm_alaw,pcm_bluray,pcm_dvd,pcm_f16le,pcm_f24le,pcm_f32be,pcm_f32le,\
pcm_f64be,pcm_f64le,pcm_lxf,pcm_mulaw,pcm_s16be,pcm_s16be_planar,pcm_s16le,\
pcm_s16le_planar,pcm_s24be,pcm_s24daud,pcm_s24le,pcm_s24le_planar,pcm_s32be,\
pcm_s32le,pcm_s32le_planar,pcm_s64be,pcm_s64le,pcm_s8,pcm_s8_planar
fi

make -j8
