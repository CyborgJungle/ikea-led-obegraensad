# ESP32-S3 DevKitC / N16R8: build now, upload at home

Repository: https://github.com/CyborgJungle/ikea-led-obegraensad
(parent: ph1p/ikea-led-obegraensad). The reported working firmware used
PlatformIO environment `esp32s3` and board `esp32-s3-devkitc-1`.
The fork's original `esp32s3` environment selected `seeed_xiao_esp32s3`.
This branch corrects that board selection and pins its pioarduino platform to
55.03.312-1 (the same archive as `stable` when this guide was prepared).

## What is already implemented

- `include/constants.h` enables `ENABLE_SERVER`.
- `src/main.cpp` calls `initOTA(server)` before starting the HTTP server and
  calls `ElegantOTA.loop()` in the main loop.
- `src/ota.cpp` configures ElegantOTA with authentication and automatic reboot.
- HTTP port: **80**. Browser UI: **`/update`**. ElegantOTA's transfer protocol
  uses **`/ota/start`** and **`/ota/upload`**, not a raw binary POST to `/update`.
- Credentials come from `include/secrets.h`. Repository defaults are
  **admin / ikea-led-wall**. The installed firmware's credentials may differ.
- No ArduinoOTA service is configured. PlatformIO's `espota` protocol cannot
  be substituted for this browser workflow.
- The web app is gzip-compressed into `src/webgui.cpp`; it is compiled into
  the application. A firmware OTA also updates the embedded web app. It does
  **not** require a filesystem upload.

These are source findings; they do not establish which source, settings or
partition table are currently installed on the display.

## Build and download while away

1. Open the fork's **Actions** tab. If GitHub shows that workflows are disabled
   on this new fork, enable Actions for the fork.
2. Open **ESP32-S3 OTA firmware**. The preparation branch and pull request
   trigger it automatically. After merge it also runs for pushes to `main`;
   **Run workflow** becomes available from the default branch and lets you
   choose a branch for later builds.
3. Select a successful run for the exact commit you intend to install. Download
   the **esp32s3-ota-RUN-ATTEMPT** artifact at the bottom of the run page while
   signed into GitHub. Unzip it. Artifacts expire after 90 days; save a copy.
4. The upload file is **`esp32s3_firmware.bin`**. Keep `SHA256SUMS`,
   `build-info.json`, `packages.txt`, the partition CSV and this guide with it.
   On macOS verify with `shasum -a 256 -c SHA256SUMS` from the unzipped folder;
   on Linux use `sha256sum -c SHA256SUMS`.

The workflow builds the locked frontend first, then compiles `pio run -e
esp32s3`. Packaging rejects an image whose chip ID is not ESP32-S3 or whose
size exceeds either OTA slot. It records source/build commits, binary checksum,
partition checksum, web asset checksum and resolved package versions. PR builds
use GitHub's merge commit; both the PR source and build commit are recorded.

Use this dedicated workflow's artifact. The inherited **Build and Release**
workflow still has an optional S3 job and tag-only S3 artifacts; it is not the
recommended download path. No release or tag is needed for this workflow.

Dependencies are installed by CI, never updated with `pio upgrade` or `pio pkg
update`. The S3 platform, PlatformIO CLI, pnpm and frontend lockfile are pinned.
Arduino library version ranges still resolve at build time; `packages.txt`
records them. Save the actual tested artifact rather than assuming a future
rebuild will be byte-identical.

## Compatibility gate before the first upload

- Keep a copy of the exact known-working firmware binary and original build
  configuration if available. Neither can be recovered from the fork alone.
- Confirm the working build uses `partitions-4MB.csv`: two **1856 KiB
  (1,900,544-byte)** OTA app slots, at 0x10000 and 0x1e0000, plus NVS,
  OTA metadata and the existing data partitions. OTA writes the inactive app
  slot; it does not install the partition CSV or a new bootloader. If the
  installed layout is unknown, stop and recover the original configuration
  or inspect the partition table over USB before the first OTA.
