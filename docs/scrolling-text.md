# Scrolling text in the IKEA display app

Open the normal local IKEA app and find **Scrolling text** below Display Mode.
Enter a message, choose a size and speed, then press **Show text**. Text repeats
from right to left. **Stop text** blanks the text display; choosing another
Display Mode resumes that mode. The normal brightness and rotation controls
also apply to text. Live preview shows the display's presented frames.

- **Small:** 5 × 7 pixel glyphs, centered vertically.
- **Large:** the same glyphs doubled to 10 × 14 pixels, fitting the 16 × 16 panel.
- **Speed:** Slow (120 ms/pixel), Normal (60 ms/pixel), Fast (30 ms/pixel).
- **Message:** 1–120 printable ASCII characters with at least one non-space.
  Letters, numbers and punctuation work; emoji and accented letters are
  rejected rather than producing broken glyphs. Text is not interpreted as HTML.
- Settings and the message are held in RAM, readable again when the page is
  refreshed, and reset on a firmware reboot. This does not save message content
  with the existing Set as default mode button.

Showing text takes over from the active animation scheduler, like a manual
mode change; the saved animation schedule is not deleted. Brightness schedules
continue to affect the display. Home automation Off can still blank the LEDs
while the text renderer runs; use the normal brightness/schedule setup described
in the home automation integration guide.

## Installation

This feature includes both firmware and its embedded web app. Hosting the new
HTML alone, or refreshing the old device app, cannot add the renderer. Build the
**ESP32-S3 OTA firmware** workflow for the text branch and install that branch's
application binary using the [OTA upload guide](esp32s3-ota.md) when at home.
The text feature branch builds on the OTA preparation branch; merge the OTA
base first, then this feature. Hard-refresh the device page after installing.

The UI, native renderer checks and firmware are prepared remotely. No upload or
physical display validation is performed while the owner is away. The original
OTA artifact produced before this feature does not contain text controls.

## Implementation and API

`Text` is appended to the existing plugin list, preserving every earlier plugin
ID. Its renderer advances at most one scroll step per call and uses `millis()`;
it does not wait for an entire message or for NTP time. ESP32 settings are
protected by a mutex, and only the plugin's display loop writes screen pixels.
This keeps mode changes, Stop, the web server and OTA responsive between frames.

- `GET /api/text`: `{text, size, delay, playing}`.
- `POST /api/text`: JSON `{ "text": "Hello!", "size": 2, "delay": 60 }` validates,
  starts/restarts text and selects Text mode. Size must be 1 or 2, delay 20–250
  ms/pixel. JSON bodies are capped at 1024 bytes before allocation.
- `DELETE /api/text`: stop text. Other active display modes are unaffected.

Like the existing local control APIs, these endpoints are intended for the
trusted LAN. The OTA username/password only protect the firmware updater.

## Verification

`tests/scrolling_text_test.cpp` tests real shared validation and rendering
helpers: accepted/rejected inputs, exact 2× pixel scaling, clipping and scroll
wrap bounds. The dedicated firmware workflow runs these native tests, frontend
lint, TypeScript checks, the embedded frontend build and the ESP32-S3 firmware
build. Type checks allow the repository's existing enum syntax (its checked-in
`erasableSyntaxOnly` setting conflicts with `SYSTEM_STATUS`).

Browser verification uses the actual built UI with a local mock HTTP/WebSocket
display, including Show, size/speed selection, Stop, validation and mobile layout.
It does not establish the final physical LED appearance or runtime behavior on
the installed device; those checks remain for the first at-home upload.
