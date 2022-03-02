// Esp32 Internet Alarm Clock
// Temperature Hummidity
// 2 OLED DISPALY
// Author: George nikolaidis
// Created Jan/2022

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <WiFi.h>
#include "time.h"
#include <EEPROM.h>
#include "BluetoothSerial.h"

#if !defined(CONFIG_BT_ENABLED) || !defined(CONFIG_BLUEDROID_ENABLED)
#error Bluetooth is not enabled! Please run `make menuconfig` to and enable it
#endif

BluetoothSerial SerialBT;

/* create a hardware timer */
hw_timer_t * timer = NULL;
volatile byte state = LOW;

#define EEPROM_SIZE 16
#define DEBOUNCE_TIME 140
#define BUZZER_DURATION 120000
#define LED_BUILTIN 2
#define BUZZER 5

#include "DHT.h"
#define DHTPIN 4        // Digital pin connected to the DHT sensor
#define DHTTYPE DHT22   // DHT11
DHT dht(DHTPIN, DHTTYPE);
float MIN_H=0;
float MAX_H=100;
float MIN_T=-40;
float MAX_T=120;
byte tempCounter = 0;
float h, t, f, prev_t;

String BTData;

const char* ssid       = "PUT_YOUR_ROUTER_SSID_HERE";
const char* password   = "PUT_YOUR_ROUTER_PASSWORD_HERE";

const char* ntpServer = "0.pool.ntp.org";
const long  gmtOffset_sec = 0;
const int   daylightOffset_sec = 3600;

int soundOnDuration;
int count   = 0; //buzzer counter
int counter = 0; //counter for ntp server list
unsigned long curTime = 0;

const char *ntpPool[4] = { "0.pool.ntp.org", "1.pool.ntp.org", "2.pool.ntp.org", "3.pool.ntp.org" };
const char *months[12] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };
const char *day[7]     = { "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" };
const char *yesNo[2]   = { "Yes", "No " };
int alarmArray[5]      = {0,0,0,0,0};

boolean failed    = false;
boolean timeSync  = false;
boolean inMenu    = false;
boolean alarmSet  = false;
boolean buzzerOn  = false;
boolean buzzerOff = false;

