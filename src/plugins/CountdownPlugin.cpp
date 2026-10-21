#include "plugins/CountdownPlugin.h"
#include "config.h"
#include "text_render.h"

namespace
{
// one ring travels from the centre past the corners in this time
constexpr unsigned long RIPPLE_PERIOD = 1600;
constexpr float RIPPLE_REACH = 12.0f;
constexpr unsigned long FRAME_INTERVAL = 33;
// how long the rings show that the time is up, before the next countdown can start
constexpr unsigned long FINISHED_ANIMATION = 6000;

void drawCentered(const PixelFont &font, const String &text, int y)
{
  const GlyphLine line = TextRender::decode(font, text)[0];
  TextRender::draw(font, line, (COLS - TextRender::width(font, line)) / 2, y);
}

String twoDigits(int value)
{
  return value < 10 ? "0" + String(value) : String(value);
}
} // namespace

void CountdownPlugin::wait()
{
  loadedReset = config.getCountdownReset();
  phase = WAITING;
  drawRemaining(config.getCountdownSeconds());
}

void CountdownPlugin::start()
{
  loadedRun = config.getCountdownRun();
  durationMillis = config.getCountdownSeconds() * 1000UL;
  startMillis = millis();
  shownSeconds = -1;
  phase = RUNNING;
}

void CountdownPlugin::setup()
{
  // starts made before switching to this mode do not count
  loadedRun = config.getCountdownRun();
  wait();
}

void CountdownPlugin::drawRemaining(int seconds)
{
  Screen.clear();

  // up to 99 seconds: big digits
  if (seconds < 100)
  {
    drawCentered(fontLarge, String(seconds), 0);
    return;
  }

  const int minutes = seconds / 60;
  const String clock = String(minutes) + ":" + twoDigits(seconds % 60);
  const GlyphLine line = TextRender::decode(fontSmall, clock)[0];
  if (TextRender::width(fontSmall, line) <= COLS)
  {
    drawCentered(fontSmall, clock, (ROWS - fontSmall.lineHeight) / 2);
    return;
  }

  // 10 minutes and more: minutes above seconds
  drawCentered(fontSmall, twoDigits(minutes), 0);
  drawCentered(fontSmall, twoDigits(seconds % 60), fontSmall.lineHeight);
}

void CountdownPlugin::drawFinished(unsigned long elapsed)
{
  Screen.clear();

  // two rings half a period apart, fading as they grow
  for (int ring = 0; ring < 2; ring++)
  {
    const float phase =
        ((elapsed + ring * RIPPLE_PERIOD / 2) % RIPPLE_PERIOD) / (float)RIPPLE_PERIOD;
    const float radius = phase * RIPPLE_REACH;
    const float fade = 1.0f - phase;

    for (int y = 0; y < ROWS; y++)
    {
      for (int x = 0; x < COLS; x++)
      {
        const float dx = x - (COLS - 1) / 2.0f;
        const float dy = y - (ROWS - 1) / 2.0f;
        const float distance = sqrtf(dx * dx + dy * dy);
        const float intensity = 1.0f - fabsf(distance - radius);
        if (intensity > 0)
        {
          const uint8_t brightness = MAX_BRIGHTNESS * intensity * fade;
          if (brightness > 0)
          {
            Screen.setPixel(x, y, 1, brightness);
          }
        }
      }
    }
  }
}

void CountdownPlugin::loop()
{
  // a new duration or reset first: a start sent together with it still starts
  if (loadedReset != config.getCountdownReset())
  {
    wait();
  }
  if (loadedRun != config.getCountdownRun())
  {
    start();
  }

  const unsigned long now = millis();
  switch (phase)
  {
  case WAITING:
    break;

  case RUNNING:
  {
    const unsigned long elapsed = now - startMillis;
    if (elapsed >= durationMillis)
    {
      phase = FINISHED;
      finishedMillis = now;
      break;
    }
    // round up: show 1 during the last second, 0 never
    const int seconds = (durationMillis - elapsed + 999) / 1000;
    if (seconds != shownSeconds)
    {
      shownSeconds = seconds;
      drawRemaining(seconds);
    }
    break;
  }

  case FINISHED:
    if (now - finishedMillis >= FINISHED_ANIMATION)
    {
      wait();
    }
    else if (now - lastFrame >= FRAME_INTERVAL)
    {
      lastFrame = now;
      drawFinished(now - finishedMillis);
    }
    break;
  }
}

const char *CountdownPlugin::getName() const
{
  return "Countdown";
}
