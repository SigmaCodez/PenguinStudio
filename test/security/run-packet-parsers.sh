#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/../.."
build_dir=${PENGUIN_SECURITY_BUILD_DIR:-$(mktemp -d)}
mkdir -p "$build_dir"
cc -g -O1 -fsanitize=address,undefined -fno-omit-frame-pointer \
  -DNO_CRYPTO -Ilibobs -I/usr/include/obs \
  ${PENGUIN_SECURITY_CPPFLAGS:-} -Iplugins/obs-outputs/librtmp \
  test/security/packet-parsers.c libobs/obs-av1.c libobs/obs-avc.c libobs/obs-nal.c \
  plugins/obs-outputs/librtmp/amf.c plugins/obs-outputs/librtmp/log.c \
  -lobs -o "$build_dir/packet-parsers"
"$build_dir/packet-parsers"