//one display
//Adafruit_SSD1306 display(-1);
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
//two displays
Adafruit_SSD1306 display1(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
Adafruit_SSD1306 display2(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);


const unsigned char bellCenter [] PROGMEM = {
0x06, 0x00, 0x06, 0x00, 0x3f, 0xc0, 0x3f, 0xc0, 0x3f, 0xc0, 0x3f, 0xc0, 0xff, 0xf0, 0xff, 0xf0,
0xff, 0xf0, 0xff, 0xf0, 0x00, 0x00, 0x00, 0x00, 0x06, 0x00, 0x06, 0x00, 0x00, 0x00, 0x00, 0x00
};

const unsigned char bellLeft [] PROGMEM = {
0x00, 0x00, 0x00, 0x00, 0x1e, 0x00, 0x1e, 0x00, 0x1e, 0x00, 0x1f, 0xe0, 0x1f, 0xe0, 0x1f, 0xf0,
0x1f, 0xf0, 0x1f, 0xe0, 0x1f, 0xe0, 0xde, 0x00, 0xde, 0x00, 0x1e, 0x00, 0x00, 0x00, 0x00, 0x00
};

const unsigned char bellRight [] PROGMEM = {
0x00, 0x00, 0x00, 0x00, 0x07, 0x80, 0x07, 0x80, 0x07, 0x80, 0x7f, 0x80, 0x7f, 0x80, 0xff, 0x80,
0xff, 0x80, 0x7f, 0x80, 0x7f, 0x80, 0x07, 0xb0, 0x07, 0xb0, 0x07, 0x80, 0x00, 0x00, 0x00, 0x00
};

const unsigned char underScore [] PROGMEM = {
0xff, 0xf0, 0xff, 0xf0, 0xff, 0xf0
};

const unsigned char temperature [] PROGMEM = {
0x07, 0x00, 0x08, 0x80, 0x08, 0x80, 0x08, 0x80, 0x08, 0x80, 0x08, 0x80, 0x08, 0x80, 0x08, 0x80,
0x08, 0x80, 0x08, 0x80, 0x0b, 0x80, 0x08, 0x80, 0x08, 0x80, 0x08, 0x80, 0x0b, 0x80, 0x08, 0x80,
0x08, 0x80, 0x08, 0x80, 0x0b, 0x80, 0x08, 0x80, 0x08, 0x80, 0x08, 0x80, 0x08, 0x80, 0x08, 0x80,
0x0f, 0x80, 0x1f, 0xc0, 0x3f, 0xe0, 0x3f, 0xe0, 0x3f, 0xe0, 0x3f, 0xe0, 0x1f, 0xc0, 0x0f, 0x80
};

const unsigned char degree [] PROGMEM = {
0x00, 0x00, 0x60, 0x00, 0x90, 0x00, 0x90, 0x00, 0x60, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

const unsigned char droplet [] PROGMEM = {
0x04, 0x00, 0x04, 0x00, 0x0e, 0x00, 0x0e, 0x00, 0x1f, 0x00, 0x1f, 0x00, 0x3f, 0x80, 0x3f, 0x80,
0x3f, 0x80, 0x1f, 0x00, 0x0e, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

const unsigned char cloud [] PROGMEM = {
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0f, 0x00, 0x1f, 0x80, 0x7f, 0xc0, 0xff, 0xf0,
0xff, 0xf0, 0xff, 0xf0, 0x7f, 0xe0, 0x3f, 0xc0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

const unsigned char sun [] PROGMEM = {
0x00, 0x00, 0x00, 0x00, 0x42, 0x10, 0x22, 0x20, 0x12, 0x40, 0x00, 0x00, 0x07, 0x00, 0x0f, 0x80,
0x6f, 0xb0, 0x0f, 0x80, 0x07, 0x00, 0x00, 0x00, 0x12, 0x40, 0x22, 0x20, 0x42, 0x10, 0x00, 0x00
};

const unsigned char snow [] PROGMEM = {
0x00, 0x00, 0x15, 0x00, 0x8e, 0x20, 0x44, 0x40, 0x24, 0x80, 0x95, 0x20, 0x44, 0x40, 0xff, 0xe0,
0x44, 0x40, 0x95, 0x20, 0x24, 0x80, 0x44, 0x40, 0x8e, 0x20, 0x15, 0x00, 0x00, 0x00, 0x00, 0x00
};

struct TimeBuffer {
  int tmin;
  int tsec;
  int thour;
  int tyear;
  int tday;
  int tdate;
  int tmonth;
  int tmp_year;
  int tmp_mon;
};

struct AlarmCounters
{
  int hCounter;
  int minCounter;
  int day1Counter;
  int day2Counter;
  int yesNoCounter;
};

struct SButton {
  const uint8_t SET_PIN;
  int setKeyCounter;
  bool setPressed;
};

struct RButton {
  const uint8_t RIGHT_PIN;
  int rightKeyCounter;
  bool rightPressed;
};

struct UButton {
  const uint8_t UP_PIN;
  int upKeyCounter;
  volatile bool upPressed;
};

RButton rightButton = {19, 1, false};
UButton upButton    = {18, 0, false};
SButton setButton   = {23, 0, false};

TimeBuffer timerBuffer = {0,0,0,0,0,0,0,0,0};
AlarmCounters alarmCounters = { 0,0,1,5,1 };

//   int tm_sec;         /* seconds,  range 0 to 59          */
//   int tm_min;         /* minutes, range 0 to 59           */
//   int tm_hour;        /* hours, range 0 to 23             */
//   int tm_mday;        /* day of the month, range 1 to 31  */
//   int tm_mon;         /* month, range 0 to 11             */
//   int tm_year;        /* The number of years since 1900   */
//   int tm_wday;        /* day of the week, range 0 to 6    */
//   int tm_yday;        /* day in the year, range 0 to 365  */
//   int tm_isdst;       /* daylight saving time             */


//****************Interrupt ISR's**********************
void IRAM_ATTR right_isr() {
  volatile unsigned long currentTime = millis();
  while( millis() < currentTime + DEBOUNCE_TIME ) { }
  rightButton.rightPressed = true;
  if(buzzerOn) {
      buzzerOn = false;
      buzzerOff = true;
      timerAlarmDisable(timer);
      timerEnd(timer);
      digitalWrite(BUZZER,LOW);
      digitalWrite(LED_BUILTIN, LOW);
    }
}

void IRAM_ATTR set_isr() {
  volatile unsigned long currentTime = millis();
  while( millis() < currentTime + DEBOUNCE_TIME) { }
  setButton.setKeyCounter += 1;
  setButton.setPressed = true;
}

void IRAM_ATTR up_isr() {
  volatile unsigned long currentTime = millis();
  while( millis() < currentTime + DEBOUNCE_TIME) { }
  upButton.upPressed = true;
}

void IRAM_ATTR onTimer(){
  state = !state;
  digitalWrite(BUZZER,state);
}

//****************Interrupt ISR's**********************


//Check local time date year
void printLocalTime()
{
  struct tm timeinfo;
  int dcout;

  if(timeSync) getLocalTime(&timeinfo);

  if(!getLocalTime(&timeinfo)){
    Serial.println("Failed to obtain time");
    failed = true;
    counter+= 1; //Ntp counter list, hops to the next ntp server in the array
    return;
  }

  Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");
  timerBuffer.tmin   = timeinfo.tm_min;
  timerBuffer.tsec   = timeinfo.tm_sec;
  timerBuffer.thour  = timeinfo.tm_hour;
  timerBuffer.tyear  = timeinfo.tm_year;
  timerBuffer.tday   = timeinfo.tm_wday;
  timerBuffer.tdate  = timeinfo.tm_mday;
  timerBuffer.tmonth = timeinfo.tm_mon;

//Sync the clock every sunday at 03:00:00
  if( timerBuffer.thour == 3 && timerBuffer.tmin == 0 && timerBuffer.tsec == 0 && timerBuffer.tday == 0) {
    timeSync = true;
    syncDateTime();
  }

//check time and date to fire up the alarm
  if( alarmSet )
  {
    if( timerBuffer.thour == alarmCounters.hCounter && timerBuffer.tmin == alarmCounters.minCounter && timerBuffer.tsec == 0 )
    {
      dcout = alarmCounters.day1Counter;
      while ( dcout != alarmCounters.day2Counter )
      {
        if ( dcout == timerBuffer.tday ) {
          digitalWrite(LED_BUILTIN, HIGH);
          /* Start an alarm */
          timer = timerBegin(0, 80, true);
          timerAttachInterrupt(timer, &onTimer, true);
          timerAlarmWrite(timer, 500000, true);
          timerAlarmEnable(timer);
          buzzerOn = true;
          soundOnDuration = millis();
        }
        dcout++;
        if ( dcout > 6) dcout = 0;
      }
      if ( dcout == timerBuffer.tday ) {
        digitalWrite(LED_BUILTIN, HIGH);
        timer = timerBegin(0, 80, true);
        timerAttachInterrupt(timer, &onTimer, true);
        timerAlarmWrite(timer, 500000, true);
        timerAlarmEnable(timer);
        buzzerOn = true;
        soundOnDuration = millis();
      }
    }
  }

  if ( alarmSet == true && buzzerOn ==true)
  {
    if( millis() > soundOnDuration + BUZZER_DURATION ) {
       buzzerOn = false;
       timerAlarmDisable(timer);
       digitalWrite(LED_BUILTIN, LOW);
       digitalWrite(BUZZER,LOW);
    }
  }
}


void setup()
{
  Serial.begin(115200);
  EEPROM.begin(EEPROM_SIZE);

  SerialBT.begin("ESP32Blue"); //Bluetooth device name
  Serial.println("The device started, now you can pair it with bluetooth!");

  pinMode(rightButton.RIGHT_PIN, INPUT_PULLUP);
  pinMode(upButton.UP_PIN, INPUT_PULLUP);
  pinMode(setButton.SET_PIN, INPUT_PULLUP); // INPUT I use 10k resistance with sort cable, to avoid random iterrupt fire up

  attachInterrupt(rightButton.RIGHT_PIN, right_isr, FALLING);
  attachInterrupt(upButton.UP_PIN, up_isr, FALLING);
  attachInterrupt(setButton.SET_PIN, set_isr, FALLING);

  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(BUZZER, OUTPUT);
  digitalWrite(BUZZER,LOW);

  /* Use 1st timer of 4 */
  /* 1 tick take 1/(80MHZ/80) = 1us so we set divider 80 and count up */
  //  timer = timerBegin(0, 80, true);
  /* Attach onTimer function to our timer */
  //  timerAttachInterrupt(timer, &onTimer, true);
  /* Set alarm to call onTimer function every second 1 tick is 1us
  => 1 second is 1000000us */
  /* Repeat the alarm (third parameter) */
  //  timerAlarmWrite(timer, 1000000, true);

//OLED init, boot up message
//One display
//display1.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display1.begin(SSD1306_SWITCHCAPVCC, 0x3C); //Clock display
  display2.begin(SSD1306_SWITCHCAPVCC, 0x3D); //Temperature display
  display1.clearDisplay();
  display2.clearDisplay();
  displayMessage(0, 0, "Booting up", 2);
//Send a message to display2 so not to show just static
  displayCloudSun();
  displayRectangle();
  display2.display();
  delay(2000);

  dht.begin();
  h = dht.readHumidity();
  t = dht.readTemperature();
  prev_t = t;
  MIN_H=h;
  MIN_T=t;
  MAX_H=h;
  MAX_T=t;

//connect to WiFi
  Serial.printf("Connecting to %s ", ssid);
  WiFi.begin(ssid, password);

  display1.clearDisplay();
  displayMessage(0, 0, "Connecting to WiFi..", 1);
  while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
      display1.print(".");
      display1.display();
  }
  Serial.println("CONNECTED");
  displayMessage(0, 16, "Connected to", 1);
  displayMessage(0, 32, ssid, 1);
  delay(2000);

//get and set the time
  configTime(gmtOffset_sec, daylightOffset_sec, ntpPool[counter]);
  printLocalTime();
  bootUpMessage();
//disconnect WiFi as it's no longer needed
  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);
  readEeprom();
  restoreAlarm();
}


void loop()
{

  if(SerialBT.available())
  {
    BTData = SerialBT.readString();
  }
  if (Serial.available()) {
    SerialBT.write(Serial.read());
  }
  if (SerialBT.available()) {
    Serial.write(SerialBT.read());
  }

 // Reading temperature or humidity takes about 250 milliseconds!
  // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
  h = dht.readHumidity();
  // Read temperature as Celsius (the default)
  t = dht.readTemperature();
  // Read temperature as Fahrenheit (isFahrenheit = true)
  f = dht.readTemperature(true);
  // Check if any reads failed and exit early (to try again).
  if (isnan(h) || isnan(t) || isnan(f)) {
    Serial.println(F("Failed to read from DHT sensor!"));
    return;
  }
  // Compute heat index in Fahrenheit (the default)
  float hif = dht.computeHeatIndex(f, h);
  // Compute heat index in Celsius (isFahreheit = false)
  float hic = dht.computeHeatIndex(t, h, false);
  if(MIN_H>=h) {
    MIN_H=h;
  }
  if(MIN_T>=t) {
    MIN_T=t;
  }
  if(MAX_H<=h) {
    MAX_H=h;
  }
  if(MAX_T<=t){
    MAX_T=t;
  }
  if(tempCounter > 3) tempCounter = 0;
  if(tempCounter == 0 && timerBuffer.tsec <= 20) printFirstTempPage();
  if(tempCounter == 1 && timerBuffer.tsec > 20 && timerBuffer.tsec <= 40 ) printSecTempPage();
  if(tempCounter == 2 && timerBuffer.tsec > 40 && timerBuffer.tsec <= 50) printThirdTempPage();
  if(tempCounter == 3 && timerBuffer.tsec > 50 && timerBuffer.tsec <= 59) printForthTempPage();

  if(prev_t != t)
  {
    prev_t = t;
    sendBTData();
  }

  if(BTData == "get")
  {
    sendBTData();
    BTData = "";
  }

 //delay(700);
 if(buzzerOn){
  count++;
  if(count > 3) count = 0;
  if(count == 0) moveBell(bellCenter);
  if(count == 1) moveBell(bellLeft);
  if(count == 2) moveBell(bellCenter);
  if(count == 3) moveBell(bellRight);
 }
//Move the bell on the center pos when stop the alarm
 if(buzzerOff)
 {
  moveBell(bellCenter);
  buzzerOff = false;
 }

//Manual sync with ntp
 if(upButton.upPressed && !inMenu)
 {
  upButton.upPressed = false;
  timeSync = true;
  syncDateTime();
 }
 if(millis() > curTime + 9000 && timeSync)
 {
   displayMessage(0, 56, "                     ", 1);
   timeSync = false;
   WiFi.disconnect(true);
   WiFi.mode(WIFI_OFF);
 }

 printLocalTime();
 checkFailedTime();
 lcdPrint();

 if ( setButton.setPressed ){ printAlarmMenu(); }

 while (inMenu)
 {
  if( setButton.setPressed && alarmCounters.yesNoCounter == 0) //when choose "Yes" in the save menu
  {
    setButton.setPressed = false;
    alarmSet = true;
    alarmArray[0] = alarmCounters.hCounter;
    alarmArray[1] = alarmCounters.minCounter;
    alarmArray[2] = alarmCounters.day1Counter;
    alarmArray[3] = alarmCounters.day2Counter;
    alarmArray[4] = alarmCounters.yesNoCounter;
    writeEeprom();
    inMenu = false;
    display1.clearDisplay();
    moveBell(bellCenter);
  }

  if( setButton.setPressed && alarmCounters.yesNoCounter == 1) //when choose "no" in the save menu
  {
    setButton.setPressed = false;
    alarmSet = false;
    display1.setCursor(110,18);
    display1.setTextSize(2);
    display1.setTextColor(WHITE,BLACK);
    display1.print("  ");
    display1.display();
    if(EEPROM.read(5) != 255 && EEPROM.read(4) != 1) {
      EEPROM.writeInt(4, 1);
      EEPROM.writeInt(5, 255);
      EEPROM.commit();
    }
    inMenu = false;
    display1.clearDisplay();
  }

  if( rightButton.rightPressed )
  {
    rightButton.rightKeyCounter += 1;
    rightButton.rightPressed = false;
    if( rightButton.rightKeyCounter > 5 ){ rightButton.rightKeyCounter = 1; }
    setCursorToPos(rightButton.rightKeyCounter);
  }

  if( upButton.upPressed )
  {
    upButton.upPressed = false;
    //lastState1 = false;
    if(rightButton.rightKeyCounter == 1) alarmCounters.hCounter++;
    if( alarmCounters.hCounter > 23 && rightButton.rightKeyCounter == 1)
    {
      alarmCounters.hCounter = 0;
      dispaly2chars(60,0,"00");
    }
    if( alarmCounters.hCounter < 10 && rightButton.rightKeyCounter == 1)
    {
      dispaly1digit(72,0,alarmCounters.hCounter);
    }
    if( alarmCounters.hCounter >= 10 && rightButton.rightKeyCounter == 1)
    {
      dispaly2digits(60,0,alarmCounters.hCounter);
    }

    if(rightButton.rightKeyCounter == 2) alarmCounters.minCounter++;
    if( alarmCounters.minCounter > 59 && rightButton.rightKeyCounter == 2)
    {
      alarmCounters.minCounter = 0;
      dispaly2chars(96,0,"00");
    }
    if( alarmCounters.minCounter < 10 && rightButton.rightKeyCounter == 2)
    {
      dispaly1digit(108,0,alarmCounters.minCounter);
    }
    if( alarmCounters.minCounter >= 10 && rightButton.rightKeyCounter == 2)
    {
      dispaly2digits(96,0,alarmCounters.minCounter);
    }

    if(rightButton.rightKeyCounter == 3) alarmCounters.day1Counter++;
    if(alarmCounters.day1Counter >= 7 && rightButton.rightKeyCounter == 3)
    {
      alarmCounters.day1Counter = 0;
      dispaly3char(12,22,day[alarmCounters.day1Counter]);
    }
    if(alarmCounters.day1Counter < 7 && rightButton.rightKeyCounter == 3)
    {
      dispaly3char(12,22,day[alarmCounters.day1Counter]);
    }
    if(rightButton.rightKeyCounter == 4) alarmCounters.day2Counter++;
    if(alarmCounters.day2Counter >= 7 && rightButton.rightKeyCounter == 4)
    {
      alarmCounters.day2Counter = 0;
      dispaly3char(60,22,day[alarmCounters.day2Counter]);
    }
    if(alarmCounters.day2Counter < 7 && rightButton.rightKeyCounter == 4)
    {
      dispaly3char(60,22,day[alarmCounters.day2Counter]);
    }

    if(rightButton.rightKeyCounter == 5) alarmCounters.yesNoCounter++;
    if(alarmCounters.yesNoCounter > 1 && rightButton.rightKeyCounter == 5)
    {
      alarmCounters.yesNoCounter = 0;
      dispaly3char(72,44,yesNo[alarmCounters.yesNoCounter]);
    }
    if(alarmCounters.yesNoCounter <= 1 && rightButton.rightKeyCounter == 5)
    {
      dispaly3char(72,44,yesNo[alarmCounters.yesNoCounter]);
    }
  }
 }
}

//Move the bell left, center, right
void moveBell(const unsigned char bellArray[])
{
  display1.setCursor(108,16);
  display1.setTextSize(2);
  display1.setTextColor(WHITE,BLACK);
  display1.print("  ");
  display1.setCursor(108,16);
  display1.drawBitmap(108, 16, bellArray, 12, 16, WHITE);
  display1.display();
  delay(200);
}

//Save alarm values to eprom
void writeEeprom()
{
  bool notEq = false;
  for (int i = 0;  i < 5; i++)
  {
    if( alarmArray[i] != EEPROM.read(i) )
    {
      notEq = true;
      EEPROM.writeInt(i, alarmArray[i]);
    }
  }
  if(notEq)
  {
    if(EEPROM.read(5) != 100){
      EEPROM.writeInt(5, 100);
      EEPROM.commit();
    }else {
      EEPROM.commit();
    }
  }
}

//read alarm settings from eeprom
void readEeprom()
{
  int tmpBuffer;
  for(int i=0; i<6;i++)
  {
    tmpBuffer = EEPROM.read(i);
  }
}

//restore alarm settings from eeprom to the main program
void restoreAlarm()
{
//Read EEPROM address 5 and if you find 100 then there are data available for the alarm to restore after power failure.
  byte tmpRead = EEPROM.read(5);
  if(tmpRead == 100)
  {
     alarmCounters.hCounter     = EEPROM.read(0);
     alarmCounters.minCounter   = EEPROM.read(1);
     alarmCounters.day1Counter  = EEPROM.read(2);
     alarmCounters.day2Counter  = EEPROM.read(3);
     alarmCounters.yesNoCounter = EEPROM.read(4);
     alarmSet = true;
     moveBell(bellCenter);
  }
}

//Functions to position time date year
void dispaly1digit(int X, int Y, int cnt)
{
  display1.setCursor(X,Y);
  display1.setTextColor(WHITE, BLACK);
  display1.print(" ");
  display1.setCursor(X,Y);
  display1.print(cnt);
  display1.display();
}

void dispaly2digits(int X, int Y, int num)
{
  display1.setCursor(X,Y);
  display1.setTextColor(WHITE, BLACK);
  display1.print("  ");
  display1.setCursor(X,Y);
  display1.print(num);
  display1.display();
}

void dispaly2chars(int X, int Y, const char *strArray)
{
  display1.setCursor(X,Y);
  display1.setTextColor(WHITE, BLACK);
  display1.print(" ");
  display1.setCursor(X,Y);
  display1.print(strArray);
  display1.display();
}

void dispaly3char(int X, int Y, const char *strArray)
{
  display1.setCursor(X,Y);
  display1.setTextColor(WHITE, BLACK);
  display1.print("   ");
  display1.setCursor(X,Y);
  display1.print(strArray);
  display1.display();
}

//Move cursor to places
void setCursorToPos (int pos)
{
  switch (pos)
  {
    case 1:
      display1.drawBitmap(71, 60, underScore, 12, 3, BLACK);
      display1.drawBitmap(71, 16, underScore, 12, 3, WHITE);
      display1.display();
      break;
    case 2:
      display1.drawBitmap(71, 16, underScore, 12, 3, BLACK);
      display1.drawBitmap(107, 16, underScore, 12, 3, WHITE);
      display1.display();
      break;
    case 3:
      display1.drawBitmap(107, 16, underScore, 12, 3, BLACK);
      display1.drawBitmap(11, 38, underScore, 12, 3, WHITE);
      display1.display();
      break;
    case 4:
      display1.drawBitmap(11, 38, underScore, 12, 3, BLACK);
      display1.drawBitmap(59, 38, underScore, 12, 3, WHITE);
      display1.display();
      break;
    case 5:
      display1.drawBitmap(59, 38, underScore, 12, 3, BLACK);
      display1.drawBitmap(71, 60, underScore, 12, 3, WHITE);
      display1.display();
      break;
    default:
      display1.print("err");
  }
}

//Sync time with ntp
void syncDateTime() {
  struct tm timeinfo;
  displayMessage(0, 56, "Connecting to WiFi", 1);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
      delay(400);
      printLocalTime();
      lcdPrint();
  }
  displayMessage(0, 56, "Sync with ntp srv ", 1);
  curTime = millis();
  configTime(gmtOffset_sec, daylightOffset_sec, ntpPool[counter]);
  getLocalTime(&timeinfo);
  //WiFi.disconnect(true);
  //WiFi.mode(WIFI_OFF);
}

//Check if getting the time failed
void checkFailedTime()
{
  struct tm timeinfo;
  if ( failed && counter < 4 ) {
    displayMessage(0, 56, "Connecting to WiFi", 1);
    if ( WiFi.status() != WL_CONNECTED )
    {
      connectWiFi();
    }
    displayMessage(0, 56, "Connected..          ", 1);
    configTime(gmtOffset_sec, daylightOffset_sec, ntpPool[counter]);
    getLocalTime(&timeinfo);
    failed = false;
    timeSync = true;
    //WiFi.disconnect(true);
    //WiFi.mode(WIFI_OFF);
  } else if( failed == true && counter > 3) {
    displayMessage(0, 56, "Sync FailedRetryLater", 1);
    delay(1000);
    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);
    delay(100);
    ESP.restart();
  }
}

