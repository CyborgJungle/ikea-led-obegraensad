"""Validate the OTA application and record the inputs needed to identify it."""
import csv
import hashlib
import json
import os
import struct
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
output = ROOT / "ota-artifact"
firmware = (output / "esp32s3_firmware.bin").read_bytes()


def size_bytes(value):
    value = value.strip().upper()
    if value.endswith("K"):
        return int(value[:-1], 0) * 1024
    if value.endswith("M"):
        return int(value[:-1], 0) * 1024 * 1024
    return int(value, 0)


rows = list(csv.reader(line for line in (ROOT / "partitions-4MB.csv").read_text().splitlines()
                       if line.strip() and not line.lstrip().startswith("#")))
slots = [size_bytes(row[4]) for row in rows
         if row[1].strip() == "app" and row[2].strip() in ("ota_0", "ota_1")]
if len(slots) != 2:
    raise SystemExit("Expected two OTA application slots")
if len(firmware) < 24 or firmware[0] != 0xE9:
    raise SystemExit("Not an ESP application image")
# ESP image header chip_id at offset 12: ESP32-S3 is 9.
if struct.unpack_from("<H", firmware, 12)[0] != 9:
    raise SystemExit("Refusing to package firmware for a chip other than ESP32-S3")
if len(firmware) > min(slots):
    raise SystemExit(f"Firmware is too large for the OTA slots: {len(firmware)} > {min(slots)}")
metadata = {
    "environment": "esp32s3",
    "board": "esp32-s3-devkitc-1",
    "source_commit": os.environ.get("SOURCE_COMMIT", "local-unrecorded"),
    "build_commit": os.environ.get("GITHUB_SHA", "local-unrecorded"),
    "run_url": f"{os.environ.get('GITHUB_SERVER_URL', 'https://github.com')}/{os.environ.get('GITHUB_REPOSITORY', '')}/actions/runs/{os.environ.get('GITHUB_RUN_ID', '')}",
    "firmware_bytes": len(firmware),
    "ota_slot_bytes": min(slots),
    "sha256": hashlib.sha256(firmware).hexdigest(),
    "partition_sha256": hashlib.sha256((ROOT / "partitions-4MB.csv").read_bytes()).hexdigest(),
    "webgui_sha256": hashlib.sha256((ROOT / "src/webgui.cpp").read_bytes()).hexdigest(),
    "credentials": "Repository defaults; see UPLOAD-GUIDE.md",
    "hardware_tested": False,
}
(output / "build-info.json").write_text(json.dumps(metadata, indent=2) + "\n")
print(json.dumps(metadata, indent=2))
