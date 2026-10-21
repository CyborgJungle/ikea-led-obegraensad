#pragma once

#include "PluginManager.h"
#include <Arduino.h>
#include <vector>

struct ScheduleItem
{
  int pluginId;
  unsigned long duration; // Duration in milliseconds
};

class PluginScheduler
{
private:
  PluginScheduler() = default;
  unsigned long lastSwitch = 0;
  size_t currentIndex = 0;

  // last result of the active hours check: -1 unknown, 0 outside, 1 inside
  int8_t insideHours = -1;
  unsigned long lastHoursCheck = 0;

  bool needsPersist = false;
  unsigned long lastPersistRequest = 0;
  static constexpr unsigned long PERSIST_DELAY_MS = 2000;

public:
  static PluginScheduler &getInstance();

  PluginScheduler(const PluginScheduler &) = delete;
  PluginScheduler &operator=(const PluginScheduler &) = delete;

  bool isActive = false;
  std::vector<ScheduleItem> schedule;

  void addItem(int pluginId, unsigned long durationSeconds);
  void clearSchedule(bool emptyStorage = false);
  void start();
  void stop();
  void update();
  void init();
  bool setScheduleByJSONString(String scheduleJson);

private:
  void switchToCurrentPlugin();
  void updateActiveHours();
  void requestPersist();
  void checkAndPersist();
};

extern PluginScheduler &Scheduler;