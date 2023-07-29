/*
          _____ ____  ____   _      __    __  ______    __  __ __ 
         / ___/|    \|    \ | |    |  |__|  ||      |  /  ]|  |  |
        (   \_ |  o  )     || |    |  |  |  ||      | /  / |  |  |
         \__  ||   _/|  |  || |___ |  |  |  ||_|  |_|/  /  |     |
         /  \ ||  |  |  |  ||     ||  '  '  |  |  | /   \_ |  |  |
         \    ||  |  |  |  ||     | \      /   |  | \     ||  |  |
          \___||__|  |__|__||_____|  \_/\_/    |__|  \____||__|__|

                       Smart Programmable Next-Level Wearable Tech
*/
// https://github.com/Uberi/Arduino-HardwareBLESerial/blob/master/README.md
#include <HardwareBLESerial.h>
#include <SPI.h>
#include "gui.h"
#include "fonts.h"
#include "buttonhandling.h"
#include "NRF52_MBED_TimerInterrupt.h"

#define DEVICE_NAME "Bangle.js"

//#define bit(b) (1UL << (b))
#define WIDTH_8 18
#define WIDTH 144
#define HEIGHT 168

#define GREY 2
#define WHITE 1
#define BLACK 0

#define LED_BLUE_PIN LEDB
#define LED_RED_PIN  LED_BUILTIN /* Same as LED_BUILTIN */
#define LED_GREEN_PIN  LEDG

#define RUMBLE_PIN 2
int rumbleTimer = 0;

#define CHARGE_LED 23          // P0_17 = 17  D23   YELLOW CHARGE LED
volatile int BAT_CHRG; //is the battery connected and charging?   5V connected/charging = 0; disconnected = 1
int batList[128];
int batNum = 0;
int charge_frame = 0;

uint8_t gScale = 1;

const uint8_t sprite[] PROGMEM =
{
16,22,
1,192, 14,32, 48,32, 64,32, 128,16, 128,16, 64,23, 33,249, 30,6, 96,250, 159,2, 224,1, 32,1, 32,1, 32,81, 16,82, 16,2, 8,4, 6,24, 5,240, 4,40, 7,248, 
};

#define RUMBLE_PIN 2
int CS_PIN = 8;
int fCount=0;
int where = 0; // current offset of screen buffer

char sBuff[WIDTH*HEIGHT/8]; // 1bpp buffer (8 pixels per byte)

#define _SHARPMEM_BIT_WRITECMD 0x80
#define _SHARPMEM_BIT_VCOM 0x40
#define _SHARPMEM_BIT_CLEAR 0x20

int vCom = 1;
long unsigned int lastMillis = 0;
int fpsCount = 0;
int framecount = 0;

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

HardwareBLESerial &bleSerial = HardwareBLESerial::getInstance();

// Init NRF52 timer NRF_TIMER3
NRF52_MBED_Timer ITimer0(NRF_TIMER_3);
#define TIMER0_INTERVAL_MS        1000

int calculateDayOfWeek(int day, int month, int year) {
  if (month < 3) {
    month += 12;
    year--;
  }
  int h = ((day+1) + 2*(month + 1) + 3*(month + 1)/5 + year + year/4 - year/100 + year/400) % 7;
  return ((h + 5) % 7);
}


void TimerHandler0()
{
  int allSeconds = millis()/1000;
  if(oldSec != allSeconds){
    oldSec = allSeconds;
    if(++myTime[SECOND]==60){
      myTime[SECOND]=0;
      if(++myTime[MINUTE]==60){
        myTime[MINUTE]=0;
        if(++myTime[HOUR]==24){
          myTime[HOUR]=0;
          int today = monthDays[myTime[MONTH]];
          if(myTime[MONTH]==1){ // febuary
            if(LEAP_YEAR(myTime[YEAR])){
              today++;
            }
          }
          if(++myTime[DAY]>=today){
            myTime[DAY]=0;
            if(++myTime[MONTH]==13){ // months are 1 to 12 for some reason
              myTime[MONTH]=0;
              myTime[YEAR]++;
            }
          }
        }
      }
    }
  }
}

int stringLength(const char* text){
  int x=0;
  uint8_t numChars = strlen(text);
  for (uint8_t t = 0; t < numChars; t++) {
    uint8_t character = text[t] - 32;
    x+=(pgm_read_byte(&font01_widths[character])*gScale);
  }
  return x;
}

void print(int x, int y, const char* text){
  uint8_t numChars = strlen(text);
  for (uint8_t t = 0; t < numChars; t++) {
    uint8_t character = text[t] - 32;
    drawSprite(&font01[character][0], x, y);
    x+=(pgm_read_byte(&font01_widths[character])*gScale);
  }
}



