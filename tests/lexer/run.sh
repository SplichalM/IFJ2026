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
printf '!' | "$test_binary" lx02-peek-error
printf 'LX-02: chyba alokace textu chyby pri nahledu\n'
printf '!' | "$test_binary" lx02-peek-no-memory
printf 'LX-02: dosud nepodporovany znak\n'
printf '\000' | "$test_binary" lx02-unimplemented
printf 'LX-02: vsechny testy prosly\n'
printf 'LX-03: identifikatory, klicova slova, nahled a hranice tokenu\n'
printf ' val2\n_x x_2 main Val int val Int if while def do Double else import return String then Unit var' | "$test_binary" lx03-identifiers
printf 'LX-03: samotne podtrzitko je lexikalni chyba\n'
printf '_' | "$test_binary" lx03-underscore
printf 'LX-03: obraceny apostrof je lexikalni chyba\n'
printf '%s' '`foo`' | "$test_binary" lx03-backtick
printf '%s' '`' | "$test_binary" lx03-backtick
printf 'LX-04: operatory, oddelovace, anotace a nahled\n'
printf '%s' 'a<=b<c=d==e > >= != + - * / import ifj26.* @main def main():Unit,;@main' |
    "$test_binary" lx04-symbols
printf 'LX-04: neplatny vykricnik a neplatne anotace\n'
for input in '!' '!x' '@' '@m' '@ma' '@mai' '@ main' '@Main' '@mainx' '@main0' '@main_'; do
    printf '%s' "$input" | "$test_binary" lx04-invalid-symbol
done
printf 'LX-04: zacatek desetinneho literalu\n'
printf '%s' '.5' | "$test_binary" lx04-deferred-token
printf 'LX-04: chyba alokace textu tokenu\n'
for input in '==' '(' '@main'; do
    printf '%s' "$input" | "$test_binary" lx04-token-no-memory
done
printf 'LX-05: cela cisla a minus\n'
printf '%s' '1_000_000 -42' | "$test_binary" lx05-integer
for input in '01' '1__0' '1_'; do
    printf '%s' "$input" | "$test_binary" lx05-invalid
done
printf 'LX-06: desetinna cisla a nahled\n'
printf '%s' '.5 1.25e+2D 1e3' | "$test_binary" lx06-float
for input in '12.' '1e' '1e+' '1.2e-'; do
    printf '%s' "$input" | "$test_binary" lx06-invalid
done
printf 'LX-07: prazdny retezec, escape sekvence, nahled a hranice tokenu\n'
printf '%s' '"" "a\\\"\n\r\t")' | "$test_binary" lx07-strings
printf 'LX-07: neplatna escape sekvence, LF a neukonceny retezec\n'
for input in '"\q"' '"abc' '"abc\'; do
    printf '%s' "$input" | "$test_binary" lx07-invalid
done
printf '"a\nb"' | "$test_binary" lx07-invalid
printf 'LX-07: chyba alokace obsahu retezce\n'
printf '%s' '"abc"' | "$test_binary" lx07-token-no-memory
printf 'LX-08: prazdny a viceradkovy retezec, doslovny obsah a nahled\n'
printf '%s\n%s\n' '"""""" """A' '  B\n ""x""""' | "$test_binary" lx08-strings
printf 'LX-08: neukonceny viceradkovy retezec\n'
for input in '"""' '"""a"' '"""a""'; do
    printf '%s' "$input" | "$test_binary" lx08-invalid
done
printf '"""a\nb' | "$test_binary" lx08-invalid
printf 'LX-08: chyba alokace obsahu retezce\n'
printf '%s' '"""abc"""' | "$test_binary" lx08-token-no-memory
printf 'LX-09: radkovy komentar, nahled a dalsi radek\n'
printf 'a// /* */\n  b' | "$test_binary" lx09-line
printf 'LX-09: odsazeni po samostatnem komentari\n'
printf 'a\n  // poznamka\n    b' | "$test_binary" lx09-comment-line
printf 'LX-09: radkovy komentar u konce souboru\n'
printf '// konec' | "$test_binary" lx09-eof
printf 'LX-10: blokovy komentar a vnoreny komentar\n'
printf 'a/* jedna /* dva */ tri */b' | "$test_binary" lx10-nested
printf 'LX-10: neukonceny vnoreny komentar\n'
printf '/* jedna /* dva */' | "$test_binary" lx10-unclosed
printf 'LX-11: vice radku a token za komentarem\n'
printf '/*\n*/x' | "$test_binary" lx11-trailing
printf 'LX-12: vice radku v komentari a nasledujici odsazeni\n'
printf '/*\ntext\n*/  \n  b' | "$test_binary" lx12-block-eol
