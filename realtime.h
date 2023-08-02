long unsigned int lastMillis = 0;
int fpsCount = 0;
int framecount = 0;

long unsigned int startMillis = 0;

#define HOUR 0
#define MINUTE 1
#define SECOND 2
#define YEAR 3
#define MONTH 4
#define DAY 5
#define LEAP_YEAR(Y)     ( (Y>0) && !(Y%4) && ( (Y%100) || !(Y%400) ))     // from time-lib

int myTime[]={0,0,0,0,0,0,0,0,0,0};
int timeZone = 0;

int oldSec = 0; // track when seconds change to update screen
const char *wDays[]={"Sun","Mon","Tue","Wed","Thu","Fri","Sat"};
const char *wDaysFull[]={"Sunday","Monday","Tuesday","Wednesday","Thursday","Friday","Saturday"};
const char *mDays[]={"Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sep","Nov","Dec"};
const char *mDaysFull[]={"January","Febuary","March","April","May","June","July","August","September","November","December"};
const int monthDays[]={31,28,31,30,31,30,31,31,30,31,30,31};
unsigned long globalMillis = 0;
unsigned long referenceMillis = 0;




int calculateDayOfWeek(int day, int month, int year) {
  if (month < 3) {
    month += 12;
    year--;
  }
  int h = ((day+1) + 2*(month + 1) + 3*(month + 1)/5 + year + year/4 - year/100 + year/400) % 7;
  return ((h + 5) % 7);
}



void convertUnixTime(unsigned long timestamp) {

  uint32_t seconds, minutes, hours, days, year, month;
  uint32_t dayOfWeek;
  seconds = timestamp;

  /* calculate minutes */
  minutes  = seconds / 60;
  seconds -= minutes * 60;
  /* calculate hours */
  hours    = minutes / 60;
  minutes -= hours   * 60;
  /* calculate days */
  days     = hours   / 24;
  hours   -= days    * 24;

  /* Unix time starts in 1970 on a Thursday */
  year      = 1970;
  dayOfWeek = 4;

  while(1)
  {
    bool     leapYear   = (year % 4 == 0 && (year % 100 != 0 || year % 400 == 0));
    uint16_t daysInYear = leapYear ? 366 : 365;
    if (days >= daysInYear)
    {
      dayOfWeek += leapYear ? 2 : 1;
      days      -= daysInYear;
      if (dayOfWeek >= 7)
        dayOfWeek -= 7;
      ++year;
    }
    else
    {
      //tm->tm_yday = days;
      dayOfWeek  += days;
      dayOfWeek  %= 7;

      /* calculate the month and day */
      static const uint8_t daysInMonth[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
      for(month = 0; month < 12; ++month)
      {
        uint8_t dim = daysInMonth[month];

        /* add a day to feburary if this is a leap year */
        if (month == 1 && leapYear)
          ++dim;

        if (days >= dim)
          days -= dim;
        else
          break;
      }
      break;
    }
  }
  
  // Step 10
  char datetime[20];
  snprintf(datetime, sizeof(datetime), "%04u-%02u-%02u %02u:%02u:%02u", year, month, days, hours, minutes, seconds);

  myTime[YEAR] = year;
  myTime[MONTH] = month+1;
  myTime[DAY] = days+1;
  myTime[HOUR] = (hours+timeZone) % 24;
  myTime[MINUTE] = minutes;
  myTime[SECOND] = seconds;
}
