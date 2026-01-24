#include "components/pomodoro/PomodoroController.h"
#include "systemtask/SystemTask.h"
#include "task.h"
#include <chrono>
#include <libraries/log/nrf_log.h>
#include <systemtask/Messages.h>

using namespace Pinetime::Controllers;
using namespace std::chrono_literals;

PomodoroController::PomodoroController(Controllers::DateTime& dateTimeController,
                                       Controllers::FS& filesystem,
                                       Controllers::MotorController& motorController)
  : dateTimeController {dateTimeController}, fs {filesystem}, motorController {motorController} {
}

namespace {
  void alarmTimerStartHandler(TimerHandle_t xTimer) {
    auto* controller = static_cast<PomodoroController*>(pvTimerGetTimerID(xTimer));
    controller->StartAlarm();
  }

  void alarmTimerStopHandler(TimerHandle_t xTimer) {
    auto* controller = static_cast<PomodoroController*>(pvTimerGetTimerID(xTimer));
    controller->StopAlarm();
  }
}

void PomodoroController::Init(System::SystemTask* systemTask) {
  this->systemTask = systemTask;
  loadState();
  alarmTimer = xTimerCreate("PomodoroAlarmStart", 1, pdFALSE, this, alarmTimerStartHandler);
  alarmStopTimer = xTimerCreate("PomodoroAlarmStop", 1, pdFALSE, this, alarmTimerStopHandler);
  if (IsEnabled()) {
    NRF_LOG_INFO("[PomodoroController] Scheduling the alarm");
    ScheduleAlarm();
  }
}

void PomodoroController::SaveState() {
  if (stateChanged) {
    saveState();
  }
}

uint32_t PomodoroController::SecondsLeft() {
  auto now = dateTimeController.CurrentDateTime();
  if (alarmTime >= now) {
    return std::chrono::duration_cast<std::chrono::seconds>(alarmTime - now).count();
  }
  return 0;
}

void PomodoroController::ToggleInterval() {
  switch (Interval()) {
    case IntervalType::Break:
      UpdateInterval(IntervalType::Focus);
      break;
    case IntervalType::Focus:
      UpdateInterval(IntervalType::Break);
      break;
  }
}

void PomodoroController::UpdateInterval(IntervalType intervalType) {
  if (state.intervalType == intervalType) {
    return;
  }
  stateChanged = true;
  state.intervalType = intervalType;
  if (IsEnabled()) {
    ScheduleAlarm();
  }
}

void PomodoroController::UpdateFocus(uint8_t focusDuration) {
  if (state.focusDuration != focusDuration) {
    stateChanged = true;
    state.focusDuration = focusDuration;
  }
}

void PomodoroController::UpdateBreak(uint8_t breakDuration) {
  if (state.breakDuration != breakDuration) {
    stateChanged = true;
    state.breakDuration = breakDuration;
  }
}

void PomodoroController::UpdateEnabled(bool isEnabled) {
  if (state.isEnabled == isEnabled) {
    return;
  }
  stateChanged = true;
  state.isEnabled = isEnabled;
  if (isEnabled) {
    ScheduleAlarm();
  } else {
    xTimerStop(alarmTimer, 0);
  }
}

// DisplayApp::LoadNewScreen disables the motor
void PomodoroController::UpdateRinging() {
  if (isAlerting) {
    startRinging();
  }
}

void PomodoroController::StartAlarm() {
  if (isAlerting) {
    return;
  }
  startRinging();
  isAlerting = true;
  systemTask->PushMessage(System::Messages::OnPomodoroAlarm);
}

void PomodoroController::startRinging() {
  motorController.StartRinging();
  xTimerStop(alarmStopTimer, 0);
  xTimerChangePeriod(alarmStopTimer, 1 * 60 * configTICK_RATE_HZ, 0);
  xTimerStart(alarmStopTimer, 0);
}

void PomodoroController::StopAlarm() {
  if (!isAlerting) {
    return;
  }
  motorController.StopRinging();
  xTimerStop(alarmStopTimer, 0);
  xTimerStop(alarmTimer, 0);
  isAlerting = false;
  if (IsEnabled()) {
    ToggleInterval();
    ScheduleAlarm();
  }
  systemTask->PushMessage(System::Messages::OnPomodoroAlarmStop);
}

void PomodoroController::ScheduleAlarm() {
  uint8_t duration = 0;
  switch (Interval()) {
    case Break:
      duration = BreakDuration();
      break;
    case Focus:
      duration = FocusDuration();
      break;
  }

  if (duration > 0) {
    alarmTime = dateTimeController.CurrentDateTime() + std::chrono::minutes(duration);
    xTimerChangePeriod(alarmTimer, duration * 60 * configTICK_RATE_HZ, 0);
    xTimerStart(alarmTimer, 0);
  }
}

void PomodoroController::saveState() {
  lfs_dir stateDir {};
  lfs_file_t stateFile;

  if (fs.DirOpen("/.system", &stateDir) != LFS_ERR_OK) {
    fs.DirCreate("/.system");
  }
  fs.DirClose(&stateDir);

  if (fs.FileOpen(&stateFile, "/.system/pomodoro.dat", LFS_O_WRONLY | LFS_O_CREAT) != LFS_ERR_OK) {
    NRF_LOG_WARNING("[PomodoroController] Could not open state file to save state");
    return;
  }

  fs.FileWrite(&stateFile, reinterpret_cast<const uint8_t*>(&state), sizeof(state));
  fs.FileClose(&stateFile);
  NRF_LOG_INFO("[PomodoroController] Saved pomodoro state");
}

void PomodoroController::loadState() {
  lfs_file_t stateFile;
  PomodoroState curState;

  if (fs.FileOpen(&stateFile, "/.system/pomodoro.dat", LFS_O_RDONLY) != LFS_ERR_OK) {
    NRF_LOG_WARNING("[PomodoroController] Failed to open the pomodoro state file");
    return;
  }

  fs.FileRead(&stateFile, reinterpret_cast<uint8_t*>(&curState), sizeof(curState));
  fs.FileClose(&stateFile);
  state = curState;
  NRF_LOG_INFO("[PomodoroController] Loaded pomodoro state from file");
}
