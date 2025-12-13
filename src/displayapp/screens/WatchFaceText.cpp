#include <lvgl/lvgl.h>
#include <lvgl/src/lv_widgets/lv_label.h>
#include "displayapp/screens/WatchFaceText.h"
#include "components/battery/BatteryController.h"
#include "components/alarm/AlarmController.h"
#include "components/ble/BleController.h"
#include "components/ble/NotificationManager.h"
#include "components/heartrate/HeartRateController.h"
#include "components/motion/MotionController.h"
#include "components/settings/Settings.h"

using namespace Pinetime::Applications::Screens;

WatchFaceText::WatchFaceText(Controllers::AlarmController& alarmController,
                             Controllers::DateTime& dateTimeController,
                             const Controllers::Battery& batteryController,
                             const Controllers::Ble& bleController,
                             Controllers::NotificationManager& notificationManager,
                             Controllers::Settings& settingsController,
                             Controllers::HeartRateController& heartRateController,
                             Controllers::MotionController& motionController)
  : currentDateTime {{}},
    alarmController {alarmController},
    dateTimeController {dateTimeController},
    batteryController {batteryController},
    bleController {bleController},
    notificationManager {notificationManager},
    settingsController {settingsController},
    heartRateController {heartRateController},
    motionController {motionController} {
  label_time = lv_label_create(lv_scr_act(), nullptr);
  label_date = lv_label_create(lv_scr_act(), nullptr);
  weekdayValue = lv_label_create(lv_scr_act(), nullptr);
  monthValue = lv_label_create(lv_scr_act(), nullptr);
  batteryValue = lv_label_create(lv_scr_act(), nullptr);
  stepValue = lv_label_create(lv_scr_act(), nullptr);
  heartbeatValue = lv_label_create(lv_scr_act(), nullptr);
  connectState = lv_label_create(lv_scr_act(), nullptr);
  alarmValue = lv_label_create(lv_scr_act(), nullptr);

  lv_obj_align(label_time, lv_scr_act(), LV_ALIGN_IN_LEFT_MID, 0, -100);
  lv_obj_align(label_date, lv_scr_act(), LV_ALIGN_IN_LEFT_MID, 0, -75);
  lv_obj_align(weekdayValue, lv_scr_act(), LV_ALIGN_IN_LEFT_MID, 0, -50);
  lv_obj_align(monthValue, lv_scr_act(), LV_ALIGN_IN_LEFT_MID, 0, -25);
  lv_obj_align(batteryValue, lv_scr_act(), LV_ALIGN_IN_LEFT_MID, 0, 0);
  lv_obj_align(alarmValue, lv_scr_act(), LV_ALIGN_IN_LEFT_MID, 0, 25);
  lv_obj_align(connectState, lv_scr_act(), LV_ALIGN_IN_LEFT_MID, 0, 50);
  lv_obj_align(stepValue, lv_scr_act(), LV_ALIGN_IN_LEFT_MID, 0, 75);
  lv_obj_align(heartbeatValue, lv_scr_act(), LV_ALIGN_IN_LEFT_MID, 0, 100);

  taskRefresh = lv_task_create(RefreshTaskCallback, LV_DISP_DEF_REFR_PERIOD, LV_TASK_PRIO_MID, this);
  Refresh();
}

WatchFaceText::~WatchFaceText() {
  lv_task_del(taskRefresh);
  lv_obj_clean(lv_scr_act());
}

void WatchFaceText::Refresh() {
  refreshDatetime();
  refreshCharge();
  refreshBle();
  refreshHeartbeat();
  refreshSteps();
  refreshAlarm();
}

void WatchFaceText::refreshAlarm() {
  if (alarmController.IsEnabled()) {
    auto minutesLeft = alarmController.SecondsToAlarm() / 60;
    lv_label_set_text_fmt(alarmValue,
                          "alarm %02d:%02d (%02d:%02d)",
                          alarmController.Hours(),
                          alarmController.Minutes(),
                          minutesLeft / 60,
                          minutesLeft % 60);
  } else {
    lv_label_set_text_static(alarmValue, "alarm ---");
  }
}

void WatchFaceText::refreshCharge() {
  powerPresent = batteryController.IsPowerPresent();
  batteryPercentRemaining = batteryController.PercentRemaining();
  if (batteryPercentRemaining.IsUpdated() || powerPresent.IsUpdated()) {
    lv_label_set_text_fmt(batteryValue, "charge %d%%", batteryPercentRemaining.Get());
    if (batteryController.IsPowerPresent()) {
      lv_label_ins_text(batteryValue, LV_LABEL_POS_LAST, " (*)");
    }
  }
}

void WatchFaceText::refreshBle() {
  bleState = bleController.IsConnected();
  bleRadioEnabled = bleController.IsRadioEnabled();
  if (bleState.IsUpdated() || bleRadioEnabled.IsUpdated()) {
    if (!bleRadioEnabled.Get()) {
      lv_label_set_text_static(connectState, "connected ---");
    } else {
      if (bleState.Get()) {
        lv_label_set_text_static(connectState, "connected true");
      } else {
        lv_label_set_text_static(connectState, "connected false");
      }
    }
  }
}

void WatchFaceText::refreshDatetime() {
  currentDateTime = std::chrono::time_point_cast<std::chrono::seconds>(dateTimeController.CurrentDateTime());
  if (!currentDateTime.IsUpdated()) {
    return;
  }

  uint8_t hour = dateTimeController.Hours();
  uint8_t minute = dateTimeController.Minutes();
  uint8_t second = dateTimeController.Seconds();

  lv_label_set_text_fmt(label_time, "time %02d:%02d:%02d", hour, minute, second);

  currentDate = std::chrono::time_point_cast<std::chrono::days>(currentDateTime.Get());
  if (currentDate.IsUpdated()) {
    uint16_t year = dateTimeController.Year();
    Controllers::DateTime::Months month = dateTimeController.Month();
    uint8_t day = dateTimeController.Day();
    lv_label_set_text_fmt(label_date, "date %04d-%02d-%02d", short(year), char(month), char(day));
  }

  lv_label_set_text_fmt(weekdayValue,
                        "weekday %s (%d)",
                        Pinetime::Controllers::DateTime::DayOfWeekShortToStringLow(dateTimeController.DayOfWeek()),
                        dateTimeController.DayOfWeek());

  lv_label_set_text_fmt(monthValue,
                        "month %s (%d)",
                        Pinetime::Controllers::DateTime::MonthShortToStringLow(dateTimeController.Month()),
                        dateTimeController.Month());
}

void WatchFaceText::refreshHeartbeat() {
  heartbeat = heartRateController.HeartRate();
  heartbeatRunning = heartRateController.State() != Controllers::HeartRateController::States::Stopped;
  if (heartbeat.IsUpdated() || heartbeatRunning.IsUpdated()) {
    if (heartbeatRunning.Get()) {
      lv_label_set_text_fmt(heartbeatValue, "heartbeat %d bpm", heartbeat.Get());
    } else {
      lv_label_set_text_static(heartbeatValue, "heartbeat ---");
    }
  }
}

void WatchFaceText::refreshSteps() {
  stepCount = motionController.NbSteps();
  if (stepCount.IsUpdated()) {
    lv_label_set_text_fmt(stepValue, "steps %lu", stepCount.Get());
  }
}
