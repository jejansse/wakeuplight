#include <Wire.h>
#include <DS1307RTC.h>
#include <Time.h>
#include <TimeAlarms.h>
#include <Timezone.h>
#include <LiquidCrystal_I2C.h>
#include <SoftwareSerial.h>

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
    delay(4000);
  }
  
  // (!) Use Alarm.delay instead of delay otherwise alarms won't trigger!
  Alarm.delay(0);
  
  // Read commands sent over bluetooth
  while (BluetoothSerial.available()) {
    char input = BluetoothSerial.read();
    inputString += input;
    
    if (input == '\n') {
      processInput(inputString);
      inputString = "";
    }
  }
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
    // Format: ALARM WEEKDAY HHMM, with WEEKDAY an int
    int alarmWeekDay = str.substring(6,7).toInt();
    int alarmHour = str.substring(8,10).toInt();
    int alarmMinute = str.substring(10,12).toInt();
    timeDayOfWeek_t dow;
    switch (alarmWeekDay) {
      case 0:
        dow = dowMonday;
        break;
      case 1:
        dow = dowTuesday;
        break;
      case 2:
        dow = dowWednesday;
        break;
      case 3:
        dow = dowThursday;
        break;
      case 4:
        dow = dowFriday;
        break;
      case 5:
        dow = dowSaturday;
        break;
      case 6:
        dow = dowSunday;
        break;
      default:
        dow = dowInvalid;
        break;
    }
    if (dow != dowInvalid && alarmHour != 0 && alarmMinute != 0) {
      // FIXME: should set the time to UTC!
      Alarm.alarmRepeat(dow, alarmHour, alarmMinute, 0, turnOnLights);
      Serial.println("INFO: ALARM set at " + String(alarmWeekDay) + " " + String(alarmHour) + ":" + String(alarmMinute));
    } else {
      Serial.println("ERROR: Invalid ALARM command");
    }
  } else if (str.startsWith("TIME")) {
    // Parse time and set time, but remember to set to UTC first!
  } else {
    Serial.println("Invalid command");
  }
}
