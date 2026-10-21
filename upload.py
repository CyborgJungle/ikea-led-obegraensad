# Uploads the firmware over WiFi to the lamp's /api/firmware endpoint (see src/ota.cpp).
#
# In platformio.ini:
#
#   extra_scripts = upload.py
#   upload_protocol = custom
#   custom_upload_url = http://ikea-led.local
#   custom_username = admin          ; OTA_USERNAME in include/secrets.h
#   custom_password = ikea-led-wall  ; OTA_PASSWORD in include/secrets.h

import hashlib

import requests
from requests.auth import HTTPDigestAuth

Import("env")

try:
    from requests_toolbelt import MultipartEncoder, MultipartEncoderMonitor
    from tqdm import tqdm
except ImportError:
    env.Execute("$PYTHONEXE -m pip install requests_toolbelt tqdm")
    from requests_toolbelt import MultipartEncoder, MultipartEncoderMonitor
    from tqdm import tqdm


def on_upload(source, target, env):
    firmware_path = str(source[0])
    endpoint = env.GetProjectOption("custom_upload_url").rstrip("/") + "/api/firmware"
    auth = HTTPDigestAuth(
        env.GetProjectOption("custom_username"), env.GetProjectOption("custom_password")
    )

    # the first request answers the digest challenge, the upload then reuses it
    check = requests.get(endpoint, auth=auth, timeout=15)
    if check.status_code == 401:
        return "Wrong OTA username or password (custom_username / custom_password)"
    if check.status_code != 200:
        return f"Lamp not ready for an update: HTTP {check.status_code} {check.text}"

    with open(firmware_path, "rb") as firmware:
        md5 = hashlib.md5(firmware.read()).hexdigest()
        firmware.seek(0)
        encoder = MultipartEncoder(
            fields={"firmware": ("firmware.bin", firmware, "application/octet-stream")}
        )
        bar = tqdm(desc="Upload", total=encoder.len, unit="B", unit_scale=True, unit_divisor=1024)
        monitor = MultipartEncoderMonitor(encoder, lambda m: bar.update(m.bytes_read - bar.n))
        response = requests.post(
            endpoint,
            data=monitor,
            headers={"Content-Type": monitor.content_type, "X-MD5": md5},
            auth=auth,
            timeout=120,
        )
        bar.close()

    if response.status_code != 200:
        return f"Upload failed: HTTP {response.status_code} {response.text}"
    print("Upload successful, the lamp restarts into the new firmware.")


env.Replace(UPLOADCMD=on_upload)
