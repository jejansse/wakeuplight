#include <Wire.h>
#include <DS1307RTC.h>
#include <Time.h>
#include <TimeAlarms.h>
#include <Timezone.h>
#include <LiquidCrystal_I2C.h>

// Set the pins on the I2C chip used for LCD connections:
//                    addr,en,rw,rs,d4,d5,d6,d7,bl,blpol
LiquidCrystal_I2C lcd(0x27, 2, 1, 0, 4, 5, 6, 7, 3, POSITIVE);

// The light transistor gate is connected to pin 6
int LIGHT_PIN = 6;

// Set timezone
TimeChangeRule CEST = {"CEST", Last, Sun, Mar, 2, 120};     //Central European Summer Time
TimeChangeRule CET = {"CET ", Last, Sun, Oct, 3, 60};       //Central European Standard Time
Timezone CE(CEST, CET);

void setup() {
  // Initialize serial interface
  Serial.begin(9600);
  
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
  Serial.println("Setting alarm");
  Alarm.timerOnce(5, turnOnLights);
}

void loop() {
  if (timeStatus() == timeSet) {
    time_t t = CE.toLocal(now());
    displayLCDClock(t);
  } else {
    Serial.println("ERROR: Time was not set!");
    delay(4000);
  }
  // (!) Use Alarm.delay instead of delay otherwise alarms won't trigger!
  Alarm.delay(1000);
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
