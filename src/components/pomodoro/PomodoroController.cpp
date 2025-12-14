#include "components/pomodoro/PomodoroController.h"
#include "systemtask/SystemTask.h"
#include "task.h"
#include <chrono>
#include <libraries/log/nrf_log.h>
#include <systemtask/Messages.h>

using namespace Pinetime::Controllers;
using namespace std::chrono_literals;

PomodoroController::PomodoroController(Controllers::DateTime& dateTimeController, Controllers::FS& fs)
  : dateTimeController {dateTimeController}, fs {fs} {
}

namespace {
  void alarmTimerHandler(TimerHandle_t xTimer) {
    auto* controller = static_cast<PomodoroController*>(pvTimerGetTimerID(xTimer));
    controller->StartAlarm();
  }
}

void PomodoroController::Init(System::SystemTask* systemTask) {
  this->systemTask = systemTask;
  loadState();
  alarmTimer = xTimerCreate("Pomodoro", 1, pdFALSE, this, alarmTimerHandler);
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

void PomodoroController::StartAlarm() {
  if (IsEnabled()) {
    isAlerting = true;
    systemTask->PushMessage(System::Messages::OnPomodoroAlarm);
  }
}

uint32_t PomodoroController::SecondsLeft() {
  auto now = dateTimeController.CurrentDateTime();
  if (alarmTime >= now) {
    return std::chrono::duration_cast<std::chrono::seconds>(alarmTime - now).count();
  }
  return 0;
}

PomodoroController::IntervalType PomodoroController::Interval() {
  return state.intervalType;
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

void PomodoroController::UpdateDuration(uint8_t focusDuration, uint8_t breakDuration) {
  if (state.focusDuration != focusDuration) {
    stateChanged = true;
    state.focusDuration = focusDuration;
  }
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
  xTimerStop(alarmTimer, 0);
}

bool PomodoroController::IsAlerting() {
  return isAlerting;
}

void PomodoroController::StopAlarm() {
  isAlerting = false;
  systemTask->PushMessage(System::Messages::OnPomodoroAlarmStop);
}

uint8_t PomodoroController::FocusDuration() {
  return state.focusDuration;
}

uint8_t PomodoroController::BreakDuration() {
  return state.breakDuration;
}

bool PomodoroController::IsEnabled() {
  return state.isEnabled;
}

void PomodoroController::ScheduleAlarm() {
  xTimerStop(alarmTimer, 0);

  uint8_t duration = 0;
  switch (Interval()) {
    case Break:
      duration = BreakDuration();
      break;
    case Focus:
      duration = FocusDuration();
      break;
  }

  alarmTime = dateTimeController.CurrentDateTime() + std::chrono::minutes(duration);
  xTimerChangePeriod(alarmTimer, duration * 60 * configTICK_RATE_HZ, 0);
  xTimerStart(alarmTimer, 0);
  UpdateEnabled(true);
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
