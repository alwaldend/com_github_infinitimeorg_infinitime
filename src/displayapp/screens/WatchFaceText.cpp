#include <components/pomodoro/PomodoroController.h>
#include <lvgl/lvgl.h>
#include <lvgl/src/lv_core/lv_disp.h>
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
                             Controllers::PomodoroController& pomodoroController,
                             Controllers::DateTime& dateTimeController,
                             const Controllers::Battery& batteryController,
                             const Controllers::Ble& bleController,
                             Controllers::MotionController& motionController)
  : batteryController {batteryController},
    bleController {bleController},
    currentDateTime {{}},
    dateTimeController {dateTimeController},
    pomodoroController {pomodoroController},
    motionController {motionController},
    alarmController {alarmController} {
  timeLabel = lv_label_create(lv_scr_act(), nullptr);
  dateLabel = lv_label_create(lv_scr_act(), nullptr);
  weekdayLabel = lv_label_create(lv_scr_act(), nullptr);
  monthLabel = lv_label_create(lv_scr_act(), nullptr);
  batteryLabel = lv_label_create(lv_scr_act(), nullptr);
  stepLabel = lv_label_create(lv_scr_act(), nullptr);
  connectLabel = lv_label_create(lv_scr_act(), nullptr);
  alarmLabel = lv_label_create(lv_scr_act(), nullptr);
  pomodoroLabel = lv_label_create(lv_scr_act(), nullptr);

  lv_obj_align(timeLabel, lv_scr_act(), LV_ALIGN_IN_LEFT_MID, 0, -100);
  lv_obj_align(dateLabel, lv_scr_act(), LV_ALIGN_IN_LEFT_MID, 0, -75);
  lv_obj_align(weekdayLabel, lv_scr_act(), LV_ALIGN_IN_LEFT_MID, 0, -50);
  lv_obj_align(monthLabel, lv_scr_act(), LV_ALIGN_IN_LEFT_MID, 0, -25);
  lv_obj_align(batteryLabel, lv_scr_act(), LV_ALIGN_IN_LEFT_MID, 0, 0);
  lv_obj_align(alarmLabel, lv_scr_act(), LV_ALIGN_IN_LEFT_MID, 0, 25);
  lv_obj_align(pomodoroLabel, lv_scr_act(), LV_ALIGN_IN_LEFT_MID, 0, 50);
  lv_obj_align(stepLabel, lv_scr_act(), LV_ALIGN_IN_LEFT_MID, 0, 75);
  lv_obj_align(connectLabel, lv_scr_act(), LV_ALIGN_IN_LEFT_MID, 0, 100);

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
  refreshPomodoro();
  refreshSteps();
  refreshAlarm();
}

void WatchFaceText::refreshAlarm() {
  alarmEnabled = alarmController.IsEnabled();
  if (alarmEnabled.Get()) {
    alarmSeconds = alarmController.SecondsToAlarm();
    if (!alarmSeconds.IsUpdated()) {
      return;
    }
    auto minutesLeft = alarmSeconds.Get() / 60;
    lv_label_set_text_fmt(alarmLabel,
                          "alarm %02d:%02d (%02d:%02d)",
                          alarmController.Hours(),
                          alarmController.Minutes(),
                          minutesLeft / 60,
                          minutesLeft % 60);
  } else {
    lv_label_set_text_static(alarmLabel, "alarm ---");
  }
}

void WatchFaceText::refreshCharge() {
  powerPresent = batteryController.IsPowerPresent();
  batteryPercentRemaining = batteryController.PercentRemaining();
  if (batteryPercentRemaining.IsUpdated() || powerPresent.IsUpdated()) {
    lv_label_set_text_fmt(batteryLabel, "charge %d%%", batteryPercentRemaining.Get());
    if (batteryController.IsPowerPresent()) {
      lv_label_ins_text(batteryLabel, LV_LABEL_POS_LAST, " (*)");
    }
  }
}

void WatchFaceText::refreshBle() {
  bleState = bleController.IsConnected();
  bleRadioEnabled = bleController.IsRadioEnabled();
  if (bleState.IsUpdated() || bleRadioEnabled.IsUpdated()) {
    if (!bleRadioEnabled.Get()) {
      lv_label_set_text_static(connectLabel, "connected ---");
    } else {
      if (bleState.Get()) {
        lv_label_set_text_static(connectLabel, "connected True");
      } else {
        lv_label_set_text_static(connectLabel, "connected False");
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

  lv_label_set_text_fmt(timeLabel, "time %02d:%02d:%02d", hour, minute, second);

  currentDate = std::chrono::time_point_cast<std::chrono::days>(currentDateTime.Get());
  if (currentDate.IsUpdated()) {
    uint16_t year = dateTimeController.Year();
    Controllers::DateTime::Months month = dateTimeController.Month();
    uint8_t day = dateTimeController.Day();
    lv_label_set_text_fmt(dateLabel, "date %04d-%02d-%02d", short(year), char(month), char(day));
  }

  dayOfTheWeek = dateTimeController.DayOfWeek();
  if (dayOfTheWeek.IsUpdated()) {
    lv_label_set_text_fmt(weekdayLabel,
                          "weekday %s (%d)",
                          Pinetime::Controllers::DateTime::DayOfWeekShortToStringLow(dayOfTheWeek.Get()),
                          dayOfTheWeek.Get());
  }

  month = dateTimeController.Month();
  if (month.IsUpdated()) {
    lv_label_set_text_fmt(monthLabel, "month %s (%d)", Pinetime::Controllers::DateTime::MonthShortToStringLow(month.Get()), month.Get());
  }
}

void WatchFaceText::refreshPomodoro() {
  pomodoroEnabled = pomodoroController.IsEnabled();
  auto enabledUpdated = pomodoroEnabled.IsUpdated();
  if (!pomodoroEnabled.Get()) {
    if (enabledUpdated) {
      lv_label_set_text_static(pomodoroLabel, "pmdoro ---");
    }
    return;
  }
  pomodoroInterval = pomodoroController.Interval();
  pomodoroSecondsLeft = pomodoroController.SecondsLeft();
  if (!pomodoroInterval.IsUpdated() && !pomodoroSecondsLeft.IsUpdated()) {
    return;
  }
  auto secondsFull = pomodoroSecondsLeft.Get();
  auto minutes = secondsFull / 60;
  auto seconds = secondsFull % 60;
  switch (pomodoroInterval.Get()) {
    case Pinetime::Controllers::PomodoroController::IntervalType::Break:
      lv_label_set_text_fmt(pomodoroLabel, "pmdoro Break (%02d:%02d)", minutes, seconds);
      break;
    case Pinetime::Controllers::PomodoroController::IntervalType::Focus:
      lv_label_set_text_fmt(pomodoroLabel, "pmdoro Focus (%02d:%02d)", minutes, seconds);
      break;
  }
}

void WatchFaceText::refreshSteps() {
  stepCount = motionController.NbSteps();
  if (stepCount.IsUpdated()) {
    lv_label_set_text_fmt(stepLabel, "steps %lu", stepCount.Get());
  }
}
