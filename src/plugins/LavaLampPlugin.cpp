#include "plugins/LavaLampPlugin.h"
#include "constants.h"
#include "mode_clock.h"
#include "screen.h"
#include <math.h>

namespace
{
// an LED cell is about 1.5 times taller than wide, as in Blobs: y is scaled so
// round wax looks round on the lamp
constexpr float ASPECT = 1.5f;
constexpr float Y_MAX = ROWS * ASPECT;

// wax resting on the bottom, or this close above it, sits on the heater and warms up
constexpr float HEAT_DEPTH = 1.6f;
constexpr float HEAT = 0.22f; // per second
// cooling per second: little low down, up to seven times as much at the cold top
constexpr float COOL = 0.02f;
// warmer than this rises, cooler sinks
constexpr float NEUTRAL = 0.45f;
constexpr float RISE = 1.6f; // units per second per degree from neutral
// seconds for the wax's speed to follow its buoyancy: thick and slow
constexpr float VISCOSITY = 1.2f;
constexpr float WOBBLE = 0.06f; // sideways drift, radians per second
constexpr float DRIFT = 2.5f;   // columns either side of home

// wax rests against the bottom and the top this far in, as a part of its radius
constexpr float REST = 0.6f;
// moving wax gets taller and thinner, per unit per second
constexpr float STRETCH = 0.45f;

// field values of the soft edge: off below, full brightness above
constexpr float EDGE_LO = 0.05f;
constexpr float EDGE_HI = 0.95f;
// dimmer LEDs flicker, they stay off
constexpr uint8_t FLOOR = 10;

constexpr unsigned long FRAME_MS = 40;
// the most mode time one frame moves on: 0.1 s at the fastest speed (4x)
constexpr float MAX_STEP_S = 0.4f;
constexpr float TWO_PI_F = 6.2831853f;

float randomBetween(float low, float high)
{
  return low + (high - low) * static_cast<float>(random(0, 10001)) / 10000.0f;
}

float smoothstep(float low, float high, float value)
{
  float t = (value - low) / (high - low);
  t = t < 0 ? 0 : (t > 1 ? 1 : t);
  return t * t * (3 - 2 * t);
}

float wrap(float phase)
{
  return phase >= TWO_PI_F ? phase - TWO_PI_F : phase;
}

// warm wax swells a little
float radius(float size, float temperature)
{
  return size * (0.85f + 0.25f * fminf(1.0f, temperature));
}
} // namespace

void LavaLampPlugin::setup()
{
  Screen.clear();
  // the screen was cleared, so the diff against the last frame is stale
  memset(previousBrightness, 0, sizeof(previousBrightness));

  // spread over the width and over the cycle, so not all wax moves together
  for (uint8_t i = 0; i < NUM_WAX; i++)
  {
    Wax &w = wax[i];
    w.home = 2.5f + i * (11.0f / (NUM_WAX - 1)) + randomBetween(-1, 1);
    w.phase = randomBetween(0, TWO_PI_F);
    w.wobble = WOBBLE * randomBetween(0.7f, 1.3f);
    w.size = randomBetween(3.9f, 4.9f);
    w.heat = HEAT * randomBetween(0.8f, 1.2f);
    w.y = randomBetween(Y_MAX * 0.3f, Y_MAX - 1);
    w.temperature = randomBetween(0.1f, 0.9f);
    w.vy = 0;
    w.x = w.home;
  }
  lastFrame = millis();
  lastModeTime = modeMillis();
}

void LavaLampPlugin::loop()
{
  // frames on real time, so it stays smooth also when slowed down
  const unsigned long now = millis();
  if (now - lastFrame < FRAME_MS)
  {
    return;
  }
  lastFrame = now;
  // the wax moves by the mode's clock, so the speed setting runs the whole
  // lamp faster or slower and it moves the same way; after a pause (a busy
  // moment, a message on screen) it carries on instead of jumping
  const uint32_t modeNow = modeMillis();
  const float dt = fminf((modeNow - lastModeTime) / 1000.0f, MAX_STEP_S);
  lastModeTime = modeNow;

  for (Wax &w : wax)
  {
    step(w, dt);
  }
  render();
}

void LavaLampPlugin::step(Wax &w, float dt)
{
  const float height = w.y / Y_MAX; // 0 at the top, 1 at the bottom
  const float rest = radius(w.size, w.temperature) * REST;
  const float heat = w.y > Y_MAX - rest - HEAT_DEPTH ? w.heat : 0;
  const float above = 1 - height;
  const float cool = COOL * (1 + 6 * above * above * above * above);
  w.temperature = fmaxf(0, fminf(1.2f, w.temperature + (heat - cool * w.temperature) * dt));

  // buoyancy, followed slowly: the wax is thick
  const float target = -(w.temperature - NEUTRAL) * RISE;
  w.vy += (target - w.vy) * fminf(1.0f, dt / VISCOSITY);
  w.y += w.vy * dt;

  // resting on the bottom, or pressed against the top cap
  if (w.y > Y_MAX - rest)
  {
    w.y = Y_MAX - rest;
    w.vy = fminf(w.vy, 0);
  }
  if (w.y < rest)
  {
    w.y = rest;
    w.vy = fmaxf(w.vy, 0);
  }

  w.phase = wrap(w.phase + w.wobble * dt);
  w.x = fmaxf(1.0f, fminf(COLS - 2.0f, w.home + DRIFT * sinf(w.phase)));
}

void LavaLampPlugin::render()
{
  // per wax: its ellipse, stretched by its speed
  float rx2[NUM_WAX], ry2[NUM_WAX];
  for (uint8_t i = 0; i < NUM_WAX; i++)
  {
    const float r = radius(wax[i].size, wax[i].temperature);
    const float s = 1 + STRETCH * fabsf(wax[i].vy);
    rx2[i] = r * r / s;
    ry2[i] = r * r * s * s;
  }

  for (uint8_t row = 0; row < ROWS; row++)
  {
    const float y = (row + 0.5f) * ASPECT;
    for (uint8_t col = 0; col < COLS; col++)
    {
      const float x = col + 0.5f;
      float value = 0;
      for (uint8_t i = 0; i < NUM_WAX; i++)
      {
        const float dx = wax[i].x - x;
        const float dy = wax[i].y - y;
        const float d = dx * dx / rx2[i] + dy * dy / ry2[i];
        if (d < 1)
        {
          value += (1 - d) * (1 - d);
        }
      }

      // LEDs at half power look much brighter than half: squared keeps the edge soft
      const float level = smoothstep(EDGE_LO, EDGE_HI, value);
      uint8_t brightness = static_cast<uint8_t>(level * level * 255.0f + 0.5f);
      if (brightness < FLOOR)
      {
        brightness = 0;
      }

      const uint16_t index = row * COLS + col;
      if (previousBrightness[index] != brightness)
      {
        Screen.setPixel(col, row, brightness > 0, brightness);
        previousBrightness[index] = brightness;
      }
    }
  }
}

const char *LavaLampPlugin::getName() const
{
  return "Lavalamp";
}