void setup() {

  pinMode(P0_13, OUTPUT); // to set charging speed
  digitalWrite(P0_13, LOW); // HIGH = 50mA, LOW = 100mA
  pinMode(P0_14, OUTPUT); // Must be set LOW for battery charging
  digitalWrite(P0_14,LOW); // If this pin is not low, the battery voltage will be too high and burn out the pin.
  pinMode(P0_31, INPUT); //read battery vlotage?
  pinMode (CHARGE_LED, INPUT);  //sets to detetct if charge LED is on or off to see if USB is plugged in
  analogReference(AR_INTERNAL2V4);  //Vref=2.4V
  analogReadResolution(12);         //12bits
    
  pinMode(RUMBLE_PIN, OUTPUT);
  digitalWrite(RUMBLE_PIN, HIGH);

  pinMode(BTN_0, INPUT_PULLUP);
  pinMode(BTN_1, INPUT_PULLUP);
  pinMode(BTN_2, INPUT_PULLUP);
  pinMode(BTN_3, INPUT_PULLUP);


  pinMode(LED_RED_PIN,  OUTPUT);
  pinMode(LED_BLUE_PIN, OUTPUT);
  pinMode(LED_GREEN_PIN, OUTPUT);
  digitalWrite(LED_RED, HIGH);
  digitalWrite(LED_BLUE, HIGH);
  digitalWrite(LED_GREEN, HIGH);

  bleSerial.beginAndSetupBLE(DEVICE_NAME); // Start the BLE Serial thingy

  Serial.begin(9600);
  SPI.begin();
  // begin initialization
  Serial.println("Started Watch.");

  pinMode(CS_PIN, OUTPUT);
  digitalWrite(CS_PIN, LOW);
  
  // clear the gfx buffer.
  memset(sBuff, 0x00, WIDTH*HEIGHT/8);

  lastMillis = millis();
  // Interval in microsecs
  ITimer0.attachInterruptInterval(TIMER0_INTERVAL_MS, TimerHandler0);

  digitalWrite(RUMBLE_PIN, LOW);

  for(int t=0; t<128; t++){
    readBattery();
  }

  // Minimizes power when bluetooth is used
  NRF_POWER->DCDCEN = 1;


/*
//Disabling UART0 (saves around 300-500µA) - @Jul10199555 contribution
NRF_UART0->TASKS_STOPTX = 1;
NRF_UART0->TASKS_STOPRX = 1;
NRF_UART0->ENABLE = 0;

*(volatile uint32_t *)0x40002FFC = 0;
*(volatile uint32_t *)0x40002FFC;
*(volatile uint32_t *)0x40002FFC = 1; //Setting up UART registers again due to a library issue
*/

    Serial.println("Starting main loop.");

}

void loop() {


//////////////////////////////////////////////////////////////////////////////////////////////////
/*
//  if(BLE_Connected){
  // Put nrf52840 into low power mode
  NRF_POWER->TASKS_LOWPWR = 1;
  
  // Wake up after 1 second
  NRF_RTC0->CC[0] = 500;
  NRF_RTC0->EVTENSET = RTC_EVTENSET_COMPARE0_Msk;
  NRF_RTC0->TASKS_CLEAR = 1;
  NRF_RTC0->TASKS_START = 1;

  // Wait for RTC interrupt
  while (!NRF_RTC0->EVENTS_COMPARE[0]){
    // CPU in sleep mode waiting for interrupt
    __SEV(); 
    __WFE();
    __WFE();
  }
  
  // Clear RTC interrupt
  NRF_RTC0->EVENTS_COMPARE[0] = 0;
*/
//////////////////////////////////////////////////////////////////////////////////////////////////

  // this must be called regularly to perform BLE updates
  bleSerial.poll();
  while (bleSerial.availableLines() > 0) {
    Serial.print("GB said: ");
    char line[1024]; bleSerial.readLine(line, 1024);

    // Extract the information we're interested in
    char *setTimeStr = strstr(line, "setTime(");
    char *setTimeZoneStr = strstr(line, "E.setTimeZone(");
    char *setGPSStr = strstr(line, "is_gps_active");
    char *findWatchstr = strstr(line, "t:\"find\",n:true");

    if (findWatchstr != NULL) {
      rumbleTimer = 120; // 2 seconds?
    }

    if (setGPSStr != NULL) {
      bleSerial.println("{t:\"gps_power\", status:true}.");
      bleSerial.println("{t:\"ver:\", fw1:0.01, fw2:0.2}.");
    }

    // find timezone first, we might need it to update the clock
    if (setTimeZoneStr != NULL) {
      // Extract the value of n from E.setTimeZone(n)
      int setTimeZoneValue = atoi(setTimeZoneStr + 14);
      timeZone = setTimeZoneValue;
    }

    if (setTimeStr != NULL) {
      // Extract the value of nnnnnnnnnn from setTime(nnnnnnnnnn)
      long setTimeValue = atol(setTimeStr + 8);
      convertUnixTime(setTimeValue);
    }
    
    Serial.println(line);
  }

  // Serial data is pending
  if (Serial.available()) 
  { 
    // Echo serial data on serial device
    bleSerial.write(Serial.read());
  }  

  
    // Call update function
    if(digitalRead(CHARGE_LED)){
      digitalWrite(LED_GREEN, HIGH); // use the brighter green LED to show charging (for now)
      update();
    }else{
      digitalWrite(LED_GREEN, LOW); // use the brighter green LED to show charging (for now)
      chargingScreen();
    }

//  }else{
//    no_connection_screen();
//  }
  //bleSerial.end();

}

