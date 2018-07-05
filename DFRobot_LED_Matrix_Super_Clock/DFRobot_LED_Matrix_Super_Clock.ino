/*
 * Super Clock - Firebeetle ESP32 and LED Matrix Cover.
 * Many options and modes can be set via a webpage.
 * 
 * v1.0.0 - init release
 * v1.0.1 - added pushingbox for Alarm/Timer notification
 * v1.0.2 - added Binary mode
 * 
Copyright (c) 2018 LeRoy Miller

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses>

If you find this or any of my projects useful or enjoyable please support me.  
Anything I do get goes to buy more parts and make more/better projects.  
https://www.patreon.com/kd8bxp  
https://ko-fi.com/lfmiller  

https://github.com/kd8bxp
https://www.youtube.com/channel/UCP6Vh4hfyJF288MTaRAF36w  
https://kd8bxp.blogspot.com/  
*/

#include <TimeLib.h>

#if defined(ESP8266)
#include <ESP8266WiFi.h>          //https://github.com/esp8266/Arduino
#else
#include <WiFi.h>          
#endif

//needed for library
#include <DNSServer.h>  //https://github.com/bbx10/DNSServer_tng
#if defined(ESP8266)
#include <ESP8266WebServer.h>
#else
#include <WebServer.h> //https://github.com/bbx10/WebServer_tng
#endif
#include <WiFiManager.h>         //https://github.com/bbx10/WiFiManager/tree/esp32

#include <NTPClient.h>  //https://github.com/arduino-libraries/NTPClient
#include "DFRobot_HT1632C.h"
#include <WiFi.h>
#include <WiFiUdp.h>
#include <HTTPClient.h>

#if defined( ESP_PLATFORM ) || defined( ARDUINO_ARCH_FIREBEETLE8266 )  //FireBeetle-ESP32 FireBeetle-ESP8266
#define DATA D6
#define CS D2
#define WR D7
//#define RD D8
#else
#define DATA 6
#define CS 2
#define WR 7
//#define RD 8
#endif

DFRobot_HT1632C display = DFRobot_HT1632C(DATA, WR,CS);

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org");
WiFiServer server(80);
WiFiClient notifyClient;

#define TIMEOFFSET -14400 //Find your Time Zone off set Here https://www.epochconverter.com/timezones OFF Set in Seconds
bool AMPM = 1; //1 = AM PM time, 0 = MILITARY/24 HR Time
bool DISPLAYZERO = 0; //display leading zero in zero 0 = No, 1 = Yes


char temp1[30];
int hours,minutes, seconds;
int slideTime = 5; //fast
int updateDelay = 5; //about 10 seconds
static int taskCore = 0;
int tempDigit1, tempDigit2, tempDigit3, tempDigit4;
uint8_t *DIGIT0,*DIGIT1,*DIGIT2, *DIGIT3, *DIGIT0A, *DIGIT1A, *DIGIT2A, *DIGIT3A;
int dim = 8;
int scrollDelay; //for original scrolling this is max of 50 and min of 30 it's based on a slideTime of 5 (5 * 6 = 30 default) but can be changed slightly.

/* Modes:
 *  0 - Scroll clock (original example)
 *  1 - Clock fall from right to left, stops, and falls off screen
 *  2 - Left to Right slow scroll...falling numbers....
 *  3 - sliding up from the bottom of the screen MODE 3 overrides SETREMOVE (Mode 3 always SETREMOVE to ZERO)
 *      slideTime also can't be set with mode 3.
 *  4 - Scroll from Center,    
 *  5 - Scroll Epoch Time - offset adjusted I believe.
 *  6 - The Easiest Star Date Conversion - based on probably the original 6 movies
 *      http://trekguide.com/Stardates.htm   YYMM.DD  YY current year minus 1900, two digit month, and two digit day. Doesn't take into account time.
 *  7 - Binary Clock Mode - Upper line, 8 4 2 1, lower line 32 16 8 4 2 1    
 */

int mode = 1; 
int SETREMOVE = 1; //animated scrolling remove
bool DISPLAYDATE = 0; //1= yes, 0 = no
bool DISPLAYDAY = 0; //1 = yes, 0 = no

// Client variables 
char linebuf[80];
int charcount=0;

//Alarm Variables 2 alarms, and 2 minute timers.
int alarm1Hour = 12;
int alarm1Minute = 00;
int alarm2Hour = 12;
int alarm2Minute = 00;
bool alarm1Set = 0; //Off
bool alarm2Set = 0; //Off
int timer1 = 0; //minute timer
int timer2 = 0; //minute timer
int timer1Set = 0; //Off
int timer2Set = 0; //Off
int timer1Notified = 0;
int timer2Notified = 0;
int alarm1Notified = 0;
int alarm2Notified = 0;

//Pushingbox Setup
const char* notifyServer = "api.pushingbox.com";
String timer1API = "xxxxxx";
String timer2API = "xxxxxxx";
String alarm1API = "xxxxx"; //alarm 1 and 2 still are not working yet, this is to setup when I get them working.
String alarm2API = "xxxxxx";


uint8_t COLONON[] = {
  B00000000,
  B00010100,
  B00000000,
  B00000000
}; //:

uint8_t COLONOFF[] = {
  B00000000,
  B00000000,
  B00000000,
  B00000000
};

//zero
 uint8_t zerobyte0[] = {B01111110,B00000000,B00000000,B00000000};
 uint8_t zerobyte1[] = {B10110001,B00000000,B00000000,B00000000};
 uint8_t zerobyte2[] = {B10001101,B00000000,B00000000,B00000000};
 uint8_t zerobyte3[] = {B01111110,B00000000,B00000000,B00000000};

//one
 uint8_t onebyte0[] = {B01000001,B00000000,B00000000,B00000000};
 uint8_t onebyte1[] = {B11111111,B00000000,B00000000,B00000000};
 uint8_t onebyte2[] = {B00000001,B00000000,B00000000,B00000000};
 uint8_t onebyte3[] = {B00000000,B00000000,B00000000,B00000000};

//two
 uint8_t twobyte0[] = {B01000011,B00000000,B00000000,B00000000};
 uint8_t twobyte1[] = {B10000101,B00000000,B00000000,B00000000};
 uint8_t twobyte2[] = {B10001001,B00000000,B00000000,B00000000};
 uint8_t twobyte3[] = {B01110001,B00000000,B00000000,B00000000};

//three
 uint8_t threebyte0[] = {B01000010,B00000000,B00000000,B00000000}; 
 uint8_t threebyte1[] = {B10001001,B00000000,B00000000,B00000000};
 uint8_t threebyte2[] = {B10001001,B00000000,B00000000,B00000000};
 uint8_t threebyte3[] = {B01110110,B00000000,B00000000,B00000000};

 //four
uint8_t fourbyte0[] = {B00011100,B00000000,B00000000,B00000000};
uint8_t fourbyte1[] = {B00100100,B00000000,B00000000,B00000000};
uint8_t fourbyte2[] = {B01001111,B00000000,B00000000,B00000000};
uint8_t fourbyte3[] = {B10000100,B00000000,B00000000,B00000000};

 //five
