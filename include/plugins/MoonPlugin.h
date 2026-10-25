#pragma once

#include "PluginManager.h"
#include <atomic>

// Moon: the moon as it is now, from new through crescent, half and gibbous to
// full, with its dark seas. Seen from the weather location in Settings: the
// lit edge points at the sun and the face is tilted as the moon stands in
// that sky, turning through the night. The phase is computed on the lamp from
// the time; the city is looked up once (Open-Meteo) to know where it is.
class MoonPlugin : public Plugin
{
public:
  void setup() override;
  void loop() override;
  const char *getName() const override;

  static void geocodeTask(void *parameter);

private:
  // the place: written by the lookup task, read by loop()
  std::atomic<bool> placeReady{false};
  std::atomic<bool> lookupRunning{false};
  std::atomic<bool> lookupFailed{false};
  // written by the task, copied into latitude/longitude by loop() on placeReady
  double foundLatitude = 0, foundLongitude = 0;
  double latitude = 0, longitude = 0;
  String placeFor;   // the city the coordinates are for
  String lookupCity; // the city the task is looking up
  unsigned long lastLookup = 0;
  unsigned long lastCityCheck = 0;
  bool hasPlace = false;

  unsigned long lastDraw = 0;
  bool drawn = false;
  uint8_t previousBrightness[ROWS * COLS];

  void startLookup(const String &city);
  bool lookup();
  void draw();
};
