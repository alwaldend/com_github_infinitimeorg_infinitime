#pragma once

#include "displayapp/apps/Apps.h"
#include "displayapp/screens/Screen.h"
#include "displayapp/widgets/Counter.h"
#include "displayapp/Controllers.h"
#include "utility/DirtyValue.h"
#include "Symbols.h"
#include <components/pomodoro/PomodoroController.h>

namespace Pinetime::Applications {
  namespace Screens {
    class Pomodoro : public Screen {
    public:
      explicit Pomodoro(Controllers::PomodoroController& pomodoroController);
      ~Pomodoro() override;

      bool OnButtonPushed() override;
      bool OnTouchEvent(TouchEvents event) override;

      void OnValueChanged();
      void OnButtonEvent(lv_obj_t* obj, lv_event_t event);
      void StopAlarm();
      void UpdateUI(lv_anim_enable_t animation);

    private:
      Controllers::PomodoroController& pomodoroController;

      lv_obj_t *buttonStop, *buttonStopLabel;
      Utility::DirtyValue<bool> alerting;

      lv_obj_t *buttonInterval, *buttonIntervalLabel;
      Utility::DirtyValue<Pinetime::Controllers::PomodoroController::IntervalType> interval;

      Widgets::Counter focusCounter = Widgets::Counter(1, 59, jetbrains_mono_76);
      Utility::DirtyValue<uint8_t> focusValue;
      Widgets::Counter breakCounter = Widgets::Counter(1, 59, jetbrains_mono_76);
      Utility::DirtyValue<uint8_t> breakValue;

      lv_obj_t* enableSwitch;
      Utility::DirtyValue<bool> enabled;

      lv_obj_t* buttonInfo;
      void updateInfoButtonUI();

      lv_obj_t *infoPopupButton, *infoPopupButtonLabel;
      Utility::DirtyValue<bool> showInfo;
    };
  }

  template <>
  struct AppTraits<Apps::Pomodoro> {
    static constexpr Apps app = Apps::Pomodoro;
    static constexpr const char* icon = Screens::Symbols::clock;

    static Screens::Screen* Create(AppControllers& controllers) {
      return new Screens::Pomodoro(controllers.pomodoroController);
    };
  };
}