uint8_t fivebyte0[] = {B11110001,B00000000,B00000000,B00000000};
uint8_t fivebyte1[] = {B10010001,B00000000,B00000000,B00000000};
uint8_t fivebyte2[] = {B10010001,B00000000,B00000000,B00000000};
uint8_t fivebyte3[] = {B10001110,B00000000,B00000000,B00000000};

 //six
 uint8_t sixbyte0[] = {B01111110,B00000000,B00000000,B00000000};
 uint8_t sixbyte1[] = {B10001001,B00000000,B00000000,B00000000};
 uint8_t sixbyte2[] = {B10001001,B00000000,B00000000,B00000000};
 uint8_t sixbyte3[] = {B01000110,B00000000,B00000000,B00000000};

//seven
uint8_t sevenbyte0[] = {B10000000,B00000000,B00000000,B00000000};
uint8_t sevenbyte1[] = {B10000111,B00000000,B00000000,B00000000};
uint8_t sevenbyte2[] = {B10011000,B00000000,B00000000,B00000000};
uint8_t sevenbyte3[] = {B11100000,B00000000,B00000000,B00000000};

//eight
uint8_t eightbyte0[] = {B01110110,B00000000,B00000000,B00000000};
uint8_t eightbyte1[] = {B10001001,B00000000,B00000000,B00000000};
uint8_t eightbyte2[] = {B10001001,B00000000,B00000000,B00000000};
uint8_t eightbyte3[] = {B01110110,B00000000,B00000000,B00000000};

//nine
uint8_t ninebyte0[] = {B01110010,B00000000,B00000000,B00000000};
uint8_t ninebyte1[] = {B10001001,B00000000,B00000000,B00000000};
uint8_t ninebyte2[] = {B10001001,B00000000,B00000000,B00000000};
uint8_t ninebyte3[] = {B01111110,B00000000,B00000000,B00000000};

uint8_t blank[] = {B00000000,B00000000,B00000000,B00000000};

uint8_t blankBinary[] = {
 B11100000,
  B10100000,
  B11100000,
  B00000000,
  B00000000,
  B00000000,
  B00000000,
  B00000000
}; 


void coreTask( void * pvParameters ){

 while(true) {
    webConfig();
   yield();
 }
}


void setup() {
  Serial.begin(9600);
  xTaskCreatePinnedToCore(
                    coreTask,   /* Function to implement the task */
                    "coreTask", /* Name of the task */
                    10000,      /* Stack size in words */
                    NULL,       /* Task input parameter */
                    0,          /* Priority of the task */
                    NULL,       /* Task handle. */
                    taskCore);  /* Core where the task should run */
  // put your setup code here, to run once:
  display.begin();
  display.isLedOn(true);
  display.clearScreen();
  display.setPwm(dim);
  display.setFont(FONT8X4);
  WiFiManager wifiManager;
  wifiManager.autoConnect("AutoConnectAP");
  Serial.println("connected...yeey :)");
  display.print("Connected.....",30);
  timeClient.begin();
  timeClient.setTimeOffset(TIMEOFFSET);
  server.begin();
  
}

void loop() {
  
 timeClient.update(); 
 hours = timeClient.getHours();
 minutes = timeClient.getMinutes();
 seconds = timeClient.getSeconds();
 setTime(timeClient.getEpochTime());
 display.clearScreen();
 checkAlarm();
switch (mode) {
  case 0:
  scroll();
  break;
  case 1:
  falling();
  break;
  case 2:
  falling();
  break;
  case 3:
  falling();
  break;
  case 4:
  center();
  break;
  case 5:
  epoch();
  break;
  case 6:
  starDate();
  break;
  case 7:
  binary();
  delay(100);
  yield();
  
  display.clearScreen();
  display.writeScreen();
  
  break;
  default:
  falling();
  break;
}

checkAlarm();

scrollDelay = slideTime * 6;
if (scrollDelay > 50) {scrollDelay = 50;}
if (scrollDelay < 20) {scrollDelay = 20;}



if (SETREMOVE == 1) {
  if (mode == 0) { } //do nothing
  if (mode == 1 || mode == 2) {setRemove();}
  if (mode == 3) { } //do nothing
  if (mode == 4) {setRemove();}
  if (mode == 5 || mode == 6) { } //do nothing
  if (mode == 7) { } //do nothing
 /* if (mode == 4) {
  displayCenterCase(tempDigit1,tempDigit4, 2);
  displayCenterCase(tempDigit2,tempDigit3, 1);
   } else {setRemove();}*/
   //setRemove();
  }

if (DISPLAYDATE) {
  sprintf(temp1," %d/%d/%d ",month(),day(),year());
display.print(temp1, scrollDelay);
delay(200);
}

if (DISPLAYDAY) {
  String mon = (String)dayStr(timeClient.getDay()+1) + ' ' + (String)monthStr(month()) + ' ' + day();
  char temps[51];
mon.toCharArray(temps,50);
display.print(temps,scrollDelay);
delay(200);
}
  
}

void checkAlarm() {
  if (timer1Set == 1 && timer1 == minutes) {
  display.isBlinkEnable(true); 
  sms(timer1API,timer1Notified);
  timer1Notified = 1;} //alarm on
  if (timer2Set == 1 && timer2 == minutes) {
  display.isBlinkEnable(true);
  sms(timer2API,timer2Notified);
  timer2Notified = 1;} //alarm on
  //display.isBlinkEnable(false); //alarm off
}

void starDate() {
  int y1 = year() - 1900;
  int d1 = day();
  int m1 = month();
  sprintf(temp1,"Stardate: %d%02d.%02d", y1,m1,d1);
  display.print(temp1,scrollDelay);
  delay(150);
}

void epoch() {
  sprintf(temp1,"%d",timeClient.getEpochTime()); //Has offset applied.
  display.print(temp1,scrollDelay);
  delay(150);
}

void scroll() {
  
if (AMPM) {
  if (hours >= 13) {
      hours = hours -12;
      sprintf(temp1,"%d:%02d:%02d pm",hours,minutes,seconds);
  } else { 
      if (hours == 0) {
      sprintf(temp1,"12:%02d:%02d am",minutes,seconds);
    } else {
      sprintf(temp1,"%d:%02d:%02d am",hours,minutes,seconds);
  }
 }
} else {
    sprintf(temp1,"%02d:%02d:%02d",hours,minutes,seconds);
}
display.print(temp1,scrollDelay);
delay(150);
}

void falling() {
if (AMPM) {
  if (hours >= 13) { hours = hours - 12; }
 
  if (hours == 0) {
    hours = 12;
  }
}
 if (hours < 10) {
  if (DISPLAYZERO) {
  displayCase(0,1);
  //tempDigit1 = 0; 
  }
  if (AMPM == 0 && hours == 0) {
    displayCase(0,1);
  }
  displayCase(hours,2);
  tempDigit2 = hours;
 } else {
  int temp = hours/10;
  displayCase(temp,1);
  tempDigit1 = temp;
  temp = hours - (hours/10)*10;
  displayCase(temp,2);
  tempDigit2 = temp;
 }

 if (minutes < 10) {
  displayCase(0,3);
  displayCase(minutes,4);
  tempDigit3 = 0;
  tempDigit4 = minutes;
 } else {
  int temp = minutes/10;
  displayCase(temp,3);
  tempDigit3 = temp;
  temp = minutes - (minutes/10)*10;
  //Serial.println(temp);
  displayCase(temp,4);
  tempDigit4 = temp;
 }
 colon();
 
}

