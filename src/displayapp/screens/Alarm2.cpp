/*  Copyright (C) 2021 mruss77, Florian

    This file is part of InfiniTime.

    InfiniTime is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published
    by the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    InfiniTime is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/
#include "displayapp/screens/Alarm2.h"
#include "displayapp/screens/Screen.h"
#include "displayapp/screens/Symbols.h"
#include "displayapp/InfiniTimeTheme.h"
#include "components/settings/Settings.h"
#include "components/alarm/AlarmController2.h"
#include "components/motor/MotorController.h"
#include "systemtask/SystemTask.h"

using namespace Pinetime::Applications::Screens;
using Pinetime::Controllers::AlarmController2;

namespace {
  void ValueChangedHandler(void* userData) {
    auto* screen = static_cast<Alarm2*>(userData);
    screen->OnValueChanged();
  }
}

static void btnEventHandler(lv_obj_t* obj, lv_event_t event) {
  auto* screen = static_cast<Alarm2*>(obj->user_data);
  screen->OnButtonEvent(obj, event);
}

static void StopAlarmTaskCallback(lv_task_t* task) {
  auto* screen = static_cast<Alarm2*>(task->user_data);
  screen->StopAlerting();
}

Alarm2::Alarm2(Controllers::AlarmController2& alarmController2,
             Controllers::Settings::ClockType clockType,
             System::SystemTask& systemTask,
             Controllers::MotorController& motorController)
  : alarmController2 {alarmController2}, systemTask {systemTask}, motorController {motorController} {

  hourCounter.Create();
  lv_obj_align(hourCounter.GetObject(), nullptr, LV_ALIGN_IN_TOP_LEFT, 0, 0);
  if (clockType == Controllers::Settings::ClockType::H12) {
    hourCounter.EnableTwelveHourMode();

    lblampm = lv_label_create(lv_scr_act(), nullptr);
    lv_obj_set_style_local_text_font(lblampm, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, &jetbrains_mono_bold_20);
    lv_label_set_text_static(lblampm, "AM");
    lv_label_set_align(lblampm, LV_LABEL_ALIGN_CENTER);
    lv_obj_align(lblampm, lv_scr_act(), LV_ALIGN_CENTER, 0, 30);
  }
  hourCounter.SetValue(alarmController2.Hours());
  hourCounter.SetValueChangedEventCallback(this, ValueChangedHandler);

  minuteCounter.Create();
  lv_obj_align(minuteCounter.GetObject(), nullptr, LV_ALIGN_IN_TOP_RIGHT, 0, 0);
  minuteCounter.SetValue(alarmController2.Minutes());
  minuteCounter.SetValueChangedEventCallback(this, ValueChangedHandler);

  lv_obj_t* colonLabel = lv_label_create(lv_scr_act(), nullptr);
  lv_obj_set_style_local_text_font(colonLabel, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, &jetbrains_mono_76);
  lv_label_set_text_static(colonLabel, ":");
  lv_obj_align(colonLabel, lv_scr_act(), LV_ALIGN_CENTER, 0, -29);

  lv_obj_t* alarmLabel = lv_label_create(lv_scr_act(), nullptr);
  lv_obj_set_style_local_text_font(alarmLabel, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, &jetbrains_mono_bold_20);
  lv_label_set_text_static(alarmLabel, "A2");
  lv_obj_align(alarmLabel, lv_scr_act(), LV_ALIGN_CENTER, 0, 50);

  btnStop = lv_btn_create(lv_scr_act(), nullptr);
  btnStop->user_data = this;
  lv_obj_set_event_cb(btnStop, btnEventHandler);
  lv_obj_set_size(btnStop, 115, 50);
  lv_obj_align(btnStop, lv_scr_act(), LV_ALIGN_IN_BOTTOM_LEFT, 0, 0);
  lv_obj_set_style_local_bg_color(btnStop, LV_BTN_PART_MAIN, LV_STATE_DEFAULT, LV_COLOR_RED);
  txtStop = lv_label_create(btnStop, nullptr);
  lv_label_set_text_static(txtStop, Symbols::stop);
  lv_obj_set_hidden(btnStop, true);

  static constexpr lv_color_t bgColor = Colors::bgAlt;

  btnRecur = lv_btn_create(lv_scr_act(), nullptr);
  btnRecur->user_data = this;
  lv_obj_set_event_cb(btnRecur, btnEventHandler);
  lv_obj_set_size(btnRecur, 115, 50);
  lv_obj_align(btnRecur, lv_scr_act(), LV_ALIGN_IN_BOTTOM_RIGHT, 0, 0);
  txtRecur = lv_label_create(btnRecur, nullptr);
  SetRecurButtonState();
  lv_obj_set_style_local_bg_color(btnRecur, LV_BTN_PART_MAIN, LV_STATE_DEFAULT, bgColor);

  btnInfo = lv_btn_create(lv_scr_act(), nullptr);
  btnInfo->user_data = this;
  lv_obj_set_event_cb(btnInfo, btnEventHandler);
  lv_obj_set_size(btnInfo, 50, 50);
  lv_obj_align(btnInfo, lv_scr_act(), LV_ALIGN_IN_TOP_MID, 0, -4);
  lv_obj_set_style_local_bg_color(btnInfo, LV_BTN_PART_MAIN, LV_STATE_DEFAULT, bgColor);
  lv_obj_set_style_local_border_width(btnInfo, LV_BTN_PART_MAIN, LV_STATE_DEFAULT, 4);
  lv_obj_set_style_local_border_color(btnInfo, LV_BTN_PART_MAIN, LV_STATE_DEFAULT, LV_COLOR_BLACK);

  lv_obj_t* txtInfo = lv_label_create(btnInfo, nullptr);
  lv_label_set_text_static(txtInfo, "i");

  enableSwitch = lv_switch_create(lv_scr_act(), nullptr);
  enableSwitch->user_data = this;
  lv_obj_set_event_cb(enableSwitch, btnEventHandler);
  lv_obj_set_size(enableSwitch, 100, 50);
  // Align to the center of 115px from edge
  lv_obj_align(enableSwitch, lv_scr_act(), LV_ALIGN_IN_BOTTOM_LEFT, 7, 0);
  lv_obj_set_style_local_bg_color(enableSwitch, LV_SWITCH_PART_BG, LV_STATE_DEFAULT, bgColor);

  UpdateAlarmTime();

  if (alarmController2.State() == Controllers::AlarmController2::AlarmState::Alerting) {
    SetAlerting();
  } else {
    SetSwitchState(LV_ANIM_OFF);
  }
}

Alarm2::~Alarm2() {
  if (alarmController2.State() == AlarmController2::AlarmState::Alerting) {
    StopAlerting();
  }
  lv_obj_clean(lv_scr_act());
}

void Alarm2::DisableAlarm() {
  if (alarmController2.State() == AlarmController2::AlarmState::Set) {
    alarmController2.DisableAlarm();
    lv_switch_off(enableSwitch, LV_ANIM_ON);
  }
}

void Alarm2::OnButtonEvent(lv_obj_t* obj, lv_event_t event) {
  if (event == LV_EVENT_CLICKED) {
    if (obj == btnStop) {
      StopAlerting();
      return;
    }
    if (obj == btnInfo) {
      ShowInfo();
      return;
    }
    if (obj == btnMessage) {
      HideInfo();
      return;
    }
    if (obj == enableSwitch) {
      if (lv_switch_get_state(enableSwitch)) {
        alarmController2.ScheduleAlarm();
      } else {
        alarmController2.DisableAlarm();
      }
      return;
    }
    if (obj == btnRecur) {
      DisableAlarm();
      ToggleRecurrence();
    }
  }
}

bool Alarm2::OnButtonPushed() {
  if (txtMessage != nullptr && btnMessage != nullptr) {
    HideInfo();
    return true;
  }
  if (alarmController2.State() == AlarmController2::AlarmState::Alerting) {
    StopAlerting();
    return true;
  }
  return false;
}

bool Alarm2::OnTouchEvent(Pinetime::Applications::TouchEvents event) {
  // Don't allow closing the screen by swiping while the alarm is alerting
  return alarmController2.State() == AlarmController2::AlarmState::Alerting && event == TouchEvents::SwipeDown;
}

void Alarm2::OnValueChanged() {
  DisableAlarm();
  UpdateAlarmTime();
}

void Alarm2::UpdateAlarmTime() {
  if (lblampm != nullptr) {
    if (hourCounter.GetValue() >= 12) {
      lv_label_set_text_static(lblampm, "PM");
    } else {
      lv_label_set_text_static(lblampm, "AM");
    }
  }
  alarmController2.SetAlarmTime(hourCounter.GetValue(), minuteCounter.GetValue());
}

void Alarm2::SetAlerting() {
  lv_obj_set_hidden(enableSwitch, true);
  lv_obj_set_hidden(btnStop, false);
  taskStopAlarm = lv_task_create(StopAlarmTaskCallback, pdMS_TO_TICKS(60 * 1000), LV_TASK_PRIO_MID, this);
  motorController.StartRinging();
  systemTask.PushMessage(System::Messages::DisableSleeping);
}

void Alarm2::StopAlerting() {
  alarmController2.StopAlerting();
  motorController.StopRinging();
  SetSwitchState(LV_ANIM_OFF);
  if (taskStopAlarm != nullptr) {
    lv_task_del(taskStopAlarm);
    taskStopAlarm = nullptr;
  }
  systemTask.PushMessage(System::Messages::EnableSleeping);
  lv_obj_set_hidden(enableSwitch, false);
  lv_obj_set_hidden(btnStop, true);
}

void Alarm2::SetSwitchState(lv_anim_enable_t anim) {
  switch (alarmController2.State()) {
    case AlarmController2::AlarmState::Set:
      lv_switch_on(enableSwitch, anim);
      break;
    case AlarmController2::AlarmState::Not_Set:
      lv_switch_off(enableSwitch, anim);
      break;
    default:
      break;
  }
}

void Alarm2::ShowInfo() {
  if (btnMessage != nullptr) {
    return;
  }
  btnMessage = lv_btn_create(lv_scr_act(), nullptr);
  btnMessage->user_data = this;
  lv_obj_set_event_cb(btnMessage, btnEventHandler);
  lv_obj_set_height(btnMessage, 200);
  lv_obj_set_width(btnMessage, 150);
  lv_obj_align(btnMessage, lv_scr_act(), LV_ALIGN_CENTER, 0, 0);
  txtMessage = lv_label_create(btnMessage, nullptr);
  lv_obj_set_style_local_bg_color(btnMessage, LV_BTN_PART_MAIN, LV_STATE_DEFAULT, LV_COLOR_NAVY);

  if (alarmController2.State() == AlarmController2::AlarmState::Set) {
    auto timeToAlarm = alarmController2.SecondsToAlarm();

    auto daysToAlarm = timeToAlarm / 86400;
    auto hrsToAlarm = (timeToAlarm % 86400) / 3600;
    auto minToAlarm = (timeToAlarm % 3600) / 60;
    auto secToAlarm = timeToAlarm % 60;

    lv_label_set_text_fmt(txtMessage,
                          "Time to\nalarm 2:\n%2lu Days\n%2lu Hours\n%2lu Minutes\n%2lu Seconds",
                          daysToAlarm,
                          hrsToAlarm,
                          minToAlarm,
                          secToAlarm);
  } else {
    lv_label_set_text_static(txtMessage, "Alarm 2\nis not\nset.");
  }
}

void Alarm2::HideInfo() {
  lv_obj_del(btnMessage);
  txtMessage = nullptr;
  btnMessage = nullptr;
}

void Alarm2::SetRecurButtonState() {
  using Pinetime::Controllers::AlarmController2;
  switch (alarmController2.Recurrence()) {
    case AlarmController2::RecurType::None:
      lv_label_set_text_static(txtRecur, "ONCE");
      break;
    case AlarmController2::RecurType::Daily:
      lv_label_set_text_static(txtRecur, "DAILY");
      break;
    case AlarmController2::RecurType::Weekdays:
      lv_label_set_text_static(txtRecur, "MON-FRI");
  }
}

void Alarm2::ToggleRecurrence() {
  using Pinetime::Controllers::AlarmController2;
  switch (alarmController2.Recurrence()) {
    case AlarmController2::RecurType::None:
      alarmController2.SetRecurrence(AlarmController2::RecurType::Daily);
      break;
    case AlarmController2::RecurType::Daily:
      alarmController2.SetRecurrence(AlarmController2::RecurType::Weekdays);
      break;
    case AlarmController2::RecurType::Weekdays:
      alarmController2.SetRecurrence(AlarmController2::RecurType::None);
  }
  SetRecurButtonState();
}