//Connect to WiFi
void connectWiFi()
{
 WiFi.begin(ssid, password);
 while (WiFi.status() != WL_CONNECTED) {
    delay(300);
 }
}

//Display messages
void displayMessage(int X, int Y, const char *message, int Size)
{
  display1.setTextSize(Size);
  display1.setCursor(X,Y);
  display1.setTextColor(WHITE,BLACK);
  display1.print("                     ");
  display1.setCursor(X,Y);
  display1.println(message);
  display1.display();
}

void bootUpMessage()
{
  display1.clearDisplay();
  display1.setTextSize(1);
  display1.setTextColor(WHITE);
  display1.setCursor(0,0);
  display1.println("Ntp internet clock");
  display1.display();
  display1.println(" ");
  display1.println("Esp32 Wroom 32");
  display1.display();
  display1.println(" ");
  display1.println("Created 20/Nov/2021");
  display1.display();
  display1.println(" ");
  display1.print("By George Nikolaidis");
  display1.display();
  delay(4000);
  display1.clearDisplay();
}

//Print time date month year
void lcdPrint() {
//Lcd print the Day DD/MM/YYYY
  display1.setTextSize(2);
  display1.setTextColor(WHITE);
  display1.setCursor(0,0);
  display1.setTextColor(WHITE,BLACK);
  display1.print("   ");
  display1.setCursor(0,0);
  switch (timerBuffer.tday) {
      case 0:
        display1.print(day[0]);
        break;
      case 1:
        display1.print(day[1]);
        break;
      case 2:
        display1.print(day[2]);
        break;
      case 3:
        display1.print(day[3]);
        break;
      case 4:
        display1.print(day[4]);
        break;
      case 5:
        display1.print(day[5]);
        break;
      case 6:
        display1.print(day[6]);
        break;
      default:
        display1.print("err");
  }
  display1.display();

  addZero(timerBuffer.tdate,48,0);
  display1.print(" ");
  display1.setTextColor(WHITE,BLACK);
  display1.print("   ");
  display1.setCursor(84,0);
  switch (timerBuffer.tmonth) {
      case 0:
        display1.println(months[0]);
        break;
      case 1:
        display1.println(months[1]);
        break;
      case 2:
        display1.println(months[2]);
        break;
      case 3:
        display1.println(months[3]);
        break;
      case 4:
        display1.println(months[4]);
        break;
      case 5:
        display1.println(months[5]);
        break;
      case 6:
        display1.println(months[6]);
        break;
      case 7:
        display1.println(months[7]);
        break;
      case 8:
        display1.println(months[8]);
        break;
      case 9:
        display1.println(months[9]);
        break;
      case 10:
        display1.println(months[10]);
        break;
      case 11:
        display1.println(months[11]);
        break;
      default:
        display1.println("err");
  }
  display1.display();
  addZero(timerBuffer.thour,0,16);
  display1.print(":");
  display1.display();
  addZero(timerBuffer.tmin,36,16);
  display1.print(":");
  display1.display();
  addZero(timerBuffer.tsec,72,16);
  display1.setCursor(0,32);
  display1.setTextColor(WHITE,BLACK);
  display1.print("       ");
  timerBuffer.tmp_year = timerBuffer.tyear + 1900;
  display1.setCursor(0,32);
  display1.print("   ");
  display1.print(timerBuffer.tmp_year);

}

