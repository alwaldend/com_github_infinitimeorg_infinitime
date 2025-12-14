#include "displayapp/screens/Pomodoro.h"
#include "displayapp/screens/Symbols.h"
#include "displayapp/InfiniTimeTheme.h"
#include "components/pomodoro/PomodoroController.h"
#include "components/motor/MotorController.h"
#include "systemtask/SystemTask.h"
#include <lvgl/src/lv_widgets/lv_switch.h>

using namespace Pinetime::Applications::Screens;
using Pinetime::Controllers::PomodoroController;

namespace {
  void buttonEventHandler(lv_obj_t* obj, lv_event_t event) {
    auto* screen = static_cast<Pomodoro*>(obj->user_data);
    screen->OnButtonEvent(obj, event);
  }

  void valueChangedHandler(void* userData) {
    auto* screen = static_cast<Pomodoro*>(userData);
    screen->OnValueChanged();
  }

  void stopAlarmTaskHandler(lv_task_t* task) {
    auto* screen = static_cast<Pomodoro*>(task->user_data);
    screen->StopAlarm();
  }
}

Pomodoro::Pomodoro(Controllers::PomodoroController& pomodoroController,
                   System::SystemTask& systemTask,
                   Controllers::MotorController& motorController)
  : pomodoroController {pomodoroController}, wakeLock(systemTask), motorController {motorController} {

  focusCounter.Create();
  lv_obj_align(focusCounter.GetObject(), nullptr, LV_ALIGN_IN_TOP_LEFT, 0, 0);
  focusCounter.SetValue(pomodoroController.FocusDuration());
  focusCounter.SetValueChangedEventCallback(this, valueChangedHandler);

  breakCounter.Create();
  lv_obj_align(breakCounter.GetObject(), nullptr, LV_ALIGN_IN_TOP_RIGHT, 0, 0);
  breakCounter.SetValue(pomodoroController.BreakDuration());
  breakCounter.SetValueChangedEventCallback(this, valueChangedHandler);

  buttonStop = lv_btn_create(lv_scr_act(), nullptr);
  buttonStop->user_data = this;
  lv_obj_set_event_cb(buttonStop, buttonEventHandler);
  lv_obj_set_size(buttonStop, 115, 50);
  lv_obj_align(buttonStop, lv_scr_act(), LV_ALIGN_IN_BOTTOM_LEFT, 0, 0);
  lv_obj_set_style_local_bg_color(buttonStop, LV_BTN_PART_MAIN, LV_STATE_DEFAULT, LV_COLOR_RED);
  buttonStopLabel = lv_label_create(buttonStop, nullptr);
  lv_label_set_text_static(buttonStopLabel, Symbols::stop);
  lv_obj_set_hidden(buttonStop, true);

  static constexpr lv_color_t bgColor = Colors::bgAlt;

  buttonInfo = lv_btn_create(lv_scr_act(), nullptr);
  buttonInfo->user_data = this;
  lv_obj_set_event_cb(buttonInfo, buttonEventHandler);
  lv_obj_set_size(buttonInfo, 50, 50);
  lv_obj_align(buttonInfo, lv_scr_act(), LV_ALIGN_IN_TOP_MID, 0, -4);
  lv_obj_set_style_local_bg_color(buttonInfo, LV_BTN_PART_MAIN, LV_STATE_DEFAULT, bgColor);
  lv_obj_set_style_local_border_width(buttonInfo, LV_BTN_PART_MAIN, LV_STATE_DEFAULT, 4);
  lv_obj_set_style_local_border_color(buttonInfo, LV_BTN_PART_MAIN, LV_STATE_DEFAULT, LV_COLOR_BLACK);

  lv_obj_t* buttonInfoLabel = lv_label_create(buttonInfo, nullptr);
  lv_label_set_text_static(buttonInfoLabel, "i");

  enableSwitch = lv_switch_create(lv_scr_act(), nullptr);
  enableSwitch->user_data = this;
  lv_obj_set_event_cb(enableSwitch, buttonEventHandler);
  lv_obj_set_size(enableSwitch, 100, 50);
  lv_obj_align(enableSwitch, lv_scr_act(), LV_ALIGN_IN_BOTTOM_LEFT, 7, 0);
  lv_obj_set_style_local_bg_color(enableSwitch, LV_SWITCH_PART_BG, LV_STATE_DEFAULT, bgColor);

  buttonInterval = lv_btn_create(lv_scr_act(), nullptr);
  buttonInterval->user_data = this;
  lv_obj_set_event_cb(buttonInterval, buttonEventHandler);
  lv_obj_set_size(buttonInterval, 115, 50);
  lv_obj_align(buttonInterval, lv_scr_act(), LV_ALIGN_IN_BOTTOM_RIGHT, 0, 0);
  buttonIntervalLabel = lv_label_create(buttonInterval, nullptr);
  updateIntervalButton();
  lv_obj_set_style_local_bg_color(buttonInterval, LV_BTN_PART_MAIN, LV_STATE_DEFAULT, bgColor);

  updateDuration();

  if (pomodoroController.IsAlerting()) {
    OnPomodoroAlarmTriggered();
  } else {
    updateSwitch(LV_ANIM_OFF);
  }
}

