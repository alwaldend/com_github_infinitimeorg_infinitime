#pragma once

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
                      Controllers::DateTime& dateTimeController,
                      const Controllers::Battery& batteryController,
                      const Controllers::Ble& bleController,
                      Controllers::NotificationManager& notificationManager,
                      Controllers::Settings& settingsController,
                      Controllers::HeartRateController& heartRateController,
                      Controllers::MotionController& motionController);
        ~WatchFaceText() override;

        void Refresh() override;

      private:
        void refreshCharge();
        void refreshBle();
        void refreshDatetime();
        void refreshHeartbeat();
        void refreshSteps();
        void refreshAlarm();

        Utility::DirtyValue<int> batteryPercentRemaining;
        Utility::DirtyValue<bool> powerPresent;
        Utility::DirtyValue<bool> bleState;
        Utility::DirtyValue<bool> bleRadioEnabled;
        Utility::DirtyValue<std::chrono::time_point<std::chrono::system_clock, std::chrono::seconds>> currentDateTime;
        Utility::DirtyValue<uint32_t> stepCount;
        Utility::DirtyValue<uint8_t> heartbeat;
        Utility::DirtyValue<bool> heartbeatRunning;
        Utility::DirtyValue<bool> notificationState;
        using days = std::chrono::duration<int32_t, std::ratio<86400>>; // TODO: days is standard in c++20
        Utility::DirtyValue<std::chrono::time_point<std::chrono::system_clock, days>> currentDate;

        lv_obj_t* label_time;
        lv_obj_t* label_date;
        lv_obj_t* label_prompt_1;
        lv_obj_t* label_prompt_2;
        lv_obj_t* batteryValue;
        lv_obj_t* weekdayValue;
        lv_obj_t* monthValue;
        lv_obj_t* heartbeatValue;
        lv_obj_t* stepValue;
        lv_obj_t* notificationIcon;
        lv_obj_t* connectState;
        lv_obj_t* alarmValue;

        Controllers::AlarmController& alarmController;
        Controllers::DateTime& dateTimeController;
        const Controllers::Battery& batteryController;
        const Controllers::Ble& bleController;
        Controllers::NotificationManager& notificationManager;
        Controllers::Settings& settingsController;
        Controllers::HeartRateController& heartRateController;
        Controllers::MotionController& motionController;

        lv_task_t* taskRefresh;
      };
    }

    template <>
    struct WatchFaceTraits<WatchFace::Text> {
      static constexpr WatchFace watchFace = WatchFace::Text;
      static constexpr const char* name = "Text";

      static Screens::Screen* Create(AppControllers& controllers) {
        return new Screens::WatchFaceText(controllers.alarmController,
                                          controllers.dateTimeController,
                                          controllers.batteryController,
                                          controllers.bleController,
                                          controllers.notificationManager,
                                          controllers.settingsController,
                                          controllers.heartRateController,
                                          controllers.motionController);
      };

      static bool IsAvailable(Pinetime::Controllers::FS& /*filesystem*/) {
        return true;
      }
    };
  }
}