void center() {
  if (AMPM) {
  if (hours >= 13) { hours = hours - 12; }
 
  if (hours == 0) {
    hours = 12;
  }
}

 if (hours < 10) {
  if ((DISPLAYZERO == 1) || (AMPM == 0 && hours == 0)) {
    tempDigit1 = 0; 
  } else {tempDigit1 = -1;}
    tempDigit2 = hours;
 } else {
  int temp = hours/10;
  tempDigit1 = temp;
  temp = hours - (hours/10)*10;
  tempDigit2 = temp;
 }

 if (minutes < 10) {
  tempDigit3 = 0;
  tempDigit4 = minutes;
 } else {
  int temp = minutes/10;
  tempDigit3 = temp;
  temp = minutes - (minutes/10)*10;
  tempDigit4 = temp;
 }

 displayCenterCase(tempDigit2,tempDigit3, 1);
 displayCenterCase(tempDigit1,tempDigit4, 2);
 colon();
}

 void colon() {
for (int i=0; i<updateDelay; i++) {
  display.drawImage(COLONOFF,3,8,10,0,0);
  display.writeScreen();
  delay(1000);
  display.drawImage(COLONON,3,8,10,0,0);
  display.writeScreen();
  delay(1000);
  }
}

void setRemove() {
if (DISPLAYZERO) { removeCase(0,1); } else {
  if (tempDigit1>0) { removeCase(tempDigit1,1); }
}
removeCase(tempDigit2,2);
removeCase(tempDigit3,3);
removeCase(tempDigit4,4);
delay(250);
  
}

void updateDisplay(uint8_t *DIGIT, int number, int loc) {
  int start, offset;
  if (loc == 1) {start = 0 + number; offset = 0;} //3
  if (loc == 2) {start = 5 + number; offset = 5;} //8
  if (loc == 3) {start = 10 + number; offset = 12;} //13
  if (loc == 4) {start = 15 + number; offset = 17;} //17
  if (mode == 1) {
  for (int i=start; i<25-number; i++) {
  display.drawImage(DIGIT,4,8,25+offset-i,0,0);
  display.writeScreen();
  delay(slideTime);
  } }else if (mode == 2 || mode == 3) {
     if (loc == 3) {start = 13 + number;}
     if (loc == 4) {start = 18 + number;}
    for (int i=0; i<9; i++) {
      if (mode == 2) {display.drawImage(DIGIT,4,8,start,i-8,0);}
      if (mode == 3) {display.drawImage(DIGIT,4,8,start,8-i,0);}
       display.writeScreen();
 if (mode == 2) {delay(slideTime+5);}
 if (mode == 3) {delay(30);} //slow to see the effect
    }
  } 
}

void displayCase(int number, int digit) {
  switch (number) {
    case 0:
    updateDisplay(zerobyte0,0,digit);
    updateDisplay(zerobyte1,1,digit);
    updateDisplay(zerobyte2,2,digit);
    updateDisplay(zerobyte3,3,digit);
    break;
    case 1:
    updateDisplay(onebyte0,0,digit);
    updateDisplay(onebyte1,1,digit);
    updateDisplay(onebyte2,2,digit);
    updateDisplay(onebyte3,3,digit);
    break;
    case 2:
    updateDisplay(twobyte0,0,digit);
    updateDisplay(twobyte1,1,digit);
    updateDisplay(twobyte2,2,digit);
    updateDisplay(twobyte3,3,digit);
    break;
    case 3:
    updateDisplay(threebyte0,0,digit);
    updateDisplay(threebyte1,1,digit);
    updateDisplay(threebyte2,2,digit);
    updateDisplay(threebyte3,3,digit);
    break;
    case 4:
    updateDisplay(fourbyte0,0,digit);
    updateDisplay(fourbyte1,1,digit);
    updateDisplay(fourbyte2,2,digit);
    updateDisplay(fourbyte3,3,digit);
    break;
    case 5:
    updateDisplay(fivebyte0,0,digit);
    updateDisplay(fivebyte1,1,digit);
    updateDisplay(fivebyte2,2,digit);
    updateDisplay(fivebyte3,3,digit);
    break;
    case 6:
    updateDisplay(sixbyte0,0,digit);
    updateDisplay(sixbyte1,1,digit);
    updateDisplay(sixbyte2,2,digit);
    updateDisplay(sixbyte3,3,digit);
    break;
    case 7:
    updateDisplay(sevenbyte0,0,digit);
    updateDisplay(sevenbyte1,1,digit);
    updateDisplay(sevenbyte2,2,digit);
    updateDisplay(sevenbyte3,3,digit);
    break;
    case 8:
    updateDisplay(eightbyte0,0,digit);
    updateDisplay(eightbyte1,1,digit);
    updateDisplay(eightbyte2,2,digit);
    updateDisplay(eightbyte3,3,digit);
    break;
    case 9:
    updateDisplay(ninebyte0,0,digit);
    updateDisplay(ninebyte1,1,digit);
    updateDisplay(ninebyte2,2,digit);
    updateDisplay(ninebyte3,3,digit);
    break;
    default:
    break;
  }
  
}

