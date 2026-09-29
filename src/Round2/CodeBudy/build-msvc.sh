#!/bin/sh
# FFmpeg v4.4.2 minimal MSVC x64 build: libavcodec + libavutil, mp3/sbc/pcm only
set -e
# 以脚本自身所在目录为根，不硬编码本机绝对路径
ROOT="$(cd "$(dirname "$0")" && pwd)"
SRC=$ROOT/ffmpeg-4.4.2
PREFIX=$ROOT/build-x64
cd "$SRC"
case "$1" in
  configure)
    ./configure \
      --toolchain=msvc \
      --arch=x86_64 \
      --prefix="$PREFIX" \
      --disable-programs \
      --disable-doc \
      --disable-network \
      --disable-avdevice \
      --disable-avformat \
      --disable-swscale \
      --disable-swresample \
      --disable-postproc \
      --disable-avfilter \
      --disable-everything \
      --enable-shared \
      --enable-avcodec \
      --enable-avutil \
      --enable-decoder='mp3*,mp3adu*,mp3on4*,sbc,pcm_*' \
      --enable-encoder='mp3*,sbc,pcm_*' \
      --enable-parser=mpegaudio \
      --extra-cflags=-O2
    ;;
  make)
    make -j8
    ;;
  install)
    make install
    ;;
  clean)
    make distclean 2>/dev/null || true
    ;;
  *)
    echo "usage: build-msvc.sh {configure|make|install|clean}"
    exit 1
    ;;
esac
