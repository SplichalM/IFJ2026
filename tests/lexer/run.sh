#!/bin/sh
set -eu

repo_dir=$(CDPATH= cd -- "$(dirname -- "$0")/../.." && pwd)
test_binary=$(mktemp "${TMPDIR:-/tmp}/ifj-lexer-test.XXXXXX")
trap 'rm -f "$test_binary"' EXIT HUP INT TERM

"${CC:-gcc}" -std=c11 -Wall -Wextra -Wpedantic -Werror \
    -I"$repo_dir/include" "$repo_dir/src/lexer/main.c" \
    "$repo_dir/tests/lexer/test.c" -Wl,--wrap=malloc -Wl,--wrap=free \
    -o "$test_binary"

printf '' | "$test_binary" eof
for length in 15 16 17 31 32 33 63 64 65; do
    printf '%*s' "$length" '' | "$test_binary" eof
    { printf '%*s' "$length" ''; printf '\n  \n'; } | "$test_binary" eol
done
printf ' \n  \n' | "$test_binary" eol
printf '' | "$test_binary" init-first
printf '%17s' '' | "$test_binary" init-grow
printf '%33s' '' | "$test_binary" init-grow-again
"$test_binary" init-read </
printf '\n' | "$test_binary" peek-error
printf '\n' | "$test_binary" peek-no-memory
printf 'a' | "$test_binary" unimplemented
printf 'Lexer tests passed\n'
