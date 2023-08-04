/*
          _____ ____  ____   _      __    __  ______    __  __ __ 
         / ___/|    \|    \ | |    |  |__|  ||      |  /  ]|  |  |
        (   \_ |  o  )     || |    |  |  |  ||      | /  / |  |  |
         \__  ||   _/|  |  || |___ |  |  |  ||_|  |_|/  /  |     |
         /  \ ||  |  |  |  ||     ||  '  '  |  |  | /   \_ |  |  |
         \    ||  |  |  |  ||     | \      /   |  | \     ||  |  |
          \___||__|  |__|__||_____|  \_/\_/    |__|  \____||__|__|

                       Smart Programmable Next-Level Wearable Tech

       _______ ______ _______ _____   ________ _______ ______ _______ 
      |     __|   __ \    |  |     |_|  |  |  |_     _|      |   |   |
      |__     |    __/       |       |  |  |  | |   | |   ---|       |
      |_______|___|  |__|____|_______|________| |___| |______|___|___|
                                                              
                       
*/

// Without any power saving = 6.9mA
// The following gets 3.6mA
// NRF_POWER->DCDCEN = 1;

// https://github.com/Uberi/Arduino-HardwareBLESerial/blob/master/README.md
#include <HardwareBLESerial.h>
#include <SPI.h>
#include "fonts.h"
#include "screen.h"
#include "text.h"
#include "notifications.h"
#include "realtime.h"
#include "gui.h"
#include "buttonhandling.h"

#define DEVICE_NAME "Bangle.js" // Pretend to be a different smartwatch so we can use GadgetBridge


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


#define RUMBLE_PIN 2
int fCount=0;

HardwareBLESerial &bleSerial = HardwareBLESerial::getInstance();
bool gotTime = false;

int notificationTimeout=0;




void getMessages(){

  bleSerial.poll();
  while (bleSerial.availableLines() > 0) {
    Serial.print("GB said: ");
    char line[1024]; bleSerial.readLine(line, 1024);

    // Extract the information we're interested in
    char *setTimeStr = strstr(line, "setTime(");
    char *setTimeZoneStr = strstr(line, "E.setTimeZone(");
    char *setGPSStr = strstr(line, "is_gps_active");
    char *findWatchstr = strstr(line, "t:\"find\"");
    char *isNotification = strstr(line, "t:\"notify\"");


    if(isNotification != NULL){
      notificationTimeout = 150; // number of seconds(ish)

      char* srcSubstring = getSubstring(line, "src:");
      if (srcSubstring != NULL) {
        sprintf(notification[numNotifications].src,"%s", srcSubstring);
        free(srcSubstring);
      }

      char* titleSubstring = getSubstring(line, "title:");
      if (titleSubstring != NULL) {
        sprintf(notification[numNotifications].title,"%s", titleSubstring);
        free(titleSubstring);
      }

      char* bodySubstring = getSubstring(line, "body:");
      if (bodySubstring != NULL) {
        sprintf(notification[numNotifications].body,"%s", bodySubstring);
        free(bodySubstring);
      }


    }

    if (findWatchstr != NULL) {
      //    char *findWatchstr = strstr(line, "t:\"find\",n:true");
      if(strstr(line, "true")){
        rumbleTimer = 5000; // 5 minutes?
      }else{
        rumbleTimer = 0;
      }
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
      startMillis = setTimeValue;
      convertUnixTime(setTimeValue);
      gotTime = true;
    }
    
    Serial.println(line);
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

  pinMode(CS_PIN, OUTPUT);
  digitalWrite(CS_PIN, LOW);
  
  clearScreenBuffer();

  digitalWrite(RUMBLE_PIN, LOW);

  for(int t=0; t<128; t++){
    readBattery();
  }


  // attempt some power saving

  // Minimizes power when bluetooth is used
  NRF_POWER->DCDCEN = 1;

  gotTime = false;
/*  

  //Disabling UART0 (saves around 300-500µA) - @Jul10199555 contribution
  NRF_UART0->TASKS_STOPTX = 1;
  NRF_UART0->TASKS_STOPRX = 1;
  NRF_UART0->ENABLE = 0;
  
  *(volatile uint32_t *)0x40002FFC = 0;
  *(volatile uint32_t *)0x40002FFC;
  *(volatile uint32_t *)0x40002FFC = 1; //Setting up UART registers again due to a library issue
*/
}

void loop() {

//  if(bleSerial && gotTime==true){
//    delay(500);
//  }

/*
//  if(bleSerial && gotTime==false){
  if(_BSelect[NEW]){
    //t:"status", bat:0..100, volt:float(voltage), chg:0/1 - status update
      bleSerial.println("{t:\status, bat:75, volt:3.70, chg:1}.");
  }
*/
//////////////////////////////////////////////////////////////////////////////////////////////////
/*
//  if(bleSerial){
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
//  }
*/
//////////////////////////////////////////////////////////////////////////////////////////////////

  // this must be called regularly to perform BLE updates
   getMessages();

   
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
      update();
      //chargingScreen();
    }

//  }else{
//    no_connection_screen();
//  }
  //bleSerial.end();

}










