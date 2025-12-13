#include "components/pomodoro/PomodoroController.h"
#include "task.h"
#include <chrono>
#include <libraries/log/nrf_log.h>

using namespace Pinetime::Controllers;
using namespace std::chrono_literals;

PomodoroController::PomodoroController(Controllers::DateTime& dateTimeController, Controllers::FS& fs)
  : dateTimeController {dateTimeController}, fs {fs} {
}

void PomodoroController::Init(System::SystemTask* systemTask) {
  this->systemTask = systemTask;
  loadState();
  // alarmTimer = xTimerCreate("Alarm", 1, pdFALSE, this, SetOffAlarm);
  // if (alarm.isEnabled) {
  //   NRF_LOG_INFO("[AlarmController] Loaded alarm was enabled, scheduling");
  //   ScheduleAlarm();
  // }
}

void PomodoroController::SaveState() {
  if (stateChanged) {
    saveState();
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
  if (state.isEnabled != isEnabled) {
    stateChanged = true;
    state.isEnabled = isEnabled;
  }
}

bool PomodoroController::IsAlerting() {
  return isAlerting;
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