void webConfig() {
  // listen for incoming clients
  WiFiClient client = server.available();
  if (client) {
    Serial.println("New client");
    memset(linebuf,0,sizeof(linebuf));
    charcount=0;
    // an http request ends with a blank line
    boolean currentLineIsBlank = true;
    while (client.connected()) {
      if (client.available()) {
        char c = client.read();
        Serial.write(c);
        //read char by char HTTP request
        linebuf[charcount]=c;
        if (charcount<sizeof(linebuf)-1) charcount++;
        // if you've gotten to the end of the line (received a newline
        // character) and the line is blank, the http request has ended,
        // so you can send a reply
        if (c == '\n' && currentLineIsBlank) {
          // send a standard http response header
          client.println("HTTP/1.1 200 OK");
          client.println("Content-Type: text/html");
          client.println("Connection: close");  // the connection will be closed after completion of the response
          client.println();
          client.println("<!DOCTYPE HTML><html><head>");
          client.println("<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\"></head>");
          //client.println("<meta http-equiv=\"refresh\" content=\"1\">");
          client.println("<h1><center>Clock Settings</center></h1><center>By LeRoy Miller, (C) 2018</center>");
          
          //General Infromation
          client.print("<br><h4>General Information:</h4>");
          client.print("<p>Display 24 Hour Time: ");
          client.print(AMPM ? "No" : "Yes");
          client.print("&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;Display Leading Zero: ");
          client.print(DISPLAYZERO ? "Yes" : "No");
          client.print("<br>Display Date: "); client.print(DISPLAYDATE ? "Yes" : "No");
          client.print("&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;Display Day: "); client.print(DISPLAYDAY ? "Yes" : "No");
          client.print("<br><br>Slide Timer: "); client.print(slideTime);client.print("<br> Between 5 and 30 for most Animations, Default: 5<br>low numbers will be fast, high numbers will be slower.");
          //client.print("<br><br>Scroll Delay<br>(Original Scroll only): "); client.print(scrollDelay); client.print("<br>for original scrolling this is min of 30 and max of 50. Default of 30.<br>Slide Time is related, and controls this.");
          client.print("<br><br>Update Display Timer: "); client.print(updateDelay * 2);client.print(" seconds.<br>Between 10 and 30 seconds, Default: 10 seconds.<br>This is how often the screen clears.");
          client.print("<br><br>Dimmer: ");client.print(dim);client.print("<br>Dim the screen, default setting 8.");

          //Display settings
          client.println("<br><br><h4>Display:</h4> ");
          client.println("<br>24 Hour Time: <a href=\"hour1\"><button>YES</button></a>&nbsp;<a href=\"hour0\"><button>NO</button></a>");
          client.println("&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;Display Leading Zero: <a href=\"zero1\"><button>YES</button></a>&nbsp;<a href=\"zero0\"><button>NO</button></a>");
          client.print("<br><br>Dim: <a href=\"dim-\"><button>[-]Minus</button></a>&nbsp;<a href=\"dim+\"><button>[+]Plus</button></a>");
          client.print("&nbsp;<a href=\"dim5\"><button>5</button></a>&nbsp;<a href=\"dim8\"><button>default 8</button></a>&nbsp;<a href=\"dim12\"><button>12</button></a>");
          client.print("&nbsp;<a href=\"dimmin\"><button>Min Bright</button></a>&nbsp;<a href=\"dimmax\"><button>Max Bright</button></a>&nbsp;");
          client.print("<br><br>Speed of Slide Timer: <a href=\"slide-\"><button>[-]Minus</button></a>&nbsp;<a href=\"slide+\"><button>[+]Plus</button></a>");
          client.print("&nbsp;<a href=\"slide5\"><button>Default 5 Fastest</button></a>&nbsp;<a href=\"slide10\"><button>[10]Fast</button></a>&nbsp;");
          client.print("<a href=\"slide15\"><button>[15]Slower</button></a>&nbsp;<a href=\"slide30\"><button>[30]Slowest</button></a>");
          client.print("<br><br>Display Timer: <a href=\"timer-\"><button>[-]Minus</button></a>&nbsp;<a href=\"timer+\"><button>[+]Plus</button></a>");
          client.print("&nbsp;<a href=\"timer10\"><button>[10 Seconds]Default</button></a>&nbsp;<a href=\"timer20\"><button>[20 Seconds]</button></a>&nbsp;<a href=\"timer30\"><button>[30 Seconds]</button></a>");        
          client.print("<br><br>Scroll Date: <a href=\"date1\"><button>YES</button></a>&nbsp;<a href=\"date0\"><button>NO</button></a>");
          client.print("&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;Scroll Day: <a href=\"day1\"><button>YES</button></a>&nbsp;<a href=\"day0\"><button>NO</button></a>");

          //Change Modes
          client.print("<br><br><h4>Modes:</h4> ");
          client.print("<br><a href=\"0\"><button>Original Scroll</button></a>&nbsp;<a href=\"1\"><button>Fall from Right[Default]</button></a>");
          client.print("&nbsp;<a href=\"2\"><button>Left to Right</button></a>&nbsp;<a href=\"3\"><button>Slide Up</button></a>");
          client.print("&nbsp;<a href=\"4\"><button>Center Scroll</button></a>&nbsp;<a href=\"5\"><button>Epoch Scroll</button></a>");
          
          //Alarm and Timer Settings
          client.print("<br><br><h4>Alarms and Timers</h4><br>");
          client.print("Please use 24 hour format for your hours,<br>13 to 23 is PM, 13 is 1pm, and 23 = 11pm, and Zero is Midnight.<br>");
          client.print("<br>Alarm 1 Set: "); client.print(alarm1Set ? "Yes" : "No"); 
          sprintf(temp1,"&nbsp;&nbsp;&nbsp;%02d:%02d",alarm1Hour,alarm1Minute);
          client.print(temp1);
          client.print("&nbsp;&nbsp;<form action=\"/\">Alarm 1 Hour:&nbsp;&nbsp;&nbsp; <input type=\"number\" min=\"0\" max=\"23\" name=\"hour1\">");
          client.print("&nbsp;&nbsp;&nbsp;Minute:&nbsp; <input type=\"number\" min=\"0\" max=\"59\" name=\"minute1\">&nbsp;<input type=\"submit\" value=\"Submit\"></form>");
          client.print("<br>Alarm 2 Set: "); client.print(alarm2Set ? "Yes" : "No");
          sprintf(temp1,"&nbsp;&nbsp;&nbsp;%02d:%02d",alarm2Hour,alarm2Minute);
          client.print(temp1);
          client.print("&nbsp;&nbsp;<form action=\"/\">Alarm 2 Hour:&nbsp;&nbsp;&nbsp; <input type=\"number\" min=\"0\" max=\"23\" name=\"hour2\">");
          client.print("&nbsp;&nbsp;&nbsp;Minute:&nbsp; <input type=\"number\" min=\"0\" max=\"59\" name=\"minute2\">&nbsp;<input type=\"submit\" value=\"Submit\"></form>");
          client.print("<br><a href=\"resetA1\"><button>Alarm 1 OFF</button></a>&nbsp;&nbsp;<a href=\"resetA2\"><button>Alarm 2 OFF</button></a>");
          client.print("<br><br>Timer 1 Set: ");client.print(timer1Set ? "Yes" : "No");client.print("&nbsp;&nbsp;&nbsp;");client.print(timer1);
          client.print("<br>Minute Timer 1: <a href=\"t1-5\"><button>[5 Minutes]</button></a>&nbsp;&nbsp;<a href=\"t1-10\"><button>[10 Minutes]</button></a>&nbsp;&nbsp;");
          client.print("<a href=\"t1-15\"><button>[15 Minutes]</button></a>&nbsp;&nbsp;<a href=\"t1-30\"><button>[30 Minutes]</button></a>&nbsp;&nbsp;");
          client.print("<a href=\"t1-45\"><button>[45 Minutes]</button></a>&nbsp;&nbsp;<a href=\"resett1\"><button>[Timer 1 OFF]</button></a>");
          client.print("<br><br>Timer 2 Set: ");client.print(timer2Set ? "Yes" : "No");client.print("&nbsp;&nbsp;&nbsp;");client.print(timer2);
          client.print("<br>Minute Timer 2: <a href=\"t2-5\"><button>[5 Minutes]</button></a>&nbsp;&nbsp;<a href=\"t2-10\"><button>[10 Minutes]</button></a>&nbsp;&nbsp;");
          client.print("<a href=\"t2-15\"><button>[15 Minutes]</button></a>&nbsp;&nbsp;<a href=\"t2-30\"><button>[30 Minutes]</button></a>&nbsp;&nbsp;");
          client.print("<a href=\"t2-45\"><button>[45 Minutes]</button></a>&nbsp;&nbsp;<a href=\"resett2\"><button>[Timer 2 OFF]</button></a>");          

          //Extra Setting for Original Scroll

          //Advanced Display Settings.
          client.print("<br><br><h4>Experimental/Advanced Settings:</h4> ");
          client.print("<br>These may or may not break at any time.<br>");
          client.print("Animated Clear Screen: "); client.print(SETREMOVE ? "Yes" : "No"); client.print("<br>Default yes animate clear screen.");
          client.print("<br>Animated Clear Screen: <a href=\"clear1\"><button>YES</button></a>&nbsp;<a href=\"clear0\"><button>NO</button></a>");
          client.print("<br>Most animations look better with this turned on.<br>");
          client.print("<br>Just for Fun: <a href=\"6\"><button>[StarDate]</button></a>&nbsp;<br>Based on a YYMM.DD Stardate that probably corresponds to the frist 6 Star Trek Movies & TOS.<br><a href=\"http://trekguide.com/Stardates.htm\">Trekguide.com</a>");
          client.print("<br><br>Binary Clock Mode is very EXPERIMENTAL at this stage. This WILL reset/restart your FireBeetle ESP32 if you change to another mode, all settings and timers/alarms will be lost.<br>");
          client.print("<a href=\"7\"><button>Binary Clock</button></a>");
          client.print("<br><br>Set Another Time Zone: "); client.print("<br>Default: "); client.print(TIMEOFFSET);
          client.print("&nbsp;<br><br><a href=\"default\"><button>[Default Zone]</button></a>&nbsp;<a href=\"UTC\"><button>[UTC]</button></a>");
          client.print("&nbsp;<a href=\"aest\"><button>Sydney, AU[AEST]</button></a>&nbsp;<a href=\"hst\"><button>Hawaii, US[HST]</button></a>");
          client.print("&nbsp;<br><br><a href=\"pdt\"><button>Los Angeles, US[PDT]</button></a>&nbsp;<a href=\"mdt\"><button>Denver, US[MDT]</button></a>");
          client.print("&nbsp;<a href=\"cdt\"><button>Chicago, US[CDT]</button></a>&nbsp;<a href=\"edt\"><button>New York, US[EDT]</button></a>");
          client.print("&nbsp;<br><br><a href=\"bst\"><button>London [BST]</button></a>&nbsp;<a href=\"cest\"><button>Paris [CEST]</button></a>");
          client.print("&nbsp;<a href=\"turkey\"><button>Turkey [+3]</button></a>&nbsp;<a href=\"msk\"><button>Moscow [MSK]</button></a>");
          client.print("&nbsp;<br><br><a href=\"hkt\"><button>Hong Kong [HKT]</button></a>&nbsp;<a href=\"jst\"><button>Tokyo [JST]</button></a>");
          client.print("<br>Anyone know the offset for Turkey?");
          //end site
          client.println("</html>");
          break;
        }
        if (c == '\n') {
          // you're starting a new line
          currentLineIsBlank = true;
          if (strstr(linebuf,"GET /hour1") > 0){
            //Serial.println("LED 1 ON");
            AMPM = 0;
                      }
          else if (strstr(linebuf,"GET /hour0") > 0){
            //Serial.println("LED 1 OFF");
            AMPM = 1;
                      }

          if (strstr(linebuf,"GET /date1") > 0){
            //Serial.println("LED 1 ON");
            DISPLAYDATE = 1;
                      }
          else if (strstr(linebuf,"GET /date0") > 0){
            //Serial.println("LED 1 OFF");
            DISPLAYDATE = 0;
                      }  

           if (strstr(linebuf,"GET /day1") > 0){
            //Serial.println("LED 1 ON");
            DISPLAYDAY = 1;
                      }
          else if (strstr(linebuf,"GET /day0") > 0){
            //Serial.println("LED 1 OFF");
            DISPLAYDAY = 0;
                      }            
                                           
          else if (strstr(linebuf,"GET /zero1") > 0){
            //Serial.println("LED 2 ON");
            DISPLAYZERO = 1;
                      }
          else if (strstr(linebuf,"GET /zero0") > 0){
            DISPLAYZERO = 0;
                      }
          else if (strstr(linebuf,"GET /dim-") > 0){
            dim = dim - 1;
            if (dim < 1) {dim = 1;}
            display.setPwm(dim);
                      }
          else if (strstr(linebuf,"GET /dim+") > 0) {
            dim = dim + 1;
            if (dim > 15) {dim = 15;}
            display.setPwm(dim);
                      }
           else if (strstr(linebuf,"GET /dimmin") > 0) {
            dim = 1;
            display.setPwm(dim);
                      }
           else if (strstr(linebuf,"GET /dimmax") > 0) {
            dim = 15;
            display.setPwm(dim);
                      }             
            else if (strstr(linebuf,"GET /dim5") > 0) {
            dim = 5;
            display.setPwm(dim);
                      }
             else if (strstr(linebuf,"GET /dim8") > 0) {
            dim = 8;
            display.setPwm(dim);
                      }
           else if (strstr(linebuf,"GET /dim12") > 0) {
            dim = 12;
            display.setPwm(dim);
                      }                                                                                             
          else if (strstr(linebuf,"GET /slide-") > 0) {
            slideTime = slideTime - 1;
            if (slideTime < 5) {slideTime = 5;}            
          }
          else if (strstr(linebuf,"GET /slide+") > 0) {
            slideTime = slideTime + 1;
            if (slideTime > 30) {slideTime = 30;}
          }
else if (strstr(linebuf,"GET /slide5") > 0) {
            slideTime = 5;
                      } 
          else if (strstr(linebuf,"GET /slide10") > 0) {
            slideTime = 10;
                      }
          else if (strstr(linebuf,"GET /slide15") > 0) {
            slideTime = 15;
                      }  
          else if (strstr(linebuf,"GET /slide30") > 0) {
            slideTime = 30;
                     }   
else if (strstr(linebuf,"GET /timer-") > 0) {
            updateDelay = updateDelay - 1;
            if (updateDelay < 5) {updateDelay = 5;}            
          }
          else if (strstr(linebuf,"GET /timer+") > 0) {
            updateDelay = updateDelay + 1;
            if (updateDelay > 15) {updateDelay = 15;}
          }
else if (strstr(linebuf,"GET /timer10") > 0) {
            updateDelay = 5;
                      }
          else if (strstr(linebuf,"GET /timer20") > 0) {
            updateDelay = 10;
                      }
          else if (strstr(linebuf,"GET /timer30") > 0) {
            updateDelay = 30;
                     }
                                                             
          else if (strstr(linebuf,"GET /0") > 0) {
          mode = 0; 
                  }
        else if (strstr(linebuf,"GET /1") >0) {
        mode = 1;
              }
      else if (strstr(linebuf,"GET /2") >0) {
        mode = 2;
              }
      else if (strstr(linebuf,"GET /3") >0) {
        mode = 3;
              }
      else if (strstr(linebuf,"GET /4") > 0) {
        mode = 4;
              }
       else if (strstr(linebuf,"GET /5") > 0) {
        mode = 5;
              }  
        else if (strstr(linebuf,"GET /6") > 0) {
        mode = 6;
              }   
      else if (strstr(linebuf,"GET /7") > 0) {
        mode = 7;                  
      }
if (strstr(linebuf,"GET /clear1") > 0){
            //Serial.println("LED 1 ON");
            SETREMOVE = 1;
                      }
          else if (strstr(linebuf,"GET /clear0") > 0){
            //Serial.println("LED 1 OFF");
            SETREMOVE = 0;
                      }     
          else if (strstr(linebuf,"GET /UTC") > 0) {
                  timeClient.setTimeOffset(0); 
                   }
          else if (strstr(linebuf,"GET /default") > 0){
                  timeClient.setTimeOffset(TIMEOFFSET);
                   }
          else if (strstr(linebuf,"GET /edt") > 0) {
            timeClient.setTimeOffset(-14400);    
          }
          else if (strstr(linebuf,"GET /aest") > 0) {
            timeClient.setTimeOffset(36000);    
          }
          else if (strstr(linebuf,"GET /hst") > 0) {
            timeClient.setTimeOffset(-36000);    
          }
          else if (strstr(linebuf,"GET /pdt") > 0) {
            timeClient.setTimeOffset(-25200);    
          }
          else if (strstr(linebuf,"GET /mdt") > 0) {
            timeClient.setTimeOffset(-21600);    
          }
          else if (strstr(linebuf,"GET /cdt") > 0) {
            timeClient.setTimeOffset(-18000);    
          }
          else if (strstr(linebuf,"GET /bst") > 0) {
            timeClient.setTimeOffset(3600);    
          }
          else if (strstr(linebuf,"GET /cest") > 0) {
            timeClient.setTimeOffset(7200);    
          }
          else if (strstr(linebuf,"GET /msk") > 0) {
            timeClient.setTimeOffset(10800);    
          }
          else if (strstr(linebuf,"GET /hkt") > 0) {
            timeClient.setTimeOffset(28800);    
          }
          else if (strstr(linebuf,"GET /jst") > 0) {
            timeClient.setTimeOffset(32400);    
          }
          else if (strstr(linebuf,"GET /t1-5") > 0) {
            timer1 = minutes + 5;
            checkTimerTime();
            timer1Set = 1;
          }
          else if (strstr(linebuf,"GET /t1-10") > 0) {
            timer1 = minutes + 10;
            checkTimerTime();
            timer1Set = 1;
          }
          else if (strstr(linebuf,"GET /t1-15") > 0) {
            timer1 = minutes + 15;
            checkTimerTime();
            timer1Set = 1;
          }
          else if (strstr(linebuf,"GET /t1-30") > 0) {
            timer1 = minutes + 30;
            checkTimerTime();
            timer1Set = 1;
          }
          else if (strstr(linebuf,"GET /t1-45") > 0) {
            timer1 = minutes + 45;
            checkTimerTime();
            timer1Set = 1;
          }
          else if (strstr(linebuf,"GET /resett1") > 0) {
            timer1Set = 0;
            display.isBlinkEnable(false); //alarm off
            timer1Notified = 0;
          }
          else if (strstr(linebuf,"GET /t2-5") > 0) {
            timer2 = minutes + 5;
            checkTimerTime();
            timer2Set = 1;
          }
          else if (strstr(linebuf,"GET /t2-10") > 0) {
            timer2 = minutes + 10;
            checkTimerTime();
            timer2Set = 1;
          }
          else if (strstr(linebuf,"GET /t2-15") > 0) {
            timer2 = minutes + 15;
            checkTimerTime();
            timer2Set = 1;
          }
          else if (strstr(linebuf,"GET /t2-30") > 0) {
            timer2 = minutes + 30;
            checkTimerTime();
            timer2Set = 1;
          }
          else if (strstr(linebuf,"GET /t2-45") > 0) {
            timer2 = minutes + 45;
            checkTimerTime();
            timer2Set = 1;
          }
          else if (strstr(linebuf,"GET /resett2") > 0) {
            timer2Set = 0;
            display.isBlinkEnable(false); //alarm off
            timer2Notified = 0;
          }
          // you're starting a new line
          currentLineIsBlank = true;
          memset(linebuf,0,sizeof(linebuf));
          charcount=0;
        } else if (c != '\r') {
          // you've gotten a character on the current line
          currentLineIsBlank = false;
        }
      }
    }
    // give the web browser time to receive the data
    delay(1);

    // close the connection:
    client.stop();
    Serial.println("client disconnected");
  }
  
}