void update(){

  updateButtons();

  BAT_CHRG = digitalRead(CHARGE_LED);

  memset(sBuff, 0x00, WIDTH*HEIGHT/8);
  
  gCol = WHITE;
  gFont = 1;
  char tempText[18];
  //sprintf(tempText,"%d FPS", fpsCount);
  float vBat = readBattery();
  //int batPercent = (vBat / 3.7)*100;
  sprintf(tempText,"%.2fv", vBat);
  gScale = 2;
  print(0,12,tempText);

  fillRect(0,35, 144,10, GREY); // will be battery percent later

  convertUnixTime((millis()/1000)+startMillis);
  
  char buf[21];
  sprintf(buf,"%02d:%02d:%02d", myTime[HOUR], myTime[MINUTE], myTime[SECOND]);
  //sprintf(buf,"%02d:%02d", myTime[HOUR], myTime[MINUTE]);
  gScale = 4;
  int tw = 0;//(144 - stringLength(buf)) / 2;
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

  if(_BSelect[NEW]){
    framecount = 0;
    //cardY = 144;
  }
  if(_BSelect[HELD]){
    digitalWrite(LED_GREEN, LOW);
    displayCard(168-bounce[framecount]);
    if(framecount<100)framecount+=2;
  }

  if(_BDown[NEW]){
    digitalWrite(LED_BLUE, LOW);
  }

  if(notificationTimeout >0){
    if(framecount<100){
      displayCard(168-bounce[framecount]);
      framecount+=2;
    }else{
      displayCard(0);
      notificationTimeout--;
    }
  }


  writeScreen();
  //framecount++;

/*
  if(millis() >= lastMillis+1000){
    lastMillis = millis();
    fpsCount = framecount;
    framecount = 0;
  }
*/
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

void displayCard(int y){
  gScale=1;

  fillRect(0, y, 144, 1, BLACK);
  // banner thingy for title
  fillRect(0, y+1, 144, 40, GREY);
  fillRect(0, y+40, 144, 168-40, WHITE);

  gCol = WHITE;
  drawSprite(message_icon_masked[1], 55, y+4);
  gCol = BLACK;
  drawSprite(message_icon_masked[0], 55, y+4);

  gCol = BLACK; 
  gScale = 2;
  gFont = 3; // bold?
  print(0, y+40, notification[numNotifications].src );

  gScale = 1;
  gFont = 2; // regular
  print(0, y+40+ (font03[0][1]*2), notification[numNotifications].title );

  gScale = 1;
  print(0, y+90, notification[numNotifications].body );
  
  // Title
  //print(1, 1, "Alert!");
  writeScreen();
}
