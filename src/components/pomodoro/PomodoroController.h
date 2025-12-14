#pragma once

#include <FreeRTOS.h>
#include <timers.h>
#include <cstdint>
#include "components/datetime/DateTimeController.h"

namespace Pinetime {
  namespace System {
    class SystemTask;
  }

  namespace Controllers {
    class PomodoroController {
    public:
      PomodoroController(Controllers::DateTime& dateTimeController, Controllers::FS& fs);

      enum IntervalType { Focus, Break };

      uint8_t FocusDuration();
      uint8_t BreakDuration();
      IntervalType Interval();
      bool IsEnabled();
      bool IsAlerting();
      uint32_t SecondsLeft();
      void UpdateDuration(uint8_t focusDuration, uint8_t breakDuration);
      void UpdateEnabled(bool isEnabled);
      void UpdateInterval(IntervalType intervalType);
      void ToggleInterval();
      void SaveState();
      void Init(System::SystemTask* systemTask);
      void StartAlarm();
      void StopAlarm();
      void ScheduleAlarm();

    private:
      struct PomodoroState {
        uint8_t focusDuration = 20;
        uint8_t breakDuration = 2;
        bool isEnabled = false;
        enum IntervalType intervalType = IntervalType::Focus;
      };

      Controllers::DateTime& dateTimeController;
      Controllers::FS& fs;
      System::SystemTask* systemTask = nullptr;
      bool isAlerting = false;
      bool stateChanged = false;
      PomodoroState state;
      TimerHandle_t alarmTimer;
      std::chrono::time_point<std::chrono::system_clock, std::chrono::nanoseconds> alarmTime;

      void saveState();
      void loadState();
    };
  }
}