//Add a zero in front of date and HH/MM/SS
void addZero(int tmp,int x, int y) {
  if ( tmp < 10 ) {
    display1.setTextColor(WHITE,BLACK);
    display1.setCursor(x,y);
    display1.print(" ");
    display1.setCursor(x,y);
    display1.print("0");
    display1.print(tmp);
  } else {
    display1.setTextColor(WHITE,BLACK);
    display1.setCursor(x,y);
    display1.print("  ");
    display1.setCursor(x,y);
    display1.print(tmp);
  }
}

//Alarm menu
void printAlarmMenu()
{
  if( !alarmSet )
  {
    alarmCounters = { 0,0,1,5,1 };
    rightButton.rightKeyCounter = 1;
    display1.clearDisplay();
    display1.setTextSize(2);
    display1.setCursor(0,0);
    display1.println("Time 00:00");
    display1.setCursor(0,22);
    display1.println(" Mon Fri");
    display1.setCursor(0,44);
    display1.println("Save: No");
    display1.setCursor(71,16);
    display1.drawBitmap(71, 16, underScore, 12, 3, WHITE);
    display1.display();
    setButton.setPressed     = false;
    upButton.upPressed       = false;
    rightButton.rightPressed = false;
    inMenu = true;
    setCursorToPos(1);
  }else
 {
    rightButton.rightKeyCounter = 1;
    display1.clearDisplay();
    display1.setTextSize(2);
    display1.setCursor(0,0);
    display1.print("Time ");
    addZero(alarmCounters.hCounter,60,0);
    display1.print(":");
    addZero(alarmCounters.minCounter,96,0);
    display1.setCursor(0,22);
    display1.print(" ");
    display1.print(day[alarmCounters.day1Counter]);
    display1.print(" ");
    display1.print(day[alarmCounters.day2Counter]);
    display1.setCursor(0,44);
    display1.print("Save: Yes");
    display1.drawBitmap(71, 16, underScore, 12, 3, WHITE);
    display1.display();
    upButton.upPressed       = false;
    rightButton.rightPressed = false;
    setButton.setPressed     = false;
    inMenu = true;
    setCursorToPos(1);
 }
}

