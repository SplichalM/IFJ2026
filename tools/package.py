#!/usr/bin/env python3
import argparse
from pathlib import Path
import re
import subprocess
import tempfile
import zipfile


ROOT = Path(__file__).resolve().parent.parent


def source_mapping():
    sources = sorted((ROOT / 'src').rglob('*.c'))
    sources += sorted((ROOT / 'src').rglob('*.h'))
    sources += sorted((ROOT / 'include').rglob('*.h'))
    sources += sorted(ROOT.glob('*.c'))
    sources += sorted(ROOT.glob('*.h'))
    mapping = {}
    destinations = set()
    for source in sources:
        relative = source.relative_to(ROOT).as_posix()
        destination = 'src/' + source.name
        if destination.lower() in destinations:
            raise ValueError('archive filename collision: ' + destination)
        destinations.add(destination.lower())
        mapping[relative] = destination
    if not mapping:
        raise ValueError('no source files found')
    return mapping


def archive_makefile(mapping):
    content = (ROOT / 'Makefile').read_text(encoding='utf-8-sig')
    # Expand source wildcards before flattening their paths.
    def expand(match):
        pattern = match.group(1).strip()
        if pattern.startswith('src/'):
            return ' '.join(mapping[path.relative_to(ROOT).as_posix()]
                            for path in sorted(ROOT.glob(pattern)))
        return match.group(0)
    content = re.sub(r'\$\(wildcard\s+([^()]+)\)', expand, content)
    # Replace each original path once, without replacing generated paths again.
    pattern = r'(?<![A-Za-z0-9_./-])(?:' + '|'.join(
        re.escape(path) for path in sorted(mapping, key=len, reverse=True)
    ) + r')(?![A-Za-z0-9_./-])'
    content = re.sub(pattern, lambda match: mapping[match.group(0)], content)
    content = content.replace('-Iinclude', '-Isrc')
    content = content.replace('$(wildcard include/*.h)', '$(wildcard src/*.h)')
    return content.encode('utf-8')


def validate_distribution(path):
    raw = path.read_bytes()
    if b'\r' in raw or not raw.endswith(b'\n'):
        raise ValueError('rozdeleni must use LF and end with LF')
    records = raw.decode('utf-8').splitlines()
    pattern = re.compile(r'([0-9]{5,7}):(x(?:[a-z]{5,6}[a-z0-9]{2}|[0-9]{5,7})):([0-9]{1,3})')
    matches = [pattern.fullmatch(record) for record in records]
    if len(records) not in (3, 4) or not all(matches):
        raise ValueError('invalid rozdeleni records')
    if sum(int(match[3]) for match in matches) != 100:
        raise ValueError('rozdeleni percentages must add up to 100')
    for group in (1, 2):
        if len({match[group] for match in matches}) != len(records):
            raise ValueError('duplicate student ID or login in rozdeleni')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, default=ROOT / 'xpospij01.zip')
    parser.add_argument('--clang-format', help='Optional path to clang-format')
    args = parser.parse_args()
    mapping = source_mapping()
    makefile = archive_makefile(mapping)
    documents = ('dokumentace.pdf', 'rozdeleni', 'rozsireni')
    missing = [name for name in documents if not (ROOT / name).is_file()]
    if (ROOT / 'rozdeleni').is_file():
        validate_distribution(ROOT / 'rozdeleni')
    output = args.output.resolve()
    output.parent.mkdir(parents=True, exist_ok=True)
    # Stage outside the repository; replace the output only after success.
    with tempfile.TemporaryDirectory(prefix='ifj26-package-') as directory:
        pending = Path(directory) / 'submission.zip'
        with zipfile.ZipFile(pending, 'w', zipfile.ZIP_DEFLATED) as archive:
            archive.writestr('Makefile', makefile)
            for source, destination in mapping.items():
                content = (ROOT / source).read_text(encoding='utf-8-sig')
                if args.clang_format:
                    content = subprocess.run(
                        [args.clang_format, '--style=LLVM', '--assume-filename=' + destination],
                        input=content, text=True, capture_output=True, check=True, encoding='utf-8'
                    ).stdout
                archive.writestr(destination, content.encode('utf-8'))
            for name in documents:
                source = ROOT / name
                if source.is_file():
                    if name == 'dokumentace.pdf' and not source.read_bytes().startswith(b'%PDF-'):
                        raise ValueError('dokumentace.pdf has no PDF header')
                    archive.write(source, name)
        # A temporary sibling allows replacement across different temp drives.
        sibling = output.with_name(output.name + '.tmp')
        try:
            sibling.write_bytes(pending.read_bytes())
            sibling.replace(output)
        finally:
            if sibling.exists():
                sibling.unlink()
    print(output)
    print(str(len(mapping)) + ' source files; project Makefile with archive paths')
    if missing:
        print('DRAFT - required files missing: ' + ', '.join(missing))
    else:
        print('Required documents included; run tools/is_it_ok.sh')


if __name__ == '__main__':
    main()
