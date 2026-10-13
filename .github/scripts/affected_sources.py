import os
import re
import subprocess
import sys


def main():
    scan_deps, changed = sys.argv[1], {os.path.realpath(f) for f in sys.argv[2:]}
    src = os.path.realpath("src") + os.sep

    result = subprocess.run(
        [scan_deps, "-compilation-database", "compile_commands.json", "-format", "make", "-j", str(os.cpu_count())],
        capture_output=True,
        text=True,
    )
    if not result.stdout.strip():
        sys.exit(result.stderr or f"{scan_deps} found no dependencies")

    affected = set()

    for rule in result.stdout.replace("\\\n", " ").splitlines():
        _, _, deps = rule.partition(":")
        deps = [os.path.realpath(dep) for dep in deps.split()]
        if deps and not changed.isdisjoint(deps):
            affected.add(deps[0])

    for source in re.findall(r"^Diagnostics while scanning dependencies for '(.*)':$", result.stderr, re.MULTILINE):
        affected.add(os.path.realpath(source))

    for source in sorted(affected):
        if source.startswith(src):
            print(os.path.relpath(source))


if __name__ == "__main__":
    main()
