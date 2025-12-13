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

    private:
      Controllers::PomodoroController& pomodoroController;
      System::WakeLock wakeLock;
      Controllers::MotorController& motorController;

      lv_obj_t *buttonStop, *buttonStopLabel, *enableSwitch, *buttonInfo;
      lv_obj_t* buttonInfoPopup = nullptr;
      lv_obj_t* buttonInfoPopupLabel = nullptr;

      Widgets::Counter focusCounter = Widgets::Counter(0, 59, jetbrains_mono_76);
      Widgets::Counter breakCounter = Widgets::Counter(0, 59, jetbrains_mono_76);

      void toggleSwitch();
      void showInfo();
      void hideInfo();
      void updateDuration();
      void stopAlerting();
      void startAlerting();
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