void checkTimerTime() {
  if (timer1 >= 60) { timer1 = timer1 - 60;}
  if (timer2 >= 60) { timer2 = timer2 - 60;}
}

void updateRemoveDisplay(uint8_t *DIGIT, int number, int loc) {
  int start, offset;
  if (loc == 1) {start = 0 + number; offset = 0;} //3
  if (loc == 2) {start = 5 + number; offset = 5;} //8
  if (loc == 3) {start = 10 + number; offset = 12;} //13
  if (loc == 4) {start = 15 + number; offset = 17;} //17
  for (int i=start; i>-1; i--) {
  display.drawImage(DIGIT,4,8,0+i,0,0);
  if (i == 0) { display.clrLine(0,0,0,7); }
  display.writeScreen();
delay(slideTime);
 }
}

void removeCase(int number, int digit) {
  switch (number) {
    case 0:
    updateRemoveDisplay(zerobyte0,0,digit);
    updateRemoveDisplay(zerobyte1,1,digit);
    updateRemoveDisplay(zerobyte2,2,digit);
    updateRemoveDisplay(zerobyte3,3,digit);
    break;
    case 1:
    updateRemoveDisplay(onebyte0,0,digit);
    updateRemoveDisplay(onebyte1,1,digit);
    updateRemoveDisplay(onebyte2,2,digit);
    updateRemoveDisplay(onebyte3,3,digit);
    break;
    case 2:
    updateRemoveDisplay(twobyte0,0,digit);
    updateRemoveDisplay(twobyte1,1,digit);
    updateRemoveDisplay(twobyte2,2,digit);
    updateRemoveDisplay(twobyte3,3,digit);
    break;
    case 3:
    updateRemoveDisplay(threebyte0,0,digit);
    updateRemoveDisplay(threebyte1,1,digit);
    updateRemoveDisplay(threebyte2,2,digit);
    updateRemoveDisplay(threebyte3,3,digit);
    break;
    case 4:
    updateRemoveDisplay(fourbyte0,0,digit);
    updateRemoveDisplay(fourbyte1,1,digit);
    updateRemoveDisplay(fourbyte2,2,digit);
    updateRemoveDisplay(fourbyte3,3,digit);
    break;
    case 5:
    updateRemoveDisplay(fivebyte0,0,digit);
    updateRemoveDisplay(fivebyte1,1,digit);
    updateRemoveDisplay(fivebyte2,2,digit);
    updateRemoveDisplay(fivebyte3,3,digit);
    break;
    case 6:
    updateRemoveDisplay(sixbyte0,0,digit);
    updateRemoveDisplay(sixbyte1,1,digit);
    updateRemoveDisplay(sixbyte2,2,digit);
    updateRemoveDisplay(sixbyte3,3,digit);
    break;
    case 7:
    updateRemoveDisplay(sevenbyte0,0,digit);
    updateRemoveDisplay(sevenbyte1,1,digit);
    updateRemoveDisplay(sevenbyte2,2,digit);
    updateRemoveDisplay(sevenbyte3,3,digit);
    break;
    case 8:
    updateRemoveDisplay(eightbyte0,0,digit);
    updateRemoveDisplay(eightbyte1,1,digit);
    updateRemoveDisplay(eightbyte2,2,digit);
    updateRemoveDisplay(eightbyte3,3,digit);
    break;
    case 9:
    updateRemoveDisplay(ninebyte0,0,digit);
    updateRemoveDisplay(ninebyte1,1,digit);
    updateRemoveDisplay(ninebyte2,2,digit);
    updateRemoveDisplay(ninebyte3,3,digit);
    break;
    default:
    break;
  }
  
}

