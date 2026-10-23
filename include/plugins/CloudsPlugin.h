#pragma once

#include "PluginManager.h"
#include <atomic>

// Clouds: an abstract, slowly drifting cloud texture steered by the real sky.
// Every 10 minutes a background task fetches the newest Meteosat infrared
// image of the area around Amsterdam (EUMETSAT, 16x16): it decides where the
// texture is dense or thin, and the shift between two images sets the drift.
// It never repeats; without data (yet) it shows an even, gentle sky.
class CloudsPlugin : public Plugin
{
public:
  void setup() override;
  void loop() override;
  void teardown() override;
  const char *getName() const override;

  // the fetch: in its own task, so a slow network never holds up drawing
  static void fetchTask(void *parameter);

private:
  // how much cloud: blended from the previous image's map to the newest one
  float previous[ROWS * COLS];
  float next[ROWS * COLS];
  float blend = 1;
  float lastCoverage[ROWS * COLS];
  bool hasLastCoverage = false;
  bool initialized = false;

  // texture drift, LEDs per second, easing towards the measured wind
  float vx = 0, vy = 0, targetVx = 0, targetVy = 0;
  // texture position and time, double so they stay exact for months
  double offsetX = 0, offsetY = 0, time = 0;

  unsigned long lastFrame = 0;
  uint32_t lastModeTime = 0;
  unsigned long lastFetch = 0;
  bool fetchedOnce = false;
  uint8_t previousBrightness[ROWS * COLS];

  // the fetch task fills this and raises frameReady; loop() takes it and lowers it
  uint8_t fetched[ROWS * COLS];
  std::atomic<bool> frameReady{false};
  std::atomic<bool> fetchRunning{false};
  std::atomic<bool> fetchFailed{false};

  void startFetch();
  bool fetch();
  void takeFrame(const uint8_t *grey);
  void currentField(float *field) const;
  void render();
};
