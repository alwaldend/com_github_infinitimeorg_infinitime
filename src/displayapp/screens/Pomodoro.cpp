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

  updateDuration();

  if (pomodoroController.IsAlerting()) {
    startAlerting();
  }
}

Pomodoro::~Pomodoro() {
  if (pomodoroController.IsAlerting()) {
    stopAlerting();
  }
  lv_obj_clean(lv_scr_act());
  pomodoroController.SaveState();
}

bool Pomodoro::OnButtonPushed() {
  if (buttonInfoPopupLabel != nullptr && buttonInfo != nullptr) {
    hideInfo();
    return true;
  }
  if (pomodoroController.IsAlerting()) {
    stopAlerting();
    return true;
  }
  return false;
}

void Pomodoro::OnButtonEvent(lv_obj_t* obj, lv_event_t event) {
  if (event != LV_EVENT_CLICKED) {
    return;
  }
  if (obj == buttonStop) {
    stopAlerting();
  } else if (obj == buttonInfo) {
    showInfo();
  } else if (obj == buttonInfoPopup) {
    hideInfo();
  } else if (obj == enableSwitch) {
    toggleSwitch();
  }
}

bool Pomodoro::OnTouchEvent(Pinetime::Applications::TouchEvents event) {
  // Don't allow closing the screen by swiping while the alarm is alerting
  return pomodoroController.IsAlerting() && event == TouchEvents::SwipeDown;
}

void Pomodoro::OnValueChanged() {
  updateDuration();
}

void Pomodoro::startAlerting() {
}

void Pomodoro::stopAlerting() {
}

void Pomodoro::toggleSwitch() {
  pomodoroController.UpdateEnabled(lv_switch_get_state(enableSwitch));
}

void Pomodoro::updateDuration() {
  pomodoroController.UpdateDuration(focusCounter.GetValue(), breakCounter.GetValue());
}

void Pomodoro::showInfo() {
  if (buttonInfoPopup != nullptr) {
    return;
  }
  buttonInfoPopup = lv_btn_create(lv_scr_act(), nullptr);
  buttonInfoPopup->user_data = this;
  lv_obj_set_event_cb(buttonInfoPopup, buttonEventHandler);
  lv_obj_set_height(buttonInfoPopup, 200);
  lv_obj_set_width(buttonInfoPopup, 150);
  lv_obj_align(buttonInfoPopup, lv_scr_act(), LV_ALIGN_CENTER, 0, 0);
  lv_obj_set_style_local_bg_color(buttonInfoPopup, LV_BTN_PART_MAIN, LV_STATE_DEFAULT, LV_COLOR_NAVY);

  buttonInfoPopupLabel = lv_label_create(buttonInfoPopup, nullptr);
  lv_label_set_text_fmt(buttonInfoPopupLabel, "Left: focus duration\nRight: break duration");
}

void Pomodoro::hideInfo() {
  lv_obj_del(buttonInfoPopup);
  buttonInfoPopupLabel = nullptr;
  buttonInfoPopup = nullptr;
}
