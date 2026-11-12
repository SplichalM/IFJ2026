#!/bin/sh
set -eu

repo_dir=$(CDPATH= cd -- "$(dirname -- "$0")/../.." && pwd)
test_binary=$(mktemp "${TMPDIR:-/tmp}/ifj-lexer-test.XXXXXX")
trap 'rm -f "$test_binary"' EXIT HUP INT TERM

"${CC:-gcc}" -std=c11 -Wall -Wextra -Wpedantic -Werror \
    -I"$repo_dir/include" "$repo_dir/src/lexer/main.c" \
    "$repo_dir/tests/lexer/test.c" -Wl,--wrap=malloc -Wl,--wrap=free \
    -o "$test_binary"

printf 'LX-02: opakovana kontrola konce vstupu\n'
printf '' | "$test_binary" lx02-eof
for length in 15 16 17 31 32 33 63 64 65; do
    printf 'LX-02: opakovana kontrola konce vstupu (%s mezer)\n' "$length"
    printf '%*s' "$length" '' | "$test_binary" lx02-eof
    printf 'LX-02: nahled a cteni konce radku (%s mezer)\n' "$length"
    { printf '%*s' "$length" ''; printf '\n  \n'; } | "$test_binary" lx02-eol
done
printf 'LX-02: nahled a cteni konce radku\n'
printf ' \n  \n' | "$test_binary" lx02-eol
printf 'LX-02: chyba prvni alokace pri inicializaci\n'
printf '' | "$test_binary" lx02-init-first
printf 'LX-02: chyba prvniho zvetseni vstupu\n'
printf '%17s' '' | "$test_binary" lx02-init-grow
printf 'LX-02: chyba druheho zvetseni vstupu\n'
printf '%33s' '' | "$test_binary" lx02-init-grow-again
printf 'LX-02: chyba cteni vstupu\n'
"$test_binary" lx02-init-read </
printf 'LX-02: chyba pri nahledu\n'
printf '+' | "$test_binary" lx02-peek-error
printf 'LX-02: chyba alokace textu chyby pri nahledu\n'
printf '+' | "$test_binary" lx02-peek-no-memory
printf 'LX-02: dosud nepodporovany znak\n'
printf '+' | "$test_binary" lx02-unimplemented
printf 'LX-02: vsechny testy prosly\n'
printf 'LX-03: identifikatory, klicova slova, nahled a hranice tokenu\n'
printf ' val2\n_x x_2 main Val int val Int if while def do Double else import return String then Unit var' | "$test_binary" lx03-identifiers
printf 'LX-03: samotne podtrzitko je lexikalni chyba\n'
printf '_' | "$test_binary" lx03-underscore
printf 'LX-03: obraceny apostrof je lexikalni chyba\n'
printf '%s' '`foo`' | "$test_binary" lx03-backtick
printf '%s' '`' | "$test_binary" lx03-backtick
