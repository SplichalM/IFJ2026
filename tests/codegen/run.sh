#!/bin/sh

compiler=${IFJ_COMPILER:-./ifj26}
if [ -z "${IFJ_COMPILER:-}" ] && [ -x ./ifj26.exe ]; then
    compiler=./ifj26.exe
fi

interpreter=${IFJ_INTERPRETER:-}
if [ -z "$interpreter" ]; then
    case "$(uname -s):$(uname -m)" in
        MINGW*|MSYS*|CYGWIN*) interpreter=./tools/ic26int-windows-x86_64.exe ;;
        Linux:x86_64) interpreter=./tools/ic26int-linux-x86_64 ;;
        Linux:aarch64) interpreter=./tools/ic26int-linux-aarch64 ;;
    esac
    if [ ! -x "$interpreter" ] && [ -x /pub/courses/ifj/ic26int/linux/ic26int ]; then
        interpreter=/pub/courses/ifj/ic26int/linux/ic26int
    fi
fi

if ! command -v "$compiler" >/dev/null 2>&1; then
    printf 'Compiler not found: %s. Build it with make or set IFJ_COMPILER.\n' "$compiler" >&2
    exit 1
fi
if [ -z "$interpreter" ] || ! command -v "$interpreter" >/dev/null 2>&1; then
    printf 'Interpreter not found: %s. Put it in tools/ or set IFJ_INTERPRETER.\n' "$interpreter" >&2
    exit 1
fi

work_dir=$(mktemp -d "${TMPDIR:-/tmp}/ifj26-codegen.XXXXXXXX") || exit 1
program="$work_dir/program.ifjcode"
actual="$work_dir/actual.out"
trap 'rm -f "$program" "$actual" "$work_dir/normalized.out"; rmdir "$work_dir"' EXIT
trap 'exit 1' HUP INT TERM

# Stop an individual test if the compiler or generated program does not finish.
run_case() {
    if command -v timeout >/dev/null 2>&1; then
        timeout -k 1 5 "$@"
    else
        "$@"
    fi
}

total=0
passed=0
printf '[codegen]\n'
for source in tests/codegen/*.scala; do
    [ -f "$source" ] || continue
    total=$((total + 1))
    name=${source##*/}
    expected_status=${name%%_*}
    expected_output=${source%.scala}.out
    input=${source%.scala}.in
    [ -f "$input" ] || input=/dev/null
    if [ ! -f "$expected_output" ]; then
        printf 'FAIL %s (missing .out file)\n' "$name"
        continue
    fi

    run_case "$compiler" < "$source" > "$program"
    status=$?
    if [ "$status" -ne 0 ]; then
        printf 'FAIL %s (compiler exit %s)\n' "$name" "$status"
        continue
    fi
    if [ ! -s "$program" ]; then
        printf 'FAIL %s (compiler produced no IFJcode26)\n' "$name"
        continue
    fi

    run_case "$interpreter" "$program" < "$input" > "$actual"
    status=$?
    if [ "$status" != "$expected_status" ]; then
        printf 'FAIL %s (interpreter exit %s, expected %s)\n' "$name" "$status" "$expected_status"
        continue
    fi
    # Undo Windows stdout newline conversion before comparing with the fixtures.
    case "$(uname -s)" in
        MINGW*|MSYS*|CYGWIN*)
            carriage_return=$(printf '\r')
            line=
            while IFS= read -r line; do
                printf '%s\n' "${line%"$carriage_return"}"
            done < "$actual" > "$work_dir/normalized.out"
            printf '%s' "$line" >> "$work_dir/normalized.out"
            mv "$work_dir/normalized.out" "$actual"
            ;;
    esac

    if ! cmp -s "$expected_output" "$actual"; then
        printf 'FAIL %s (output differs)\n' "$name"
        diff -u "$expected_output" "$actual"
        continue
    fi
    passed=$((passed + 1))
done

printf 'Codegen: %s/%s passed\n' "$passed" "$total"
[ "$total" -gt 0 ] && [ "$passed" -eq "$total" ]