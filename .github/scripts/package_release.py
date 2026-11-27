import argparse
import re
import stat
import sys
import zipfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
PACKAGING = ROOT / "packaging"
NOTICES = [ROOT / "LICENSE", ROOT / "THIRD_PARTY_LICENSES"]
EXECUTABLE = 0o755
REGULAR = 0o644


def add_file(archive, source, name, mode):
    info = zipfile.ZipInfo.from_file(source, name)
    info.external_attr = (stat.S_IFREG | mode) << 16
    info.compress_type = zipfile.ZIP_DEFLATED
    with open(source, "rb") as data:
        archive.writestr(info, data.read(), compresslevel=9)


def add_text(archive, text, name):
    info = zipfile.ZipInfo(name, date_time=(1980, 1, 1, 0, 0, 0))
    info.external_attr = (stat.S_IFREG | REGULAR) << 16
    info.compress_type = zipfile.ZIP_DEFLATED
    archive.writestr(info, text, compresslevel=9)


def add_notices(archive):
    for notice in NOTICES:
        add_file(archive, notice, notice.name, REGULAR)


def bundle_version(version):
    return re.split(r"[-+]", version, maxsplit=1)[0]


def binary(build_dir, name, platform):
    path = build_dir / (name + ".exe" if platform == "windows" else name)
    if not path.is_file():
        sys.exit(f"{path} not found")
    return path


def package_editor(build_dir, out, version, platform):
    stem = f"atmo_{version}_{platform}"
    executable = binary(build_dir, "atmo", platform)

    with zipfile.ZipFile(out / f"{stem}.zip", "w") as archive:
        if platform == "windows":
            add_file(archive, executable, f"{stem}.exe", EXECUTABLE)
        elif platform == "macos":
            contents = f"{stem}.app/Contents"
            plist = (PACKAGING / "macos" / "Info.plist.in").read_text().replace("@VERSION@", bundle_version(version))
            add_text(archive, plist, f"{contents}/Info.plist")
            add_text(archive, "APPL????", f"{contents}/PkgInfo")
            add_file(archive, executable, f"{contents}/MacOS/atmo", EXECUTABLE)
            add_file(archive, PACKAGING / "macos" / "atmo.icns", f"{contents}/Resources/atmo.icns", REGULAR)
        else:
            add_file(archive, executable, stem, EXECUTABLE)
            add_file(archive, PACKAGING / "icons" / "atmo-256.png", "atmo.png", REGULAR)
            add_file(archive, PACKAGING / "linux" / "atmo.desktop", "atmo.desktop", REGULAR)
        add_notices(archive)


def package_export(build_dir, out, version, platform):
    stem = f"atmo-export_{version}_{platform}"
    executable = binary(build_dir, "atmo-export", platform)

    with zipfile.ZipFile(out / f"{stem}.zip", "w") as archive:
        add_file(archive, executable, stem + (".exe" if platform == "windows" else ""), EXECUTABLE)
        add_notices(archive)


def main():
    parser = argparse.ArgumentParser(description="Package the Atmo release zips of one platform.")
    parser.add_argument("--version", required=True)
    parser.add_argument("--platform", required=True, choices=["windows", "macos", "linux"])
    parser.add_argument("--build-dir", required=True, type=Path)
    parser.add_argument("--out", required=True, type=Path)
    args = parser.parse_args()

    args.out.mkdir(parents=True, exist_ok=True)
    package_editor(args.build_dir, args.out, args.version, args.platform)
    package_export(args.build_dir, args.out, args.version, args.platform)

    for archive in sorted(args.out.glob("*.zip")):
        print(f"{archive.name}: {archive.stat().st_size / 1_000_000:.2f} MB")


if __name__ == "__main__":
    main()
