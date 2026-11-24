#!/usr/bin/env python3
"""Create the flat ZIP layout required for IFJ26 submission.

AI assistance. User request: prepare the first trial submission archive
according to the assignment and check its required files.
"""

import argparse
from pathlib import Path
import re
import shutil
import subprocess
import tempfile
import zipfile


ROOT = Path(__file__).resolve().parent.parent
SOURCES = {
    "src/main.c": "src/main.c",
    "src/lexer/main.c": "src/lexer.c",
    "src/parser/parser.c": "src/parser.c",
    "src/parser/precedence.c": "src/precedence.c",
    "src/parser/stack.c": "src/stack.c",
    "src/parser/ast.c": "src/ast.c",
    "src/semantic/main.c": "src/semantic.c",
    "src/codegen/main.c": "src/codegen.c",
    "src/codegen/codegen_expr.c": "src/codegen_expr.c",
    "src/codegen/builtins_p.c": "src/builtins_p.c",
    "symtable.c": "src/symtable.c",
    "symtable.h": "src/symtable.h",
}

MAKEFILE = """# IFJ26 compiler submission build.
# AI assistance prompt: prepare a flat submission archive with a working Makefile.
CC = gcc
CPPFLAGS = -Isrc
CFLAGS = -std=gnu11 -Wall -Wextra -pedantic -O2

SOURCES = $(wildcard src/*.c)
HEADERS = $(wildcard src/*.h)
TARGET = ifj26

.PHONY: all clean
all: $(TARGET)

$(TARGET): $(SOURCES) $(HEADERS)
\t$(CC) $(CPPFLAGS) $(CFLAGS) $(SOURCES) -o $@

clean:
\t$(RM) $(TARGET) $(TARGET).exe
"""


def validate_distribution(path):
    """Require LF-terminated student ID, login and percentage records."""
    raw = path.read_bytes()
    if b"\r" in raw or not raw.endswith(b"\n"):
        raise ValueError("rozdeleni must use LF and end with LF")
    records = raw.decode("utf-8").splitlines()
    pattern = re.compile(r"([0-9]+):(x[a-z0-9]+):([0-9]{1,3})")
    matches = [pattern.fullmatch(record) for record in records]
    if len(records) not in (3, 4) or not all(matches):
        raise ValueError("invalid rozdeleni records")
    if sum(int(match[3]) for match in matches) != 100:
        raise ValueError("rozdeleni percentages must add up to 100")
    for group in (1, 2):
        if len({match[group] for match in matches}) != len(records):
            raise ValueError("duplicate student ID or login in rozdeleni")


def main():
    """Build the ZIP atomically and report any missing required documents."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", type=Path, default=ROOT / "xpospij01.zip")
    parser.add_argument("--clang-format", help="Path to clang-format, using LLVM style")
    args = parser.parse_args()
    output = args.output.resolve()
    output.parent.mkdir(parents=True, exist_ok=True)
    mapping = dict(SOURCES)
    mapping.update(
        (str(path.relative_to(ROOT)), "src/" + path.name)
        for path in sorted((ROOT / "include").glob("*.h"))
    )
    if len(set(mapping.values())) != len(mapping):
        raise ValueError("archive filename collision")

    missing = [name for name in ("dokumentace.pdf", "rozdeleni", "rozsireni")
               if not (ROOT / name).is_file()]
    distribution = ROOT / "rozdeleni"
    if distribution.is_file():
        validate_distribution(distribution)

    with tempfile.TemporaryDirectory(prefix="ifj26-package-") as directory:
        stage = Path(directory)
        for source, destination in mapping.items():
            content = (ROOT / source).read_text(encoding="utf-8-sig")
            if args.clang_format:
                content = subprocess.run(
                    [args.clang_format, "--style=LLVM",
                     "--assume-filename=" + Path(destination).name],
                    input=content, text=True, capture_output=True, check=True,
                    encoding="utf-8",
                ).stdout
            target = stage / destination
            target.parent.mkdir(parents=True, exist_ok=True)
            target.write_bytes(content.replace("\r\n", "\n").encode("utf-8"))
        (stage / "Makefile").write_bytes(MAKEFILE.encode("utf-8"))
        for name in ("dokumentace.pdf", "rozdeleni", "rozsireni"):
            source = ROOT / name
            if source.is_file():
                if name == "dokumentace.pdf" and not source.read_bytes().startswith(b"%PDF-"):
                    raise ValueError("dokumentace.pdf has no PDF header")
                shutil.copyfile(source, stage / name)

        pending = output.with_name(output.name + ".tmp")
        try:
            with zipfile.ZipFile(pending, "w", zipfile.ZIP_DEFLATED) as archive:
                for path in sorted(stage.rglob("*")):
                    if path.is_file():
                        archive.write(path, path.relative_to(stage).as_posix())
            pending.replace(output)
        finally:
            pending.unlink(missing_ok=True)

    print(output)
    print(f"{len(mapping)} source files, flat src/ directory, LF line endings")
    if args.clang_format:
        print("Source files formatted with clang-format --style=LLVM")
    if missing:
        print("DRAFT - required files still missing: " + ", ".join(missing))
    else:
        print("Required documents included; run the school is_it_ok.sh check")


if __name__ == "__main__":
    main()