void dispalyPosT(float t, byte X, byte Y)
{
  display2.print(t);
  display2.setCursor(X,Y);
  display2.drawBitmap(X, Y, degree, 12, 16, WHITE);
  display2.setCursor(X+12,Y);
  display2.print("C");
  display2.display();
}

void dispalyPosH(byte X, byte Y, float h)
{
  display2.setCursor(24,32);
  display2.print("Humidity");
  display2.setCursor(0,Y);
  display2.print(h);
  display2.print("%");
  display2.setCursor(0,Y);
  display2.drawBitmap(X, Y, droplet, 12, 16, WHITE);
  display2.display();
}

void displayLo(int X, int Y, float tmin)
  {
    display2.setCursor(0,Y);
    display2.print("L ");
    display2.print(tmin);
    display2.setCursor(X,Y);
    display2.drawBitmap(X, Y, degree, 12, 16, WHITE);
    display2.setCursor(X+12,Y);
    display2.print("C");
    display2.display();
  }

void displayHi(int X, int Y, float thi)
  {
    display2.setCursor(0,Y);
    display2.print("H ");
    display2.print(thi);
    display2.setCursor(X,Y);
    display2.drawBitmap(X, Y, degree, 12, 16, WHITE);
    display2.setCursor(X+12,Y); //L -2.00*C
    display2.print("C");
    display2.display();
  }

