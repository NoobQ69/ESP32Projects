#ifndef WATER_TANK_OBJ_PROJECT
#define WATER_TANK_OBJ_PROJECT

enum water_tank_parameters
{
  WT_TANK_1 = 0, 
  WT_TANK_2 = 1, 
  WT_TANK_3 = 2, 
  WT_TANK_4 = 3, 
  WT_TANK_5 = 4, 
  WT_TANK_6 = 5, 
  WT_TANK_7 = 6, 
  WT_TANK_8 = 7, 

  WT_OFF_STATE = 0,
  WT_ON_STATE = 1,
  WT_IDLE_STATE = 2,
  WT_DATA_READY = 200,

  WT_DASHBOARD_REQUEST,
  WT_DASHBOARD_SENSOR_REQUEST,
  WT_CONTROLLER_REQUEST,
  WT_PROMPT_REQUEST,
  WT_GLOBAL_REQUEST,
};

typedef struct
{
  int rtc_time_state;
  int rtc_module_flag_error;
  int ntp_flag_error;
  int rtc_module_connection;

} rtc_flags_t;

typedef struct 
{
  time_attendance_t time;
  rtc_flags_t rtc_flags;
} rtc_time_t;

typedef struct
{
  int tank_id;
  unsigned int current_volume;
  unsigned int upper_limit;
  unsigned int lower_limit;
  unsigned int capacity;
  bool state;
  bool protection;
  bool mode;
  bool leakage;

  bool is_calibrated_flag;
  bool load_state_flag;
  bool flush_state_flag;

} Water_Tank_t;

#endif