#include "displayapp/screens/Pomodoro.h"
#include "displayapp/screens/Symbols.h"
#include "displayapp/InfiniTimeTheme.h"
#include "components/pomodoro/PomodoroController.h"
#include <lvgl/src/lv_core/lv_obj.h>
#include <lvgl/src/lv_misc/lv_anim.h>
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

Pomodoro::Pomodoro(Controllers::PomodoroController& pomodoroController) : pomodoroController {pomodoroController} {
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
  lv_obj_set_style_local_bg_color(buttonInterval, LV_BTN_PART_MAIN, LV_STATE_DEFAULT, bgColor);

  infoPopupButton = lv_btn_create(lv_scr_act(), nullptr);
  infoPopupButton->user_data = this;
  lv_obj_set_event_cb(infoPopupButton, buttonEventHandler);
  lv_obj_set_height(infoPopupButton, 200);
  lv_obj_set_width(infoPopupButton, 150);
  lv_obj_align(infoPopupButton, lv_scr_act(), LV_ALIGN_CENTER, 0, 0);
  lv_obj_set_style_local_bg_color(infoPopupButton, LV_BTN_PART_MAIN, LV_STATE_DEFAULT, LV_COLOR_NAVY);
  lv_obj_set_hidden(infoPopupButton, true);

  infoPopupButtonLabel = lv_label_create(infoPopupButton, nullptr);
  lv_label_set_text_fmt(infoPopupButtonLabel, "left: Focus\nright: Break");

  pomodoroController.UpdateRinging();
  UpdateUI(LV_ANIM_OFF);
}

Pomodoro::~Pomodoro() {
  if (pomodoroController.IsAlerting()) {
    pomodoroController.StopAlarm();
  }
  lv_obj_clean(lv_scr_act());
  pomodoroController.SaveState();
}

void Pomodoro::UpdateUI(lv_anim_enable_t animation) {
  alerting = pomodoroController.IsAlerting();
  if (alerting.IsUpdated()) {
    if (alerting.Get()) {
      lv_obj_set_hidden(enableSwitch, true);
      lv_obj_set_hidden(buttonStop, false);
      focusCounter.HideControls();
      breakCounter.HideControls();
    } else {
      lv_obj_set_hidden(enableSwitch, false);
      lv_obj_set_hidden(buttonStop, true);
      focusCounter.ShowControls();
      breakCounter.ShowControls();
    }
  }

  showInfo = pomodoroController.IsShowingInfo();
  if (showInfo.IsUpdated()) {
    lv_obj_set_hidden(infoPopupButton, !showInfo.Get());
  }

  focusValue = pomodoroController.FocusDuration();
  if (focusValue.IsUpdated()) {
    focusCounter.SetValue(focusValue.Get());
  }

  breakValue = pomodoroController.BreakDuration();
  if (breakValue.IsUpdated()) {
    breakCounter.SetValue(breakValue.Get());
  }

  interval = pomodoroController.Interval();
  if (interval.IsUpdated()) {
    switch (interval.Get()) {
      case Pinetime::Controllers::PomodoroController::IntervalType::Break:
        lv_label_set_text_static(buttonIntervalLabel, "BREAK");
        break;
      case Pinetime::Controllers::PomodoroController::IntervalType::Focus:
        lv_label_set_text_static(buttonIntervalLabel, "FOCUS");
        break;
    }
  }

  enabled = pomodoroController.IsEnabled();
  if (enabled.IsUpdated()) {
    if (enabled.Get()) {
      lv_switch_on(enableSwitch, animation);
    } else {
      lv_switch_off(enableSwitch, animation);
    }
  }
}

bool Pomodoro::OnButtonPushed() {
  auto res = false;
  if (pomodoroController.IsShowingInfo()) {
    res = true;
    pomodoroController.UpdateShowInfo(false);
  }
  if (pomodoroController.IsAlerting()) {
    pomodoroController.StopAlarm();
    res = true;
  }
  if (res) {
    UpdateUI(LV_ANIM_ON);
    return true;
  }
  return false;
}

void Pomodoro::OnValueChanged() {
  pomodoroController.UpdateFocus(focusCounter.GetValue());
  pomodoroController.UpdateBreak(breakCounter.GetValue());
  UpdateUI(LV_ANIM_ON);
}

void Pomodoro::OnButtonEvent(lv_obj_t* obj, lv_event_t event) {
  if (event != LV_EVENT_CLICKED) {
    return;
  }
  if (obj == buttonStop) {
    pomodoroController.StopAlarm();
  } else if (obj == buttonInfo) {
    pomodoroController.UpdateShowInfo(true);
  } else if (obj == infoPopupButton) {
    pomodoroController.UpdateShowInfo(false);
  } else if (obj == enableSwitch) {
    pomodoroController.UpdateEnabled(lv_switch_get_state(enableSwitch));
  } else if (obj == buttonInterval) {
    pomodoroController.ToggleInterval();
  } else {
    return;
  }
  UpdateUI(LV_ANIM_ON);
}

bool Pomodoro::OnTouchEvent(Pinetime::Applications::TouchEvents event) {
  // Don't allow closing the screen by swiping while the alarm is alerting
  return pomodoroController.IsAlerting() && event == TouchEvents::SwipeDown;
}
