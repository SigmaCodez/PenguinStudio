#!/usr/bin/env bash
set -euo pipefail
root=$(cd "$(dirname "$0")/../.." && pwd)
work=$(mktemp -d)
trap 'rm -rf "$work"' EXIT
printf 'int dependency(void) { return 42; }\n' > "$work/dependency.c"
printf 'extern int dependency(void); int entry(void) { return dependency(); }\n' > "$work/library.c"
printf 'Not an ELF library\n' > "$work/invalid.so"
cc -shared -fPIC "$work/dependency.c" -Wl,-soname,libpenguin-test-dependency.so -o "$work/libpenguin-test-dependency.so"
cc -shared -fPIC "$work/library.c" -L"$work" -lpenguin-test-dependency -Wl,-rpath,"$work" -o "$work/broken.so"
cc -std=c17 -Wall -Wextra -Werror -I"$root/libobs" "$root/test/penguin-optional-library.c" -ldl -o "$work/check"
# First check an actual loaded library, then remove its dependency to distinguish
# an installed but broken runtime from an absent top-level optional library.
cp "$work/libpenguin-test-dependency.so" "$work/valid.so"
rm "$work/libpenguin-test-dependency.so"
"$work/check" "$work/absent.so" "$work/valid.so" "$work/invalid.so" "$work/broken.so"
