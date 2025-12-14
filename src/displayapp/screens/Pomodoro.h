#pragma once

#include "displayapp/apps/Apps.h"
#include "displayapp/screens/Screen.h"
#include "displayapp/widgets/Counter.h"
#include "displayapp/Controllers.h"
#include "systemtask/WakeLock.h"
#include "Symbols.h"

namespace Pinetime::Applications {
  namespace Screens {
    class Pomodoro : public Screen {
    public:
      explicit Pomodoro(Controllers::PomodoroController& pomodoroController,
                        System::SystemTask& systemTask,
                        Controllers::MotorController& motorController);
      ~Pomodoro() override;

      bool OnButtonPushed() override;
      bool OnTouchEvent(TouchEvents event) override;
      void OnValueChanged();
      void OnButtonEvent(lv_obj_t* obj, lv_event_t event);
      void OnPomodoroAlarmTriggered();
      void StopAlarm();

    private:
      Controllers::PomodoroController& pomodoroController;
      System::WakeLock wakeLock;
      Controllers::MotorController& motorController;

      lv_obj_t *buttonStop, *buttonStopLabel;
      lv_task_t* taskStopAlarm = nullptr;

      lv_obj_t *buttonInterval, *buttonIntervalLabel;
      void updateIntervalButton();
      void onIntervalButtonPress();

      Widgets::Counter focusCounter = Widgets::Counter(0, 59, jetbrains_mono_76);
      Widgets::Counter breakCounter = Widgets::Counter(0, 59, jetbrains_mono_76);
      void updateDuration();

      lv_obj_t* enableSwitch;
      void onEnableSwitchPress();
      void updateSwitch(lv_anim_enable_t anim);

      lv_obj_t* buttonInfo;
      void onInfoButtonPress();

      lv_obj_t* infoPopupButton = nullptr;
      lv_obj_t* infoPopupButtonLabel = nullptr;
      void onInfoPopupButtonPress();
    };
  }

  template <>
  struct AppTraits<Apps::Pomodoro> {
    static constexpr Apps app = Apps::Pomodoro;
    static constexpr const char* icon = Screens::Symbols::clock;

    static Screens::Screen* Create(AppControllers& controllers) {
      return new Screens::Pomodoro(controllers.pomodoroController, *controllers.systemTask, controllers.motorController);
    };
  };
}
