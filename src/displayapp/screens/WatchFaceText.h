#pragma once

#include <components/pomodoro/PomodoroController.h>
#include <lvgl/src/lv_core/lv_obj.h>
#include <chrono>
#include <cstdint>
#include <memory>
#include <displayapp/Controllers.h>
#include "displayapp/screens/Screen.h"
#include "components/datetime/DateTimeController.h"
#include "utility/DirtyValue.h"

namespace Pinetime {
  namespace Controllers {
    class Settings;
    class Battery;
    class Ble;
    class NotificationManager;
    class HeartRateController;
    class MotionController;
  }

  namespace Applications {
    namespace Screens {

      class WatchFaceText : public Screen {
      public:
        WatchFaceText(Controllers::AlarmController& alarmController,
                      Controllers::PomodoroController& pomodoroController,
                      Controllers::DateTime& dateTimeController,
                      const Controllers::Battery& batteryController,
                      const Controllers::Ble& bleController,
                      Controllers::MotionController& motionController);
        ~WatchFaceText() override;

        void Refresh() override;

      private:
        void refreshCharge();
        lv_obj_t* batteryLabel;
        Utility::DirtyValue<int> batteryPercentRemaining;
        Utility::DirtyValue<bool> powerPresent;
        const Controllers::Battery& batteryController;

        void refreshBle();
        lv_obj_t* connectLabel;
        Utility::DirtyValue<bool> bleState;
        Utility::DirtyValue<bool> bleRadioEnabled;
        const Controllers::Ble& bleController;

        void refreshDatetime();
        lv_obj_t* timeLabel;
        lv_obj_t* dateLabel;
        lv_obj_t* weekdayLabel;
        lv_obj_t* monthLabel;
        using days = std::chrono::duration<int32_t, std::ratio<86400>>; // TODO: days is standard in c++20
        Utility::DirtyValue<std::chrono::time_point<std::chrono::system_clock, std::chrono::seconds>> currentDateTime;
        Utility::DirtyValue<std::chrono::time_point<std::chrono::system_clock, days>> currentDate;
        Utility::DirtyValue<Pinetime::Controllers::DateTime::Days> dayOfTheWeek;
        Utility::DirtyValue<Pinetime::Controllers::DateTime::Months> month;
        Controllers::DateTime& dateTimeController;

        void refreshPomodoro();
        lv_obj_t* pomodoroLabel;
        Utility::DirtyValue<bool> pomodoroEnabled;
        Utility::DirtyValue<Controllers::PomodoroController::IntervalType> pomodoroInterval;
        Utility::DirtyValue<uint32_t> pomodoroSecondsLeft;
        Controllers::PomodoroController& pomodoroController;

        void refreshSteps();
        lv_obj_t* stepLabel;
        Utility::DirtyValue<uint32_t> stepCount;
        Controllers::MotionController& motionController;

        void refreshAlarm();
        lv_obj_t* alarmLabel;
        Utility::DirtyValue<bool> alarmEnabled;
        Utility::DirtyValue<uint32_t> alarmSeconds;
        Controllers::AlarmController& alarmController;

        lv_task_t* taskRefresh;
      };
    }

    template <>
    struct WatchFaceTraits<WatchFace::Text> {
      static constexpr WatchFace watchFace = WatchFace::Text;
      static constexpr const char* name = "Text";

      static Screens::Screen* Create(AppControllers& controllers) {
        return new Screens::WatchFaceText(controllers.alarmController,
                                          controllers.pomodoroController,
                                          controllers.dateTimeController,
                                          controllers.batteryController,
                                          controllers.bleController,
                                          controllers.motionController);
      };

      static bool IsAvailable(Pinetime::Controllers::FS& /*filesystem*/) {
        return true;
      }
    };
  }
}
