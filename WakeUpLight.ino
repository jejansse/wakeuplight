#include <Wire.h>
#include <DS1307RTC.h>
#include <Time.h>
#include <LiquidCrystal_I2C.h>

// Set the pins on the I2C chip used for LCD connections:
//                    addr,en,rw,rs,d4,d5,d6,d7,bl,blpol
LiquidCrystal_I2C lcd(0x27, 2, 1, 0, 4, 5, 6, 7, 3, POSITIVE);

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
}

void loop() {
  if (timeStatus() == timeSet) {
    time_t t = now();
    displayLCDClock(t);
  } else {
    Serial.println("ERROR: Time was not set!");
    delay(4000);
  }
  delay(1000);
}

void displayLCDClock(time_t t) {
  String wdayStr = dayShortStr(weekday(t));
  String monthStr = monthShortStr(month(t));
  String dayStr = String(day(t));
  String yearStr = String(year(t));
  String dateString = wdayStr + " " + monthStr + " " + dayStr + " " + yearStr;
  String hourStr = String(hour(t));
  String minStr = String(minute(t));
  String secStr = String(second(t));
  String timeString = hourStr + ":" + minStr + ":" + secStr;
  lcd.setCursor(0,0);
  lcd.print(dateString);
  lcd.setCursor(4,1);
  lcd.print(timeString);
}


