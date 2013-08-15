#include <Wire.h>
#include <DS1307RTC.h>
#include <Time.h>
#include <TimeAlarms.h>
#include <Timezone.h>
#include <LiquidCrystal_I2C.h>
#include <math.h>

// Set the pins on the I2C chip used for LCD connections:
//                    addr,en,rw,rs,d4,d5,d6,d7,bl,blpol
LiquidCrystal_I2C lcd(0x27, 2, 1, 0, 4, 5, 6, 7, 3, POSITIVE);

// The light transistor gate is connected to pin 6
int LIGHT_PIN = 6;

// Set the buttons
int SETUP_PIN = 2;
int LEFT_PIN = 4;
int RIGHT_PIN = 3;

// Constant that shows how many ms the menu should
// stay open if no buttons are pressed.
int MENU_TIME = 5000;

// Set timezone
TimeChangeRule CEST = {"CEST", Last, Sun, Mar, 2, 120};     //Central European Summer Time
TimeChangeRule CET = {"CET ", Last, Sun, Oct, 3, 60};       //Central European Standard Time
Timezone CE(CEST, CET);

boolean setupPushed = false;
boolean leftPushed = false;
boolean rightPushed = false;
boolean showingMenu = false;
unsigned long lastMenuAction = 0;
int currentMenu = 0;

void setup() {
  // Initialize serial interface
  Serial.begin(9600);
  
  // Initialize LCD and turn on the backlight
  lcd.begin(16,2);
  lcd.backlight();
  
  // Use DS1307 as time provider
  setSyncProvider(RTC.get);
  if (timeStatus() != timeSet) {
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("ERR: NO TIME SYNC");
  }
  
  // Set pin for lights
  pinMode(LIGHT_PIN, OUTPUT);
  
  // Set pin for buttons
  pinMode(SETUP_PIN, INPUT_PULLUP);
  pinMode(LEFT_PIN, INPUT_PULLUP);
  pinMode(RIGHT_PIN, INPUT_PULLUP);
  
  // DEBUG: set alarm for testing
//  Serial.println("Setting alarm");
//  Alarm.timerOnce(5, turnOnLights);
}

void loop() {
  // Draw clock
  if (timeStatus() == timeSet) {
    if(!showingMenu) {
      showTime();
    } else {
      if(millis() - lastMenuAction >= MENU_TIME) {
        showTime();
        showingMenu = false;
        lastMenuAction = 0;
      }
    }
  } else {
    Serial.println("ERROR: Time was not set!");
    delay(4000);
  }
  
  // Check for setup input: ensure we only show menu on
  // each button press.
  if (!showingMenu && !setupPushed && digitalRead(SETUP_PIN) == LOW) {
    setupPushed = true;
    lastMenuAction = millis();
    showMenu(0);
  } else if(setupPushed && digitalRead(SETUP_PIN) == HIGH) {
    setupPushed = false;
  }
  
  if (showingMenu && !leftPushed && digitalRead(LEFT_PIN) == LOW) {
    leftPushed = true;
    showMenu(currentMenu-1);
  } else if(showingMenu && leftPushed && digitalRead(LEFT_PIN) == HIGH) {
    leftPushed = false;
    showMenu(currentMenu+1);
  }
  
  if (showingMenu && !rightPushed && digitalRead(RIGHT_PIN) == LOW) {
    rightPushed = true;
  } else if(showingMenu && rightPushed && digitalRead(RIGHT_PIN) == HIGH) {
    rightPushed = false;
  }
  
  // (!) Use Alarm.delay otherwise alarms won't trigger...
  Alarm.delay(0);
}

void showTime() {
  time_t t = CE.toLocal(now());
  displayLCDClock(t);
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

void showMenu(int index) {
  showingMenu = true;
  switch(index) {
    case 0:
      printMenuHeader("ALARM", true, false);
      break;
    case 1:
      printMenuHeader("CLOCK", false, true);
      break;
  }
}

/**
 Prints the menu header centered on the lcd.
**/
void printMenuHeader(String str, boolean first, boolean last) {
  String menuString = str;
  if(!first)
    menuString = "<" + menuString;
  if(!last)
    menuString = menuString + ">";
  int pos = 16/2 - floor((menuString.length()+2)/2); // FIXME: magic number
  lcd.clear();
  lcd.setCursor(pos, 0);
  lcd.print(menuString);
}
