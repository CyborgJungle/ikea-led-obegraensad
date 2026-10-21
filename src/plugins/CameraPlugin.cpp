#include "plugins/CameraPlugin.h"

// frames arrive over the HTTPS server, which only exists on ESP32
#ifdef ESP32

#include <WiFi.h>

namespace
{
portMUX_TYPE frameMux = portMUX_INITIALIZER_UNLOCKED;
uint8_t latestFrame[TOTAL_PIXELS];
volatile uint32_t frameCount = 0;
volatile unsigned long lastFrameMillis = 0;

// no frame for this long: the phone stopped sending
constexpr unsigned long IDLE_AFTER = 3000;

const char *const IDLE_ICON[] = {
    "................",
    "................",
    "................",
    "......####......",
    "..############..",
    "..#..........#..",
    "..#....##....#..",
    "..#...#..#...#..",
    "..#...#..#...#..",
    "..#....##....#..",
    "..#..........#..",
    "..############..",
    "................",
    "................",
    "................",
    "................",
};
constexpr uint8_t IDLE_BRIGHTNESS = 60;

// Camera images light most of the matrix at once; at full brightness that,
// together with the constant WiFi traffic, browned out the lamp. Frames are
// dimmed so the LEDs together never draw more than this share of all on.
constexpr uint32_t POWER_BUDGET = (uint32_t)TOTAL_PIXELS * MAX_BRIGHTNESS * 35 / 100;

void limitPower(uint8_t *frame)
{
  uint32_t load = 0;
  for (int i = 0; i < TOTAL_PIXELS; i++)
  {
    load += frame[i];
  }
  load = load * Screen.getCurrentBrightness() / MAX_BRIGHTNESS;
  if (load <= POWER_BUDGET)
  {
    return;
  }
  const uint32_t scale = POWER_BUDGET * 256 / load;
  for (int i = 0; i < TOTAL_PIXELS; i++)
  {
    frame[i] = frame[i] * scale / 256;
  }
}
} // namespace

void CameraPlugin::receiveFrame(const uint8_t *pixels)
{
  portENTER_CRITICAL(&frameMux);
  memcpy(latestFrame, pixels, TOTAL_PIXELS);
  frameCount++;
  lastFrameMillis = millis();
  portEXIT_CRITICAL(&frameMux);
}

void CameraPlugin::drawIdle()
{
  Screen.clear();
  for (int y = 0; y < ROWS; y++)
  {
    for (int x = 0; x < COLS; x++)
    {
      if (IDLE_ICON[y][x] == '#')
      {
        Screen.setPixel(x, y, 1, IDLE_BRIGHTNESS);
      }
    }
  }
  idleShown = true;
}

void CameraPlugin::setup()
{
  shownFrame = frameCount;
  drawIdle();
  // with power saving the radio sleeps between beacons and a frame can wait
  // ~100 ms for it to wake; the stream needs it awake, the other modes do not
  wifiSleepBefore = WiFi.getSleep();
  WiFi.setSleep(false);
}

void CameraPlugin::teardown()
{
  WiFi.setSleep(wifiSleepBefore);
}

void CameraPlugin::loop()
{
  if (frameCount != shownFrame)
  {
    uint8_t frame[TOTAL_PIXELS];
    portENTER_CRITICAL(&frameMux);
    memcpy(frame, latestFrame, TOTAL_PIXELS);
    shownFrame = frameCount;
    portEXIT_CRITICAL(&frameMux);

    limitPower(frame);
    for (int i = 0; i < TOTAL_PIXELS; i++)
    {
      Screen.setPixel(i % COLS, i / COLS, frame[i] > 0, frame[i]);
    }
    idleShown = false;
  }
  else if (!idleShown && millis() - lastFrameMillis > IDLE_AFTER)
  {
    drawIdle();
  }
}

const char *CameraPlugin::getName() const
{
  return "Camera";
}

#endif