void displayCloudSun()
{
  display2.setTextSize(2);
  display2.setTextColor(WHITE);
  display2.setCursor(0,0);
  display2.println("Temp");
  display2.drawBitmap(60, 0, cloud, 12, 16, WHITE);
  display2.drawBitmap(84, 0, sun, 12, 16, WHITE);
  display2.drawBitmap(115, 0, temperature, 12, 32, WHITE);
  display2.display();
}

void displayRectangle()
{
  display2.drawRect(0, 36, 127, 8, WHITE);//104
}

void printFirstTempPage()
{
  display2.clearDisplay();
  displayCloudSun();
  if(t<0 && t<-10) dispalyPosT(t, 72, 16);//-12.00
  if(t<=-10) dispalyPosT(t, 60, 16);//-9.00
  if(t<10 && t>=0) dispalyPosT(t, 48, 16);//7.00
  if(t>=10) dispalyPosT(t, 60, 16);//12.00
  if(h<100) dispalyPosH(72, 48, h);
  if(h>=100) dispalyPosH(84, 48, h);
  tempCounter++;
}

void printSecTempPage()
{
  display2.clearDisplay();
  displayCloudSun();
  if(t<0 && t<-10) dispalyPosT(t, 72, 16);//-12.00
  if(t<=-10) dispalyPosT(t, 60, 16);//-9.00
  if(t<10 && t>=0) dispalyPosT(t, 48, 16);//7.00
  if(t>=10) dispalyPosT(t, 60, 16);//12.00
  displayRectangle();
  display2.setCursor(36,48);
  display2.print(f);
  display2.setCursor(96,48);
  display2.drawBitmap(96, 48, degree, 12, 16, WHITE);
  display2.setCursor(108,48);
  display2.print("F");
  display2.display();
  tempCounter++;
}

