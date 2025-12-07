#include <lvgl/lvgl.h>
#include "displayapp/screens/WatchFaceTerminalCustom.h"
#include "displayapp/screens/BatteryIcon.h"
#include "displayapp/screens/NotificationIcon.h"
#include "displayapp/screens/Symbols.h"
#include "components/battery/BatteryController.h"
#include "components/ble/BleController.h"
#include "components/ble/NotificationManager.h"
#include "components/heartrate/HeartRateController.h"
#include "components/motion/MotionController.h"
#include "components/settings/Settings.h"

using namespace Pinetime::Applications::Screens;

WatchFaceTerminalCustom::WatchFaceTerminalCustom(Controllers::DateTime& dateTimeController,
                                     const Controllers::Battery& batteryController,
                                     const Controllers::Ble& bleController,
                                     Controllers::NotificationManager& notificationManager,
                                     Controllers::Settings& settingsController,
                                     Controllers::HeartRateController& heartRateController,
                                     Controllers::MotionController& motionController)
  : currentDateTime {{}},
    dateTimeController {dateTimeController},
    batteryController {batteryController},
    bleController {bleController},
    notificationManager {notificationManager},
    settingsController {settingsController},
    heartRateController {heartRateController},
    motionController {motionController} {
  notificationIcon = lv_label_create(lv_scr_act(), nullptr);
  lv_obj_align(notificationIcon, nullptr, LV_ALIGN_IN_LEFT_MID, 0, -100);

  label_time = lv_label_create(lv_scr_act(), nullptr);
  lv_obj_align(label_time, lv_scr_act(), LV_ALIGN_IN_LEFT_MID, 0, -80);

  label_date = lv_label_create(lv_scr_act(), nullptr);
  lv_obj_align(label_date, lv_scr_act(), LV_ALIGN_IN_LEFT_MID, 0, -60);

  weekdayValue = lv_label_create(lv_scr_act(), nullptr);
  lv_obj_align(weekdayValue, lv_scr_act(), LV_ALIGN_IN_LEFT_MID, 0, -40);

  monthValue = lv_label_create(lv_scr_act(), nullptr);
  lv_obj_align(monthValue, lv_scr_act(), LV_ALIGN_IN_LEFT_MID, 0, -20);

  batteryValue = lv_label_create(lv_scr_act(), nullptr);
  lv_obj_align(batteryValue, lv_scr_act(), LV_ALIGN_IN_LEFT_MID, 0, 0);

  stepValue = lv_label_create(lv_scr_act(), nullptr);
  lv_obj_align(stepValue, lv_scr_act(), LV_ALIGN_IN_LEFT_MID, 0, 20);

  heartbeatValue = lv_label_create(lv_scr_act(), nullptr);
  lv_obj_align(heartbeatValue, lv_scr_act(), LV_ALIGN_IN_LEFT_MID, 0, 40);

  connectState = lv_label_create(lv_scr_act(), nullptr);
  lv_obj_align(connectState, lv_scr_act(), LV_ALIGN_IN_LEFT_MID, 0, 60);

  taskRefresh = lv_task_create(RefreshTaskCallback, LV_DISP_DEF_REFR_PERIOD, LV_TASK_PRIO_MID, this);
  Refresh();
}

WatchFaceTerminalCustom::~WatchFaceTerminalCustom() {
  lv_task_del(taskRefresh);
  lv_obj_clean(lv_scr_act());
}

void WatchFaceTerminalCustom::Refresh() {
  powerPresent = batteryController.IsPowerPresent();
  batteryPercentRemaining = batteryController.PercentRemaining();
  if (batteryPercentRemaining.IsUpdated() || powerPresent.IsUpdated()) {
    lv_label_set_text_fmt(batteryValue, "charge: %d%%", batteryPercentRemaining.Get());
    if (batteryController.IsPowerPresent()) {
      lv_label_ins_text(batteryValue, LV_LABEL_POS_LAST, " charging");
    }
  }

  bleState = bleController.IsConnected();
  bleRadioEnabled = bleController.IsRadioEnabled();
  if (bleState.IsUpdated() || bleRadioEnabled.IsUpdated()) {
    if (!bleRadioEnabled.Get()) {
      lv_label_set_text_static(connectState, "status: disabled");
    } else {
      if (bleState.Get()) {
        lv_label_set_text_static(connectState, "status: connected");
      } else {
        lv_label_set_text_static(connectState, "status: disconnected");
      }
    }
  }

  notificationState = notificationManager.AreNewNotificationsAvailable();
  if (notificationState.IsUpdated()) {
    if (notificationState.Get()) {
      lv_label_set_text_static(notificationIcon, "you have mail.");
    } else {
      lv_label_set_text_static(notificationIcon, "");
    }
  }

  currentDateTime = std::chrono::time_point_cast<std::chrono::seconds>(dateTimeController.CurrentDateTime());
  if (currentDateTime.IsUpdated()) {
    uint8_t hour = dateTimeController.Hours();
    uint8_t minute = dateTimeController.Minutes();
    uint8_t second = dateTimeController.Seconds();

    if (settingsController.GetClockType() == Controllers::Settings::ClockType::H12) {
      char ampmChar[3] = "am";
      if (hour == 0) {
        hour = 12;
      } else if (hour == 12) {
        ampmChar[0] = 'p';
      } else if (hour > 12) {
        hour = hour - 12;
        ampmChar[0] = 'p';
      }
      lv_label_set_text_fmt(label_time, "time: %s", hour, minute, second, ampmChar);
    } else {
      lv_label_set_text_fmt(label_time, "time: %02d:%02d:%02d", hour, minute, second);
    }

    currentDate = std::chrono::time_point_cast<days>(currentDateTime.Get());
    if (currentDate.IsUpdated()) {
      uint16_t year = dateTimeController.Year();
      Controllers::DateTime::Months month = dateTimeController.Month();
      uint8_t day = dateTimeController.Day();
      lv_label_set_text_fmt(label_date, "date: %04d-%02d-%02d", short(year), char(month), char(day));
    }
  }

  lv_label_set_text_fmt(weekdayValue, "weekdday: %s %d", dateTimeController.DayOfWeekShortToStringLow(dateTimeController.DayOfWeek()), dateTimeController.DayOfWeek());
  
  lv_label_set_text_fmt(monthValue, "month: %s %d", dateTimeController.MonthShortToStringLow(dateTimeController.Month()), dateTimeController.Month());

  heartbeat = heartRateController.HeartRate();
  heartbeatRunning = heartRateController.State() != Controllers::HeartRateController::States::Stopped;
  if (heartbeat.IsUpdated() || heartbeatRunning.IsUpdated()) {
    if (heartbeatRunning.Get()) {
      lv_label_set_text_fmt(heartbeatValue, "heartbeat: %d bpm", heartbeat.Get());
    } else {
      lv_label_set_text_static(heartbeatValue, "heartbeat: ---");
    }
  }

  stepCount = motionController.NbSteps();
  if (stepCount.IsUpdated()) {
    lv_label_set_text_fmt(stepValue, "steps: %lu", stepCount.Get());
  }
}
