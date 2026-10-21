#include "plugins/CameraPlugin.h"

// frames arrive over the HTTPS server, which only exists on ESP32
#ifdef ESP32

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