void writeScreen(){
  digitalWrite(CS_PIN, HIGH);
  SPI.beginTransaction( SPISettings(8000000, LSBFIRST, SPI_MODE0) ); // 8mhz as fast as the nrf52840 will go, but that's 100fps, so that's OK.
  // add vCom to the 'send data' command
  SPI.transfer(_SHARPMEM_BIT_WRITECMD | (vCom ? _SHARPMEM_BIT_VCOM : 0));

  #define numLines 8
  uint8_t fullLine[numLines * 20]; // 8 lines of screen data
  uint16_t index = 0; // index of buffer to read from

  for(uint16_t line=1; line<=168; line+=8){
    
    for(uint16_t i=0; i<numLines; i++){
      index = (line + i - 1) * 18;
      fullLine[i * 20] = reverse_byte(line + i);
      for(int x=1; x<19; x++){
        fullLine[(i*20)+x] = sBuff[index + x - 1];
      }
      fullLine[i * 20 + 19] = 0x00;
    }
    
    SPI.transfer(&fullLine, numLines * 20);
  }

  SPI.transfer(0x00);
  digitalWrite(CS_PIN, LOW);
  vCom ^= 1;
  gScale = 1;

}


inline unsigned char reverse_byte(unsigned char b) {
   b = (b & 0xF0) >> 4 | (b & 0x0F) << 4;
   b = (b & 0xCC) >> 2 | (b & 0x33) << 2;
   b = (b & 0xAA) >> 1 | (b & 0x55) << 1;
   return b;
}

uint8_t GreyPattern[]={0,1,1,0};

inline void drawPixel(int x, int y, uint8_t color){
  if(x<0 || y<0 || x>=WIDTH || y>=HEIGHT) return;
  uint16_t x8 = x>>3;
  if(color > 1){
    color = GreyPattern[(x&1)+2*(y&1)];
  }
  if (color){
    sBuff[x8 + y * WIDTH_8] |= (128>>(x&7));
  }else{
    sBuff[x8 + y * WIDTH_8] &= ~(128>>(x&7));
  }
}

inline void fillRect(int x, int y, int width, int height, int colour){
  if(x<0 || y<0 || x>=WIDTH || y>=HEIGHT) return;
  for(int y1=y; y1<y+height; y1++){
    for(int x1=x; x1<x+width; x1++){
      drawPixel(x1, y1, colour);
    }
  }
}

void drawSprite(const uint8_t* sprite_data, int dx, int dy){
  int byteWidth = (pgm_read_byte(&sprite_data[0])+7)/8;
  for(int x=0; x<pgm_read_byte(&sprite_data[0]); x++){
    uint8_t x_d8 = x/8;
    int bitNum = 7-(x&7);
    for(int y=0; y<pgm_read_byte(&sprite_data[1]); y++){
      int byteNum = x_d8+(y*byteWidth);
      if((pgm_read_byte(&sprite_data[2+byteNum])>>bitNum)&1){
        if(gScale==1){
          drawPixel(dx + x, dy + y, WHITE);
        }else{
          fillRect(dx + (x*gScale), dy + (y*gScale), gScale, gScale, WHITE);
        }
      }else{
        if(gScale==1){
          drawPixel(dx + x, dy + y, BLACK);
        }else{
          fillRect(dx + (x*gScale), dy + (y*gScale), gScale, gScale, BLACK);
        }
      }
    }    
  }
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
  snprintf(datetime, sizeof(datetime), "%04u-%02u-%02u %02u:%02u:%02u", year, month + 1, days+1, hours, minutes, seconds);

  myTime[YEAR] = year;
  myTime[MONTH] = month+1;
  myTime[DAY] = days+1;
  myTime[HOUR] = (hours+timeZone) % 24;
  myTime[MINUTE] = minutes;
  myTime[SECOND] = seconds;
}