void updateCenterDisplay(int loc) {
int start1, start2;

if (loc == 1) {start1 = 9; start2 = 13;}
if (loc == 2) {start1 = 4; start2 = 18;}

display.drawImage(DIGIT3,1,8,start1,0,0);
display.drawImage(DIGIT0A,4,8,start2,0,0);
display.writeScreen();
delay(slideTime+80);

display.drawImage(DIGIT2,1,8,start1-1,0,0);
display.drawImage(DIGIT1A,4,8,start2+1,0,0);
display.writeScreen();
delay(slideTime+80);

display.drawImage(DIGIT1,1,8,start1-2,0,0);
display.drawImage(DIGIT2A,4,8,start2+2,0,0);
display.writeScreen();
delay(slideTime+80);

display.drawImage(DIGIT0,1,8,start1-3,0,0);
display.drawImage(DIGIT3A,4,8,start2+3,0,0);
display.writeScreen();
delay(slideTime+80);

}

void centerRemove(int loc) {
/*
 * This isn't working correctly.
 */

/* 
int start1, start2;

if (loc == 1) {start1 = 9; start2 = 13;}
if (loc == 2) {start1 = 4; start2 = 18;}

display.drawImage(DIGIT0,1,8,start1+3,0,0);
display.drawImage(DIGIT3A,4,8,start2-3,0,0);
display.writeScreen();
delay(slideTime+80);

display.drawImage(DIGIT1,1,8,start1+2,0,0);
display.drawImage(DIGIT2A,4,8,start2-2,0,0);
display.writeScreen();
delay(slideTime+80);

display.drawImage(DIGIT2,1,8,start1+1,0,0);
display.drawImage(DIGIT1A,4,8,start2-1,0,0);
display.writeScreen();
delay(slideTime+80);

display.drawImage(DIGIT3,1,8,start1,0,0);
display.drawImage(DIGIT0A,4,8,start2,0,0);
display.writeScreen();
delay(slideTime+80);
*/
}

