#!/bin/sh

usage() {
    printf 'Usage: sh tests/run.sh [lexer|parser|semantic|all]\n' >&2
}

if [ "$#" -gt 1 ]; then
    usage
    exit 2
fi

case "${1-all}" in
    lexer|parser|semantic) ;;
    all) set -- lexer parser semantic ;;
    *) usage; exit 2 ;;
esac

repo_dir=$(CDPATH= cd "$(dirname "$0")/.." && pwd) || exit 1
cd "$repo_dir" || exit 1

compiler=${CC:-gcc}
if [ -z "${CC:-}" ] && ! command -v "$compiler" >/dev/null 2>&1; then
    qt_compiler=/c/Qt/Tools/mingw1120_64/bin/gcc.exe
    if [ -x "$qt_compiler" ]; then
        compiler=$qt_compiler
    fi
fi
if ! command -v "$compiler" >/dev/null 2>&1; then
    printf 'Compiler not found: %s. Set CC to its path.\n' "$compiler" >&2
    exit 1
fi

build_dir=$(mktemp -d "${TMPDIR:-/tmp}/ifj26-tests.XXXXXXXX") || exit 1
stderr_file="$build_dir/stderr.txt"

cleanup() {
    rm -f "$build_dir/lexer.exe" "$build_dir/parser.exe" \
        "$build_dir/semantic.exe" "$stderr_file"
    rmdir "$build_dir"
}
trap cleanup EXIT
trap 'exit 1' HUP INT TERM

build_suite() (
    executable=$2

    case "$1" in
        lexer)
            set -- src/lexer/main.c tests/lexer/lexer_exit.c
            ;;
        parser|semantic)
            suite=$1
            set -- src/main.c src/lexer/main.c src/parser/parser.c \
                src/parser/precedence.c src/parser/stack.c src/parser/ast.c
            if [ "$suite" = semantic ]; then
                set -- -Isrc/semantic -DIFJ_ENABLE_SEMANTICS "$@" \
                    symtable.c src/semantic/*.c
            fi
            ;;
    esac

    "$compiler" -std=gnu11 -Wall -Wextra -pedantic -Iinclude -I. "$@" -o "$executable"
)

run_case() {
    if command -v timeout >/dev/null 2>&1; then
        timeout 5 "$1"
    else
        "$1"
    fi
}

failed=0
for suite do
    printf '[%s]\n' "$suite"
    executable="$build_dir/$suite.exe"

    if build_suite "$suite" "$executable"; then
        :
    else
        build_status=$?
        printf 'Build failed (exit code %s).\n' "$build_status"
        failed=1
        continue
    fi

    total=0
    passed=0
    for source in "tests/$suite/"*.scala; do
        [ -f "$source" ] || continue
        total=$((total + 1))
        name=${source##*/}
        expected=${name%%_*}
        case "$expected" in
            ''|*[!0-9]*)
                printf 'FAIL %s (missing expected exit code in filename)\n' "$name"
                continue
                ;;
        esac

        run_case "$executable" < "$source" > /dev/null 2> "$stderr_file"
        actual=$?
        if [ "$actual" -eq "$expected" ]; then
            passed=$((passed + 1))
        elif [ "$actual" -eq 124 ]; then
            printf 'FAIL %s (timed out)\n' "$name"
        else
            printf 'FAIL %s (expected %s, got %s)\n' "$name" "$expected" "$actual"
            cat "$stderr_file"
        fi
    done

    if [ "$total" -eq 0 ]; then
        printf '%s: no .scala cases found\n' "$suite"
        failed=1
    else
        printf '%s: %s/%s passed\n' "$suite" "$passed" "$total"
        if [ "$passed" -ne "$total" ]; then
            failed=1
        fi
    fi
done

exit "$failed"
