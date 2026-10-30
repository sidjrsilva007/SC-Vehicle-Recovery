"""Package this project's addon locally; no signatures or keys are included."""
from pathlib import Path
import argparse
import hashlib
import shutil
import struct

root = Path(__file__).resolve().parents[1]
source = root / "src" / "SC_VirarCarros"
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument("--check", action="store_true", help="Validate package inputs without writing output")
args = parser.parse_args()
entries = []
for path in sorted(source.rglob("*")):
    if path.is_file():
        if path.suffix.lower() not in {".cpp", ".c", ".xml", ".layout", ".paa", ".rvmat", ".p3d"}:
            raise SystemExit(f"Unexpected addon input: {path.name}")
        name = str(path.relative_to(source)).replace("/", "\\").encode("utf-8")
        entries.append((name, path.read_bytes()))
if not entries or not (source / "config.cpp").is_file():
    raise SystemExit("Missing addon configuration or source files")
body = bytearray(b"\0" + struct.pack("<5I", 0x56657273, 0, 0, 0, 0) + b"prefix\0SC_VirarCarros\0\0")
for name, data in entries:
    body += name + b"\0" + struct.pack("<5I", 0, 0, 0, 0, len(data))
body += b"\0" + bytes(20)
for _, data in entries:
    body += data
body += b"\0" + hashlib.sha1(body).digest()
if args.check:
    print(f"Package inputs OK: {len(entries)} files. Enforce compilation is not performed.")
else:
    mod = root / "dist" / "@SC_VehicleRecovery"
    output = mod / "addons" / "SC_VirarCarros.pbo"
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_bytes(body)
    shutil.copy2(root / "mod.cpp", mod / "mod.cpp")
    print(f"Local unsigned output: {output}")