void displayCenterCase(int number1, int number2,  int digit) {
  switch (number1) {
    case 0:
    DIGIT3 = zerobyte3;
    DIGIT2 = zerobyte2;
    DIGIT1 = zerobyte1;
    DIGIT0 = zerobyte0;
    break;
    case 1:
    DIGIT3 = onebyte3;
    DIGIT2 = onebyte2;
    DIGIT1 = onebyte1;
    DIGIT0 = onebyte0;
    break;
    case 2:
    DIGIT3 = twobyte3;
    DIGIT2 = twobyte2;
    DIGIT1 = twobyte1;
    DIGIT0 = twobyte0;
    break;
    case 3:
    DIGIT3 = threebyte3;
    DIGIT2 = threebyte2;
    DIGIT1 = threebyte1;
    DIGIT0 = threebyte0;
    break;
    case 4:
    DIGIT3 = fourbyte3;
    DIGIT2 = fourbyte2;
    DIGIT1 = fourbyte1;
    DIGIT0 = fourbyte0;
    break;
    case 5:
    DIGIT3 = fivebyte3;
    DIGIT2 = fivebyte2;
    DIGIT1 = fivebyte1;
    DIGIT0 = fivebyte0;
    break;
    case 6:
    DIGIT3 = sixbyte3;
    DIGIT2 = sixbyte2;
    DIGIT1 = sixbyte1;
    DIGIT0 = sixbyte0;
    break;
    case 7:
    DIGIT3 = sevenbyte3;
    DIGIT2 = sevenbyte2;
    DIGIT1 = sevenbyte1;
    DIGIT0 = sevenbyte0;
    break;
    case 8:
    DIGIT3 = eightbyte3;
    DIGIT2 = eightbyte2;
    DIGIT1 = eightbyte1;
    DIGIT0 = eightbyte0;
    break;
    case 9:
    DIGIT3 = ninebyte3;
    DIGIT2 = ninebyte2;
    DIGIT1 = ninebyte1;
    DIGIT0 = ninebyte0;
    break;
    default:
    DIGIT3 = blank;
    DIGIT2 = blank;
    DIGIT1 = blank;
    DIGIT0 = blank;
    break;
  }

switch (number2) {
    case 0:
    DIGIT3A = zerobyte3;
    DIGIT2A = zerobyte2;
    DIGIT1A = zerobyte1;
    DIGIT0A = zerobyte0;
    break;
    case 1:
    DIGIT3A = onebyte3;
    DIGIT2A = onebyte2;
    DIGIT1A = onebyte1;
    DIGIT0A = onebyte0;
    break;
    case 2:
    DIGIT3A = twobyte3;
    DIGIT2A = twobyte2;
    DIGIT1A = twobyte1;
    DIGIT0A = twobyte0;
    break;
    case 3:
    DIGIT3A = threebyte3;
    DIGIT2A = threebyte2;
    DIGIT1A = threebyte1;
    DIGIT0A = threebyte0;
    break;
    case 4:
    DIGIT3A = fourbyte3;
    DIGIT2A = fourbyte2;
    DIGIT1A = fourbyte1;
    DIGIT0A = fourbyte0;
    break;
    case 5:
    DIGIT3A = fivebyte3;
    DIGIT2A = fivebyte2;
    DIGIT1A = fivebyte1;
    DIGIT0A = fivebyte0;
    break;
    case 6:
    DIGIT3A = sixbyte3;
    DIGIT2A = sixbyte2;
    DIGIT1A = sixbyte1;
    DIGIT0A = sixbyte0;
    break;
    case 7:
    DIGIT3A = sevenbyte3;
    DIGIT2A = sevenbyte2;
    DIGIT1A = sevenbyte1;
    DIGIT0A = sevenbyte0;
    break;
    case 8:
    DIGIT3A = eightbyte3;
    DIGIT2A = eightbyte2;
    DIGIT1A = eightbyte1;
    DIGIT0A = eightbyte0;
    break;
    case 9:
    DIGIT3A = ninebyte3;
    DIGIT2A = ninebyte2;
    DIGIT1A = ninebyte1;
    DIGIT0A = ninebyte0;
    break;
    default:
    DIGIT3 = blank;
    DIGIT2 = blank;
    DIGIT1 = blank;
    DIGIT0 = blank;
    break;
  }

updateCenterDisplay(digit);

}

