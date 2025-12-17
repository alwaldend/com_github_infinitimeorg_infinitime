#pragma once

#include <FreeRTOS.h>
#include <timers.h>
#include <cstdint>
#include "components/datetime/DateTimeController.h"
#include "displayapp/Controllers.h"

namespace Pinetime {
  namespace System {
    class SystemTask;
  }

  namespace Controllers {
    class PomodoroController {
    public:
      PomodoroController(Controllers::DateTime& dateTimeController,
                         Controllers::FS& filesystem,
                         Controllers::MotorController& motorController);

      enum IntervalType { Focus, Break };

      [[nodiscard]] IntervalType Interval() const {
        return state.intervalType;
      }

      [[nodiscard]] uint8_t FocusDuration() const {
        return state.focusDuration;
      }

      [[nodiscard]] uint8_t BreakDuration() const {
        return state.breakDuration;
      }

      [[nodiscard]] bool IsEnabled() const {
        return state.isEnabled;
      }

      [[nodiscard]] bool IsAlerting() const {
        return isAlerting;
      }

      [[nodiscard]] bool IsShowingInfo() const {
        return showInfo;
      }

      void UpdateShowInfo(bool val) {
        showInfo = val;
      }

      uint32_t SecondsLeft();
      void UpdateBreak(uint8_t breakDuration);
      void UpdateFocus(uint8_t focusDuration);
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
      Controllers::MotorController& motorController;

      System::SystemTask* systemTask = nullptr;
      bool isAlerting = false;
      bool showInfo = false;
      bool stateChanged = false;
      PomodoroState state;
      TimerHandle_t alarmTimer;
      TimerHandle_t alarmStopTimer;
      std::chrono::time_point<std::chrono::system_clock, std::chrono::nanoseconds> alarmTime;

      void saveState();
      void loadState();
    };
  }
}