bool getDateTime(){
  char str[32];
  int s = 0;
  while(Serial.available() > 0){
    str[s++]=Serial.read();
    str[s]='\0';    
  }
  Serial.println(str);
  int Hour, Min, Sec;
  int Year, Month, Day;
  if (sscanf(str, "%d:%d:%d:%d:%d:%d", &Year, &Month, &Day, &Hour, &Min, &Sec) != 6) return false;
  myTime[HOUR] = Hour;
  myTime[MINUTE] = Min;
  myTime[SECOND] = Sec;
  myTime[YEAR] = Year;
  myTime[MONTH] = Month;
  myTime[DAY] = Day;

  referenceMillis = millis();
  globalMillis = (Hour * 60 * 60 * 1000) + (Min * 60 * 1000) + (Sec * 1000);
  
  return true;
}


void checkBLE(){
}


void update(){

  updateButtons();

  BAT_CHRG = digitalRead(CHARGE_LED);

  memset(sBuff, 0x00, WIDTH*HEIGHT/8);
  
  
  //gScale = 4;
  //drawSprite(sprite, 32, 40);

  char tempText[18];
  //sprintf(tempText,"%d FPS", fpsCount);
  float vBat = readBattery();
  //int batPercent = (vBat / 3.7)*100;
  sprintf(tempText,"%.2fv", vBat);
  gScale = 2;
  print(0,12,tempText);

  fillRect(0,35, 144,10, GREY); // will be battery percent later

  char buf[21];
  //sprintf(buf,"%02d:%02d:%02d", myTime[HOUR], myTime[MINUTE], myTime[SECOND]);
  sprintf(buf,"%02d:%02d", myTime[HOUR], myTime[MINUTE]);
  gScale = 6;
  int tw = (144 - stringLength(buf)) / 2;
  print(tw,58,buf);

  //int wd = dayOfWeek(myTime[YEAR], myTime[MONTH], myTime[DAY]);
  int wd = calculateDayOfWeek(myTime[DAY], myTime[MONTH], myTime[YEAR]);
  gScale = 2;
  print(5,120,wDaysFull[wd]);

  fillRect(0,100, 144,10, GREY);
  fillRect(wd*18,100, 18,10, WHITE);

  sprintf(buf,"%04d-%02d-%02d", myTime[YEAR], myTime[MONTH], myTime[DAY]);
  gScale = 2;
  print(5,140,buf);


  // check buttons
  if(_BBack[HELD]){
    digitalWrite(LED_RED, HIGH);
    digitalWrite(LED_BLUE, HIGH);
    digitalWrite(LED_GREEN, HIGH);
  }

  if(_BUp[NEW]){
    digitalWrite(LED_RED, LOW);
  }

  if(_BSelect[HELD]){
    digitalWrite(LED_GREEN, LOW);
    displayCard();
  }

  if(_BDown[NEW]){
    digitalWrite(LED_BLUE, LOW);
  }


  writeScreen();

  framecount++;
  if(millis() >= lastMillis+1000){
    lastMillis = millis();
    fpsCount = framecount;
    framecount = 0;
  }

  if(rumbleTimer >0){
    rumbleTimer--;
    digitalWrite(RUMBLE_PIN, HIGH);
  }else{
    digitalWrite(RUMBLE_PIN, LOW);
  }


}


void chargingScreen(){
  updateButtons();

  float vBat = readBattery();

  memset(sBuff, 0x00, WIDTH*HEIGHT/8);

  // to be replaced with battery later
  char tempText[18];
  //int batPercent = (vBat / 3.7)*100;
  sprintf(tempText,"%.2fv", vBat);
  gScale = 3;
  //print(0,12,tempText);
  gScale = 1;
  drawSprite(charge_icon, 17, 35);
  drawSprite(&charge_anim[charge_frame--][0], 25, 59);
  if(charge_frame<0) charge_frame=4;
  
  char buf[21];
  gScale = 3;
  sprintf(buf,"%02d:%02d:%02d", myTime[HOUR], myTime[MINUTE], myTime[SECOND]);
  int tw = (144 - stringLength(buf)) / 2;
  print(tw,140,buf);

  writeScreen();
}

float readBattery(){
  int Vadc = analogRead(P0_31);
  //float vBat = ((510e3 + 1000e3) / 510e3) * 2.4 * Vadc / 4096;
  float vBat = Vadc / 565;
  batList[batNum] = vBat * 256;
  (++batNum)&=127;
  int bTotal = 0;
  for(int t=0; t<128; t++){
    bTotal += batList[t];
  }
  bTotal /= 256;
  return float(bTotal/128.0);
}

void no_connection_screen(){
  gScale==1;
  memset(sBuff, 0x00, WIDTH*HEIGHT/8);
  drawSprite(no_connection, 12, 26);
  writeScreen();
}

void displayCard(){
  gScale==1;
  // clear the screen
  memset(sBuff, 0x00, WIDTH*HEIGHT/8);
  // banner thingy for title
  fillRect(0, 0, 144, 32, GREY);
  // Title
  print(1, 1, "Alert!");
  writeScreen();
}
