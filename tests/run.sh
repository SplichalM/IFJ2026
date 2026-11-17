#!/bin/sh

usage() {
    printf 'Usage: sh tests/run.sh [lexer|parser|semantic|codegen|all]\n' >&2
    exit 2
}

[ "$#" -le 1 ] || usage
case "${1-all}" in
    lexer|parser|semantic|codegen) ;;
    all) set -- lexer parser semantic ;;
    *) usage ;;
esac

repo_dir=$(CDPATH= cd "$(dirname "$0")/.." && pwd) || exit 1
cd "$repo_dir" || exit 1

if [ "$1" = codegen ]; then
    exec sh tests/codegen/run.sh
fi

compiler=${CC:-gcc}
qt_compiler=/c/Qt/Tools/mingw1120_64/bin/gcc.exe
if [ -z "${CC:-}" ] && ! command -v "$compiler" >/dev/null 2>&1 && [ -x "$qt_compiler" ]; then
    compiler=$qt_compiler
fi
if ! command -v "$compiler" >/dev/null 2>&1; then
    printf 'Compiler not found: %s. Set CC to its path.\n' "$compiler" >&2
    exit 1
fi

build_dir=$(mktemp -d "${TMPDIR:-/tmp}/ifj26-tests.XXXXXXXX") || exit 1
stdout_file="$build_dir/stdout.txt"
stderr_file="$build_dir/stderr.txt"
timeout_path=$(command -v timeout)

cleanup() {
    rm -f "$build_dir"/*.exe "$stdout_file" "$stderr_file" "$build_dir/lexer-input.txt"
    rmdir "$build_dir"
}
trap cleanup EXIT
trap 'exit 1' HUP INT TERM

build() {
    target=$1
    shift
    if "$compiler" -std=gnu11 -Wall -Wextra -pedantic -Iinclude -I. "$@" -o "$build_dir/$target.exe"; then
        return 0
    fi
    printf 'FAIL build %s\n' "$target"
    failed=1
    return 1
}

build_parser() {
    build "$@" src/parser/parser.c src/parser/precedence.c src/parser/stack.c src/parser/ast.c
}

run_case() {
    if [ -n "$timeout_path" ]; then
        "$timeout_path" 5 "$@"
    else
        "$@"
    fi
}

check() {
    expected=$1
    label=$2
    shift 2
    total=$((total + 1))
    run_case "$@" > "$stdout_file" 2> "$stderr_file"
    actual=$?
    if [ "$actual" -eq "$expected" ]; then
        passed=$((passed + 1))
    else
        printf 'FAIL %s (expected %s, got %s)\n' "$label" "$expected" "$actual"
        cat "$stderr_file"
        failed=1
    fi
}

build_main() {
    case "$1" in
        lexer) build lexer src/lexer/main.c tests/lexer/lexer_exit.c ;;
        parser) build_parser parser src/main.c src/lexer/main.c ;;
        semantic) build_semantic semantic src/main.c ;;
    esac
}

build_semantic() {
    build_parser "$@" src/lexer/main.c -Isrc/semantic -DIFJ_ENABLE_SEMANTICS symtable.c src/semantic/*.c
}

run_suite() (
    suite=$1
    executable="$build_dir/$suite.exe"
    failed=0
    total=0
    passed=0

    printf '[%s]\n' "$suite"
    build_main "$suite" || exit 1

    for source in "tests/$suite/"*.scala; do
        [ -f "$source" ] || continue
        name=${source##*/}
        code=${name%%_*}
        case "$code" in
            ''|*[!0-9]*)
                printf 'FAIL %s (missing expected exit code in filename)\n' "$name"
                total=$((total + 1))
                failed=1
                ;;
            *) check "$code" "$name" "$executable" < "$source" ;;
        esac
    done

    if [ "$total" -eq 0 ]; then
        printf 'FAIL no .scala cases found\n'
        failed=1
    fi
    printf 'Exit codes: %s/%s passed\n' "$passed" "$total"

    if [ -f "./tests/deep/$suite/checks.sh" ]; then
        . "./tests/deep/$suite/checks.sh" || exit 1
        total=0
        passed=0
        run_direct_checks
        printf 'Direct checks: %s/%s passed\n' "$passed" "$total"
    fi
    exit "$failed"
)

result=0
for suite do
    run_suite "$suite" || result=1
done
exit "$result"
