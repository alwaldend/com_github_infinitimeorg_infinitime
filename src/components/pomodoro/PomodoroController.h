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
      Controllers::DateTime& dateTimeController;
      Controllers::FS& fs;

      uint8_t FocusDuration();
      uint8_t BreakDuration();
      bool IsEnabled();
      bool IsAlerting();
      void UpdateDuration(uint8_t focusDuration, uint8_t breakDuration);
      void UpdateEnabled(bool isEnabled);
      void SaveState();
      void Init(System::SystemTask* systemTask);

    private:
      struct PomodoroState {
        uint8_t focusDuration = 20;
        uint8_t breakDuration = 2;
        bool isEnabled = false;
      };

      System::SystemTask* systemTask = nullptr;
      bool isAlerting = false;
      bool stateChanged = false;
      PomodoroState state;

      void saveState();
      void loadState();
    };
  }
}
