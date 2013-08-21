#include <Wire.h>
#include <DS1307RTC.h>
#include <Time.h>
#include <TimeAlarms.h>
#include <Timezone.h>
#include <LiquidCrystal_I2C.h>
#include <SoftwareSerial.h>
#include <EEPROM.h>

// Set the pins on the I2C chip used for LCD connections:
//                    addr,en,rw,rs,d4,d5,d6,d7,bl,blpol
LiquidCrystal_I2C lcd(0x27, 2, 1, 0, 4, 5, 6, 7, 3, POSITIVE);

// The light transistor gate is connected to pin 6
int LIGHT_PIN = 6;

// The bluetooth module is connected to pins 3 and 4
int BT_SERIAL_TX_DIO = 3;
int BT_SERIAL_RX_DIO = 2;

// Setup the bluetooth module
SoftwareSerial BluetoothSerial(BT_SERIAL_TX_DIO, BT_SERIAL_RX_DIO);

// Serial input string
String inputString;

// Set timezone (TODO: store in EEPROM)
TimeChangeRule CEST = {"CEST", Last, Sun, Mar, 2, 120};     //Central European Summer Time
TimeChangeRule CET = {"CET ", Last, Sun, Oct, 3, 60};       //Central European Standard Time
Timezone CE(CEST, CET);

// global that tells us whether the alarm is on
int alarmOn;

void setup() {
  // Initialize serial interface
  Serial.begin(9600);
  
  // Initialize bluetooth serial interface
  BluetoothSerial.begin(9600);
  
  // Initialize LCD and turn on the backlight
  lcd.begin(16,2);
  lcd.backlight();
  
  // Use DS1307 as time provider
  setSyncProvider(RTC.get);
  if (timeStatus() != timeSet) {
    Serial.println("ERROR: Unable to sync with the RTC!");
  } else {
    Serial.println("INFO: System time set from RTC");
  }
  
  // Read alarm time from EEPROM and set it
  alarmOn = EEPROM.read(0);
  int alarmHour = EEPROM.read(1);
  int alarmMinute = EEPROM.read(2);
  if (alarmOn == 1) {
    setAlarmWithoutWrite(alarmHour, alarmMinute);
  }
  
  // Set pin for lights
  pinMode(LIGHT_PIN, OUTPUT);
  
  // DEBUG: set alarm for testing
//  Serial.println("Setting alarm");
//  Alarm.timerOnce(5, turnOnLights);
}

void loop() {
  // Draw clock
  if (timeStatus() == timeSet) {
    time_t t = CE.toLocal(now());
    displayLCDClock(t);
  } else {
    Serial.println("ERROR: Time was not set!");
    Alarm.delay(4000);
  }
  
  // Read commands sent over bluetooth
  while (BluetoothSerial.available()) {
    char input = BluetoothSerial.read();
    inputString += input;
    
    if (input == '\n') {
      processInput(inputString);
      inputString = "";
    }
  }
  
  // (!) Use Alarm.delay to check whether the alarms need to be triggered.
  Alarm.delay(0);
}

void displayLCDClock(time_t t) {
  // Using String instead of sprintf saves 2KB of space
  String wdayStr = ((String)dayShortStr(weekday(t))).substring(0, 1);
  String monthStr = formatTimeString(month(t));
  String dayStr = formatTimeString(day(t));
  String yearStr = String(year(t)).substring(2);
  String dateString = wdayStr + " " + dayStr + "/" + monthStr + "/" + yearStr;
  String hourStr = formatTimeString(hour(t));
  String minStr = formatTimeString(minute(t));
  String timeString = hourStr + ":" + minStr;
  lcd.setCursor(0,0);
  lcd.print(dateString);
  lcd.setCursor(11,0);
  lcd.print(timeString);
}

/*
 Ensures time values are formatted with two decimals.
*/
String formatTimeString(int time) {
  if (time < 10) {
    return "0" + String(time);
  } else {
    return String(time);
  }
}

void turnOnLights() {
  Serial.println("Turning on lights");
  for (int i = 0; i < 255; i++) {
    analogWrite(LIGHT_PIN, i);
    Alarm.delay(100);
  }
}

void processInput(String str) {
  if (str.startsWith("ALARM")) {
    // Format: ALARM HHMM
    int alarmHour = str.substring(6,8).toInt();
    int alarmMinute = str.substring(8,10).toInt();
    setAlarmWithWrite(alarmHour, alarmMinute);
  } else if (str.startsWith("TIME")) {
    // Parse time, convert to UTC and set it.
  } else {
    Serial.println("Invalid command");
  }
}

/**
 Sets the alarm at the given local time hour and minute.
**/
void setAlarm(int alarmHour, int alarmMinute, boolean writeToRom) {
   time_t t = now();
   time_t localTime = CE.toLocal(t);
   tmElements_t alarmTimeElements;
   if (hour(localTime) > alarmHour && minute(localTime) > alarmMinute) {
     breakTime(nextMidnight(t), alarmTimeElements);  // Alarm for next day
   } else {
     breakTime(previousMidnight(t), alarmTimeElements);  // Alarm for previous day
   }
   alarmTimeElements.Hour = alarmHour;
   alarmTimeElements.Minute = alarmMinute;
   // Convert alarm time to UTC to set the alarm
   time_t alarmTime = CE.toUTC(makeTime(alarmTimeElements));
   alarmOn = 1;
   Alarm.alarmOnce(hour(alarmTime), minute(alarmTime), second(alarmTime), turnOnLights);
   if (writeToRom) {
     // Store the local alarm time in EEPROM
     EEPROM.write(0, 1);  // Alarm is turned ON
     EEPROM.write(1, alarmHour);
     EEPROM.write(2, alarmMinute);
   }
   Serial.println("INFO: ALARM set at " + String(alarmHour) + ":" + String(alarmMinute) + " local time");
   Serial.println("INFO: ALARM set at " + String(hour(alarmTime)) + ":" + String(minute(alarmTime)) + " UTC");
}

void setAlarmWithWrite(int alarmHour, int alarmMinute) {
  setAlarm(alarmHour, alarmMinute, true);
}

void setAlarmWithoutWrite(int alarmHour, int alarmMinute) {
  setAlarm(alarmHour, alarmMinute, false);
}
