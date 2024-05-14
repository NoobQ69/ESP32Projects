#ifndef RTC_UTILITIES
#define RTC_UTILITIES

#define DS1302_CLK 27
#define DS1302_DAT 26
#define DS1302_RST 25
#define countof(a) (sizeof(a) / sizeof(a[0]))

enum rtc_utilities_define 
{
  GET_NTP_TIME,
  GET_RTC_TIME, 

  RTC_MODULE_FAILED,
  RTC_MODULE_SUCCESS,
};

ThreeWire myWire(DS1302_DAT, DS1302_CLK, DS1302_RST);  // IO, SCLK, CE
RtcDS1302<ThreeWire> Rtc(myWire);

NTPtime NTPch("ch.pool.ntp.org");  // connect to Server NTP

typedef struct 
{
  int hour;
  int minute;
  int second;
  int week;
  int day;
  int month;
  int year;

} time_attendance_t;

String Months_of_the_year[] = {"","Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sep","Oct","Nov","Dec"};

bool get_ntp_date_time(time_attendance_t &time_atc) 
{
  strDateTime date_time = NTPch.getNTPtime(7.0, 0);  // GMT+7

  if (date_time.valid) 
  {
    time_atc.hour = date_time.hour;      
    time_atc.minute = date_time.minute;  
    time_atc.second = date_time.second; 
    time_atc.week = date_time.dayofWeek;
    time_atc.year = date_time.year;
    time_atc.month = date_time.month;
    time_atc.day = date_time.day;

    return true;
  }
  return false;
}

bool get_rtc_datetime(time_attendance_t &time_atc) 
{
  RtcDateTime now = Rtc.GetDateTime();

  if (now.IsValid())
  {
    time_atc.day = now.Day();
    time_atc.month = now.Month();
    time_atc.year = now.Year();
    time_atc.hour = now.Hour();
    time_atc.minute = now.Minute();
    time_atc.second = now.Second();

    return true;
  }
  return false;
}

String get_current_time_String_format(time_attendance_t &time_atc) 
{
  String time_str = "";
  time_str = String(time_atc.hour);
  time_str += ":";
  time_str += String(time_atc.minute);
  time_str += ":";
  time_str += String(time_atc.second);

  return time_str;
}

String get_current_date_String_format(time_attendance_t &time_atc) 
{
  String date_str = String(time_atc.day);
  date_str += "/";
  date_str += String(time_atc.month);
  date_str += "/";
  date_str += String(time_atc.year);
  
  return date_str;
}

bool rtc_init(String date_set = "", String time_set = "")
{
  Rtc.Begin();

  RtcDateTime compiled = RtcDateTime(date_set.c_str(), time_set.c_str());

  if (!Rtc.IsDateTimeValid()) {
#ifdef DEBUG_ON
    Serial.println("RTC lost confidence in the DateTime!");
#endif
    Rtc.SetDateTime(compiled);
    return false;
  }

  if (Rtc.GetIsWriteProtected()) {
#ifdef DEBUG_ON
    Serial.println("RTC was write protected, enabling writing now");
#endif
    Rtc.SetIsWriteProtected(false);
    return false;
  }

  if (!Rtc.GetIsRunning()) {
#ifdef DEBUG_ON
    Serial.println("RTC was not actively running, starting now");
#endif
    Rtc.SetIsRunning(true);
    return false;
  }

  RtcDateTime now = Rtc.GetDateTime();
  if (now < compiled) {
#ifdef DEBUG_ON
    Serial.println("RTC is older than compile time!  (Updating DateTime)");
#endif
    Rtc.SetDateTime(compiled);
  } else if (now > compiled) {
#ifdef DEBUG_ON
    Serial.println("RTC is newer than compile time. (this is expected)");
#endif
  } else if (now == compiled) {
#ifdef DEBUG_ON
    Serial.println("RTC is the same as compile time! (not expected but all is fine)");
#endif
  }
  return true;
}

void print_date_time(time_attendance_t &time_atc) 
{
  char date_string[26];

  snprintf_P(date_string,
             countof(date_string),
             PSTR("%02u/%02u/%04u %02u:%02u:%02u"),
             time_atc.day,
             time_atc.month,
             time_atc.year,
             time_atc.hour,
             time_atc.minute,
             time_atc.second);

  Serial.println(date_string);
}

String convert_to_rtc_date_init_format(int month, int day, int year) {
  String date_str = "";
  date_str += Months_of_the_year[month];
  date_str += " ";
  date_str += String(day);
  date_str += " ";
  date_str += String(year);
  return date_str;
}

bool get_date_time(time_attendance_t &time_atc, int type = GET_RTC_TIME)
{
  if (type == GET_NTP_TIME)
  {
#ifdef DEBUG_ON
    Serial.print("NTP:");
#endif
    return get_ntp_date_time(time_atc);
  }
#ifdef DEBUG_ON
  Serial.print("RTC:");
#endif
  return get_rtc_datetime(time_atc);
}

#endif