void removeCenterCase(int number1, int number2,  int digit) {
  switch (number1) {
    case 0:
    DIGIT3 = zerobyte3;
    DIGIT2 = zerobyte2;
    DIGIT1 = zerobyte1;
    DIGIT0 = zerobyte0;
    break;
    case 1:
    DIGIT3 = onebyte3;
    DIGIT2 = onebyte2;
    DIGIT1 = onebyte1;
    DIGIT0 = onebyte0;
    break;
    case 2:
    DIGIT3 = twobyte3;
    DIGIT2 = twobyte2;
    DIGIT1 = twobyte1;
    DIGIT0 = twobyte0;
    break;
    case 3:
    DIGIT3 = threebyte3;
    DIGIT2 = threebyte2;
    DIGIT1 = threebyte1;
    DIGIT0 = threebyte0;
    break;
    case 4:
    DIGIT3 = fourbyte3;
    DIGIT2 = fourbyte2;
    DIGIT1 = fourbyte1;
    DIGIT0 = fourbyte0;
    break;
    case 5:
    DIGIT3 = fivebyte3;
    DIGIT2 = fivebyte2;
    DIGIT1 = fivebyte1;
    DIGIT0 = fivebyte0;
    break;
    case 6:
    DIGIT3 = sixbyte3;
    DIGIT2 = sixbyte2;
    DIGIT1 = sixbyte1;
    DIGIT0 = sixbyte0;
    break;
    case 7:
    DIGIT3 = sevenbyte3;
    DIGIT2 = sevenbyte2;
    DIGIT1 = sevenbyte1;
    DIGIT0 = sevenbyte0;
    break;
    case 8:
    DIGIT3 = eightbyte3;
    DIGIT2 = eightbyte2;
    DIGIT1 = eightbyte1;
    DIGIT0 = eightbyte0;
    break;
    case 9:
    DIGIT3 = ninebyte3;
    DIGIT2 = ninebyte2;
    DIGIT1 = ninebyte1;
    DIGIT0 = ninebyte0;
    break;
    default:
    DIGIT3 = blank;
    DIGIT2 = blank;
    DIGIT1 = blank;
    DIGIT0 = blank;
    break;
  }

switch (number2) {
    case 0:
    DIGIT3A = zerobyte3;
    DIGIT2A = zerobyte2;
    DIGIT1A = zerobyte1;
    DIGIT0A = zerobyte0;
    break;
    case 1:
    DIGIT3A = onebyte3;
    DIGIT2A = onebyte2;
    DIGIT1A = onebyte1;
    DIGIT0A = onebyte0;
    break;
    case 2:
    DIGIT3A = twobyte3;
    DIGIT2A = twobyte2;
    DIGIT1A = twobyte1;
    DIGIT0A = twobyte0;
    break;
    case 3:
    DIGIT3A = threebyte3;
    DIGIT2A = threebyte2;
    DIGIT1A = threebyte1;
    DIGIT0A = threebyte0;
    break;
    case 4:
    DIGIT3A = fourbyte3;
    DIGIT2A = fourbyte2;
    DIGIT1A = fourbyte1;
    DIGIT0A = fourbyte0;
    break;
    case 5:
    DIGIT3A = fivebyte3;
    DIGIT2A = fivebyte2;
    DIGIT1A = fivebyte1;
    DIGIT0A = fivebyte0;
    break;
    case 6:
    DIGIT3A = sixbyte3;
    DIGIT2A = sixbyte2;
    DIGIT1A = sixbyte1;
    DIGIT0A = sixbyte0;
    break;
    case 7:
    DIGIT3A = sevenbyte3;
    DIGIT2A = sevenbyte2;
    DIGIT1A = sevenbyte1;
    DIGIT0A = sevenbyte0;
    break;
    case 8:
    DIGIT3A = eightbyte3;
    DIGIT2A = eightbyte2;
    DIGIT1A = eightbyte1;
    DIGIT0A = eightbyte0;
    break;
    case 9:
    DIGIT3A = ninebyte3;
    DIGIT2A = ninebyte2;
    DIGIT1A = ninebyte1;
    DIGIT0A = ninebyte0;
    break;
    default:
    DIGIT3 = blank;
    DIGIT2 = blank;
    DIGIT1 = blank;
    DIGIT0 = blank;
    break;
  }

centerRemove(digit);

}

void sms(String apiKey, int notifiedCheck) {
  if (notifiedCheck == 0) {
  if (notifyClient.connect(notifyServer,80)) { 
  Serial.println("Connecting");
  notifyClient.print("GET /pushingbox?devid=");
    notifyClient.print(apiKey);
    notifyClient.println(" HTTP/1.1");
    notifyClient.print("Host: ");
    notifyClient.println(notifyServer);
    notifyClient.println("User-Agent: Arduino");
    notifyClient.println();
  }
  // Read all the lines of the reply from server and print them to Serial
  while(notifyClient.available()){
    String line = notifyClient.readStringUntil('\r');
    Serial.print(line);
     }
  
  Serial.println();
  Serial.println("closing connection");
  notifyClient.stop();
    }
}  

void binary() {
   drawBlank();
   while (mode == 7) {
    timeClient.update(); 
 hours = timeClient.getHours();
 minutes = timeClient.getMinutes();
   if (hours >= 13) { hours = hours -12; }
if (hours == 0) {hours = 12;}

byte tempHour = hours;
byte mask = 0x0001;
if (tempHour & mask) {
  display.setPixel(17,1); } else {
    display.clrPixel(17,1);
  }
if (tempHour>>1 & mask) {
  display.setPixel(13,1); } else {
    display.clrPixel(13,1);
  }
if (tempHour>>2 & mask) {
  display.setPixel(9,1); } else {
    display.clrPixel(9,1);
  }
if (tempHour>>3 & mask) {
  display.setPixel(5,1); } else {
    display.clrPixel(5,1);
  }
//Serial.println(hour);
display.writeScreen();
byte tempMinute = minutes;
if (tempMinute & mask) {
  display.setPixel(21,5); } else {
    display.clrPixel(21,5); }
if (tempMinute>>1 & mask) {
  display.setPixel(17,5); } else {
    display.clrPixel(17,5); }
if (tempMinute>>2 & mask) {
  display.setPixel(13,5); } else {
    display.clrPixel(13,5); }
if (tempMinute>>3 & mask) {
  display.setPixel(9,5); } else {
    display.clrPixel(9,5); }
if (tempMinute>>4 & mask) {
  display.setPixel(5,5); } else {
    display.clrPixel(5,5); }
if (tempMinute>>5 & mask) {
  display.setPixel(1,5); } else {    
    display.clrPixel(1,5); }
display.writeScreen();
yield();
for (int x = 0; x<3; x++) {
for (int seconds=0;seconds<24;seconds++) {

//int tempSeconds = seconds;
//if (seconds > 24) {tempSeconds = 25+seconds;};
 display.setPixel(24-seconds,7);
 display.writeScreen();
 delay(25);
 display.clrPixel(24-seconds+1,7);
 display.writeScreen();
 delay(25);
}

for (int seconds=0;seconds<24;seconds++) {
 // int tempSeconds = seconds;
 // if
 display.clrPixel(0+seconds-1,7);
 display.writeScreen();
 delay(25);
 display.setPixel(0+seconds+1,7);
 display.writeScreen();
 delay(25);
}
}
   }
  ESP.restart();
  yield();
  delay(150);
}

void drawBlank() {
 
  display.drawImage(blankBinary,8,8,4,0,0);
  display.drawImage(blankBinary,8,8,8,0,0);
  display.drawImage(blankBinary,8,8,12,0,0);
  display.drawImage(blankBinary,8,8,16,0,0);
  
  display.drawImage(blankBinary,8,8,0,4,0);
  display.drawImage(blankBinary,8,8,4,4,0);
  display.drawImage(blankBinary,8,8,8,4,0);
  display.drawImage(blankBinary,8,8,12,4,0);
  display.drawImage(blankBinary,8,8,16,4,0);
  display.drawImage(blankBinary,8,8,20,4,0);
  display.drawImage(blankBinary,8,8,24,4,0);
  //display.drawImage(blankBinary,8,8,8,8,0);
 display.writeScreen();
delay(150);
 
}