- This build retains the DevKitC board's conservative flash/memory defaults
  and the 4 MiB partition layout. It deliberately does not enable the N16R8's
  8 MiB PSRAM or expand partitions to fill its 16 MiB flash. Those changes need
  separate hardware validation and potentially a USB reflash.
- Check the physical wiring against `include/constants.h`. The current S3
  branch uses **EN=GPIO5, DATA=GPIO9, CLOCK=GPIO7, LATCH=GPIO6,
  BUTTON=GPIO4**. Changing the board name does not change these raw GPIO
  assignments. If the known-working firmware used different pins, port those
  assignments before building/uploading. Do not guess them remotely.
- Have local power control and a USB data cable available for recovery. A
  successful compile is not a hardware test. Two OTA slots alone do not
  guarantee automatic rollback after a bad application boots.

## Upload when home, on the display's network

1. Join the same reachable home LAN as the display (avoid an isolated guest
   network). Keep the display powered steadily throughout the update.
2. Open **http://ikea-led.local/update**. If mDNS fails, find the current IP
   in your router's DHCP list and open **http://DEVICE-IP/update**. An IP
   mentioned in earlier conversations may have changed.
3. Log in with credentials in the **currently installed** firmware. Try
   `admin` / `ikea-led-wall` only if defaults were retained. Editing the
   repository credentials cannot unlock a different installed password.
4. Choose **Firmware** mode and select **`esp32s3_firmware.bin`** from the
   successful dedicated workflow artifact. Do not upload a ZIP, generic
   `esp32dev` image, bootloader, partition table, filesystem or merged/factory
   image. Do not choose filesystem mode.
5. Start the update and leave the page and power connected until it reports
   success and the board reboots. The display shows U during upload; the
   current code also draws R on completion, including its failure path, so
   the letter alone is not proof of success.
6. Reopen the normal app, hard-refresh to clear cached UI, and verify matrix
   output, brightness, buttons, saved settings and Wi-Fi reconnection. Open
   `/update` again to confirm OTA remains available.
7. This public artifact retains the repository's default OTA credentials.
   After reboot, those defaults apply even if the previous firmware had a
   different password. Do not expose port 80 or the update endpoints to the
   internet. For a private password, build locally with a private edited
   `include/secrets.h`; this file is tracked despite a gitignore entry, so
   never commit your password. Public downloadable binaries are not a safe
   place to distribute a private credential.

If the uploader reports an error, do not assume a reboot or R means success.
Check the error and whether the old application is still reachable. If the new
app boots but malfunctions and `/update` still works, upload the saved
known-working **S3 application** binary through the same procedure. If Wi-Fi
or OTA no longer starts, recover via USB with the known-working board settings
and layout. Avoid a full flash erase unless deliberately restoring from backup,
as it removes saved Wi-Fi/settings. A missing `/update` page can mean a different
installed build; it may require a one-time USB flash to establish OTA support.

## Scope while away

Remote preparation can inspect code, build, review, merge and download artifacts.
CI never contacts the display and never uploads firmware. Opening a PR or merging
it does not update the hardware. This is a manual LAN push workflow; the display
does not poll GitHub for updates. This preparation does not set up remote access.
The first endpoint, credentials, wiring/layout check, firmware upload and physical
validation wait until someone is at home with recovery access.

## Optional local build

Use Python 3.10 or newer, PlatformIO 6.2.0, Node 24 and pnpm 12.8.1:

```sh
cd frontend
pnpm install --frozen-lockfile
pnpm build
cd ..
pio run -e esp32s3
```

The resulting application is `.pio/build/esp32s3/firmware.bin`.
`pio run` compiles only; do not add `-t upload` while preparing remotely.
The inherited `upload.py` is not needed for browser uploads. It contains legacy
URL handling (including a possible doubled `/update`), auto-installs Python
packages and lacks request timeouts; it has not been hardware-validated here.
Leave its commented custom-upload options disabled for this first update.
