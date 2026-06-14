#pragma once

/*
 * Actions.h — Routing action channels (index 0 = triac; 1..NbActions-1 = GPIO/HTTP loads).
 * User: schedules and thresholds — /fr/user-guide/#guide-d2-planning-et-conditions
 * Actif holds regulation mode (MODE_* constants in balansun_regulation_modes.h).
 */
#include <Arduino.h>
#include "balansun_regulation_modes.h"

// --- Routing actions (schedule, GPIO/HTTP, triac) ---
class Action {
private:
  bool valide;
  int Idx;  //Index
  void CallExterne(String host,String url, int port);
  int GpioOn;
  int GpioOff;
  int OutOn;
  int OutOff;
  int T_LastAction=0;
  int tempoTimer=0;
  byte LastOverrideState=0;
 
  

public:
  Action();  // Default constructor
  Action(int aIdx);
  
  void parse_from_wire(String ligne);
  String serialize_to_wire();
  void initDefaultSchedule();
  bool sanitizePeriodSchedule();  
  void apply_regulation(float Pw, int wall_decihours, float Temperature);
  bool OverrideExpired(unsigned long nowMs);
  void ClearOverride();
  void SetOverride(byte state, byte triacPercent, unsigned long durationSec);
 
  byte schedule_type_at(int wall_decihours);
  byte current_triac_schedule_type(int wall_decihours, float Temperature);
  int threshold_min_at(int wall_decihours);
  int threshold_max_at(int wall_decihours);
  void InitGpio();
  void relay_on_from_triac_delay(int triac_delay_percent_percent);
  void apply_pwm_from_triac_delay_f(float triac_delay_percent_f);
  void tick_half_sine(bool phase_230v, uint8_t pulse_on, uint8_t &pulse_total, int &pulseCounter);
  void tick_pulse_train(uint8_t pulse_on, uint8_t pulse_total, int &pulseCounter);
  void StopOutputs();
  byte Actif;
  int Port;
  byte Kp;
  byte Ki;
  byte Kd;
  bool PID;
  int Repet;
  int Tempo;
  String title;
  String Host;
  String path_on;
  String path_off;
  byte periodCount;
  bool On;
  byte Type[8];
  int period_start[8];
  int period_end[8];
  int power_min[8];
  int power_max[8];
  int temp_min[8];
  int temp_max[8];
  byte OverrideState;
  byte OverrideTriacPercent;
  unsigned long OverrideUntilMillis;
};

#define ACTION_OVERRIDE_AUTO 0
#define ACTION_OVERRIDE_ON 1
#define ACTION_OVERRIDE_OFF 2
#define ACTION_OVERRIDE_TRIAC_FIXED 3