void printThirdTempPage()
{
  display2.clearDisplay();
  displayCloudSun();
  if(MIN_T<0 && MIN_T<-10) displayLo(84, 16, MIN_T);
  if(MIN_T<-10) displayLo(96, 16, MIN_T);
  if(MIN_T<10 && MIN_T>=0) displayLo(72, 16, MIN_T);
  if(MIN_T>=10) displayLo(84, 16, MIN_T);
  displayRectangle();
  if(MAX_T<0 && MAX_T<-10) displayHi(84, 48, MAX_T);
  if(MAX_T<-10) displayHi(96, 48, MAX_T);
  if(MAX_T<10 && MAX_T>=0) displayHi(72, 48, MAX_T);
  if(MAX_T>=10) displayHi(84, 48, MAX_T);
  tempCounter++;
}

void printForthTempPage()
{
  display2.clearDisplay();
  display2.drawBitmap(115, 0, temperature, 12, 32, WHITE);
  display2.setCursor(0,0);
  display2.setTextSize(2);
  display2.println("Humidity");
  display2.print("L ");
  display2.print(MIN_H);
  display2.print("%");
  display2.drawBitmap(96, 16, droplet, 12, 16, WHITE);
  displayRectangle();
  display2.setCursor(0,48);
  if(h>=100)
  {
    display2.print("H");
    display2.print(MAX_H);
    display2.print("%");
  } else {
    display2.print("H ");
    display2.print(MAX_H);
    display2.print("%");
  }
  display2.drawBitmap(96, 48, droplet, 12, 16, WHITE);
  display2.display();
  tempCounter++;
}

void sendBTData()
{
  SerialBT.print("Temperature: ");
  SerialBT.println(prev_t);
  SerialBT.print("Humudity: ");
  SerialBT.println(h);
}