Pomodoro::~Pomodoro() {
  if (pomodoroController.IsAlerting()) {
    StopAlarm();
  }
  lv_obj_clean(lv_scr_act());
  pomodoroController.SaveState();
}

bool Pomodoro::OnButtonPushed() {
  if (infoPopupButtonLabel != nullptr && buttonInfo != nullptr) {
    onInfoPopupButtonPress();
    return true;
  }
  if (pomodoroController.IsAlerting()) {
    StopAlarm();
    return true;
  }
  return false;
}

void Pomodoro::OnButtonEvent(lv_obj_t* obj, lv_event_t event) {
  if (event != LV_EVENT_CLICKED) {
    return;
  }
  if (obj == buttonStop) {
    StopAlarm();
  } else if (obj == buttonInfo) {
    onInfoButtonPress();
  } else if (obj == infoPopupButton) {
    onInfoPopupButtonPress();
  } else if (obj == enableSwitch) {
    onEnableSwitchPress();
  } else if (obj == buttonInterval) {
    onIntervalButtonPress();
  }
}

bool Pomodoro::OnTouchEvent(Pinetime::Applications::TouchEvents event) {
  // Don't allow closing the screen by swiping while the alarm is alerting
  return pomodoroController.IsAlerting() && event == TouchEvents::SwipeDown;
}

void Pomodoro::OnValueChanged() {
  updateDuration();
}

void Pomodoro::OnPomodoroAlarmTriggered() {
  lv_obj_set_hidden(enableSwitch, true);
  lv_obj_set_hidden(buttonStop, false);
  taskStopAlarm = lv_task_create(stopAlarmTaskHandler, pdMS_TO_TICKS(60 * 1000), LV_TASK_PRIO_MID, this);
  motorController.StartRinging();
  wakeLock.Lock();
}

void Pomodoro::StopAlarm() {
  motorController.StopRinging();
  pomodoroController.ToggleInterval();
  pomodoroController.ScheduleAlarm();
  updateIntervalButton();
  updateSwitch(LV_ANIM_OFF);
  if (taskStopAlarm != nullptr) {
    lv_task_del(taskStopAlarm);
    taskStopAlarm = nullptr;
  }
  wakeLock.Release();
  lv_obj_set_hidden(enableSwitch, false);
  lv_obj_set_hidden(buttonStop, true);
  pomodoroController.StopAlarm();
}

void Pomodoro::onIntervalButtonPress() {
  pomodoroController.ToggleInterval();
  updateIntervalButton();
}

void Pomodoro::updateIntervalButton() {
  switch (pomodoroController.Interval()) {
    case Pinetime::Controllers::PomodoroController::IntervalType::Break:
      lv_label_set_text_static(buttonIntervalLabel, "BREAK");
      break;
    case Pinetime::Controllers::PomodoroController::IntervalType::Focus:
      lv_label_set_text_static(buttonIntervalLabel, "FOCUS");
      break;
  }
}

void Pomodoro::onEnableSwitchPress() {
  pomodoroController.UpdateEnabled(lv_switch_get_state(enableSwitch));
}

void Pomodoro::updateSwitch(lv_anim_enable_t anim) {
  if (pomodoroController.IsEnabled()) {
    lv_switch_on(enableSwitch, anim);
  } else {
    lv_switch_off(enableSwitch, anim);
  }
}

void Pomodoro::updateDuration() {
  pomodoroController.UpdateDuration(focusCounter.GetValue(), breakCounter.GetValue());
}

void Pomodoro::onInfoButtonPress() {
  if (infoPopupButton != nullptr) {
    return;
  }
  infoPopupButton = lv_btn_create(lv_scr_act(), nullptr);
  infoPopupButton->user_data = this;
  lv_obj_set_event_cb(infoPopupButton, buttonEventHandler);
  lv_obj_set_height(infoPopupButton, 200);
  lv_obj_set_width(infoPopupButton, 150);
  lv_obj_align(infoPopupButton, lv_scr_act(), LV_ALIGN_CENTER, 0, 0);
  lv_obj_set_style_local_bg_color(infoPopupButton, LV_BTN_PART_MAIN, LV_STATE_DEFAULT, LV_COLOR_NAVY);

  infoPopupButtonLabel = lv_label_create(infoPopupButton, nullptr);
  lv_label_set_text_fmt(infoPopupButtonLabel, "left: Focus\nright: Break");
}

void Pomodoro::onInfoPopupButtonPress() {
  lv_obj_del(infoPopupButton);
  infoPopupButtonLabel = nullptr;
  infoPopupButton = nullptr;
}
