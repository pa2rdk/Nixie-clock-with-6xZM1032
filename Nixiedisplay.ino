#include "Arduino.h"
#include <Wire.h>
#define DS3231_I2C_ADDRESS 0x68

int timer1_counter = 34286;
byte actNixie = 0;
byte digit1 = 0;
byte digit2 = 1;
byte digit3 = 2;
byte digit4 = 3;
byte digit5 = 4;
byte digit6 = 5;
byte digitS = 0;
int intCnt = 0;
bool secLed = 1;
bool isSummerTime = 0;
bool clockMode = 1;
long currentMillis;
long passedMillis = 0;

char receivedString[8];
char chkGS[3] = "GS";
byte second, minute, hour, dayOfWeek, dayOfMonth, month, year;

const byte pos[7] = {4, 6, 8, 10, 16, 18};

void setup() {
  // put your setup code here, to run once:
  pinMode(3, OUTPUT);
  pinMode(4, OUTPUT);
  pinMode(5, OUTPUT);

  digitalWrite(3, secLed);
  digitalWrite(4, secLed);
  digitalWrite(5, secLed);

  pinMode(6, OUTPUT);
  pinMode(7, OUTPUT);
  pinMode(8, OUTPUT);
  pinMode(9, OUTPUT);

  pinMode(10, OUTPUT);
  pinMode(11, OUTPUT);
  pinMode(12, OUTPUT);
  pinMode(13, OUTPUT);

  Serial.begin(9600);
  Serial.println(F("Lets start"));
  Wire.begin();

  noInterrupts();           // disable all interrupts
  TCCR2A = 0;// set entire TCCR0A register to 0
  TCCR2B = 0;// same for TCCR0B
  TCNT2  = 0;//initialize counter value to 0
  // set compare match register for 2khz increments
  OCR2A = 250;// = (16*10^6) / (500*256) - 1 (must be <256)
  // turn on CTC mode
  TCCR2A |= (1 << WGM21);
  // Set CS22 for 256 prescaler
  TCCR2B |= (1 << CS22); //| (1 << CS00);
  // enable timer compare interrupt
  TIMSK2 |= (1 << OCIE2A);
  interrupts();             // enable all interrupts
  delay(1000);
  for (int i = 0; i < 10; i++) {
    digit1 = i;
    digit2 = i;
    digit3 = i;
    digit4 = i;
    digit5 = i;
    digit6 = i;
    delay(500);
  }
  Serial.println(F("Type GS to enter setup:"));
  delay(5000);
  if (Serial.available()) {
    Serial.println(F("Check for setup"));
    if (Serial.find(chkGS)) {
      setKlok();
    }
  }
  readDS3231time();
  if (month == 3 && dayOfMonth > 24)  isSummerTime = 1;
  if (month == 10 && dayOfMonth < 24) isSummerTime = 1;
  if (month == 10 && dayOfMonth < 29 && year == 2017) isSummerTime = 1;
  if (month > 3 && month < 10) isSummerTime = 1;

}

void loop() {
  delay(1000);
  while (Serial.available() > 0) {
    clockMode = 0;
    currentMillis = millis();
    digitS = Serial.read();
    if ((digitS > 47 && digitS < 58) || digitS == 46) {
      digit6 = digit5;
      digit5 = digit4;
      digit4 = digit3;
      digit3 = digit2;
      digit2 = digit1;
      digit1 = digitS;
    }
  }

  if (clockMode == 0 && millis() - currentMillis > 5000) {
    digitalWrite(4, 1);
    clockMode = 1;
  }

  if (clockMode == 1) {
    displayTime();
  }

  if (month == 3 && dayOfMonth > 24 && dayOfWeek == 1 && hour == 2 && isSummerTime == 0) {
    isSummerTime = 1;
    hour++;
    setDS3231time();
  }
  if (month == 10 && dayOfMonth > 24 && dayOfWeek == 1 && hour == 2  && isSummerTime == 1) {
    isSummerTime = 0;
    hour--;
    setDS3231time();
  }
}

void chgNrs() {
  digit1++;
  if (digit1 > 9) digit1 = 0;
  digit2++;
  if (digit2 > 9) digit2 = 0;
  digit3++;
  if (digit3 > 9) digit3 = 0;
  digit4++;
  if (digit4 > 9) digit4 = 0;
  digit5++;
  if (digit5 > 9) digit5 = 0;
  digit6++;
  if (digit6 > 9) digit6 = 0;
}

ISR(TIMER2_COMPA_vect) // Timer1_COMPA_vect timer compare interrupt service routine
{
  intCnt++;
  if (intCnt > 1) {
    //turnAllOff();
  }
  if (intCnt == 4) {
    nextNixie();
    intCnt = 0;
  }

}

void readDS3231time()
//byte * second, byte * minute, byte * hour, byte * dayOfWeek, byte * dayOfMonth, byte * month, byte * year)
{
  Wire.beginTransmission(DS3231_I2C_ADDRESS);
  Wire.write(0); // set DS3231 register pointer to 00h
  Wire.endTransmission();
  Wire.requestFrom(DS3231_I2C_ADDRESS, 7);
  // request seven bytes of data from DS3231 starting from register 00h
  second = bcdToDec(Wire.read() & 0x7f);
  minute = bcdToDec(Wire.read());
  hour = bcdToDec(Wire.read() & 0x3f);
  dayOfWeek = bcdToDec(Wire.read());
  dayOfMonth = bcdToDec(Wire.read());
  month = bcdToDec(Wire.read());
  year = bcdToDec(Wire.read());

  digit6 = (hour / 10);
  digit5 = hour - (digit6 * 10);
  digit4 = (minute / 10);
  digit3 = minute - (digit4 * 10);
  digit2 = (second / 10);
  digit1 = second - (digit2 * 10);
  secLed = ((second & 1) == 1);
}

void setDS3231time() {
  // sets time and date data to DS3231
  Wire.beginTransmission(DS3231_I2C_ADDRESS);
  Wire.write(0); // set next input to start at the seconds register
  Wire.write(decToBcd(second)); // set seconds
  Wire.write(decToBcd(minute)); // set minutes
  Wire.write(decToBcd(hour)); // set hours
  Wire.write(decToBcd(dayOfWeek)); // set day of week (1=Sunday, 7=Saturday)
  Wire.write(decToBcd(dayOfMonth)); // set date (1 to 31)
  Wire.write(decToBcd(month)); // set month
  Wire.write(decToBcd(year)); // set year (0 to 99)
  Wire.endTransmission();
}

byte decToBcd(byte val) {
  return ( (val / 10 * 16) + (val % 10) );
}

// Convert binary coded decimal to normal decimal numbers
byte bcdToDec(byte val) {
  return ( (val / 16 * 10) + (val % 16) );
}

void setKlok() {
  Serial.print("Year:");
  year = getNumericValue();
  Serial.println(year);
  Serial.print("Month:");
  month = getNumericValue();
  Serial.println(month);
  Serial.print("Day:");
  dayOfMonth = getNumericValue();
  Serial.println(dayOfMonth);
  Serial.print("Day of week (1=sunday):");
  dayOfWeek = getNumericValue();
  Serial.println(dayOfWeek);
  Serial.print("Hour:");
  hour = getNumericValue();
  Serial.println(hour);
  Serial.print("Minute:");
  minute = getNumericValue();
  Serial.println(minute);
  Serial.print("Second:");
  second = getNumericValue();
  Serial.println(second);
  setDS3231time();
}

byte getNumericValue() {
  serialFlush();
  byte myByte = 0;
  byte inChar = 0;
  bool isNegative = false;
  receivedString[0] = 0;

  int i = 0;
  while (inChar != 13) {
    if (Serial.available() > 0) {
      inChar = Serial.read();
      if (inChar > 47 && inChar < 58) {
        receivedString[i] = inChar;
        i++;
        Serial.write(inChar);
        myByte = (myByte * 10) + (inChar - 48);
      }
      if (inChar == 45) {
        Serial.write(inChar);
        isNegative = true;
      }
    }
  }
  receivedString[i] = 0;
  if (isNegative == true) myByte = myByte * -1;
  serialFlush();
  return myByte;
}

void serialFlush() {
  for (int i = 0; i < 10; i++)
  {
    while (Serial.available() > 0) {
      Serial.read();
    }
  }
}

void nextNixie() {
  actNixie++;
  if (actNixie == 7) actNixie = 1;

  switch (actNixie) {
    case 1:
      turnOnNixie(1, digit1);
      break;
    case 2:
      turnOnNixie(2, digit2);
      break;
    case 3:
      turnOnNixie(3, digit3);
      break;
    case 4:
      turnOnNixie(4, digit4);
      break;
    case 5:
      turnOnNixie(5, digit5);
      break;
    case 6:
      turnOnNixie(6, digit6);
      break;
  }
}

void displayTime() {
  // retrieve data from DS3231
  readDS3231time();
  // send it to the serial monitor
  Serial.print(hour, DEC);
  // convert the byte variable to a decimal number when displayed
  Serial.print(":");
  if (minute < 10)
  {
    Serial.print("0");
  }
  Serial.print(minute, DEC);
  Serial.print(":");
  if (second < 10)
  {
    Serial.print("0");
  }
  Serial.print(second, DEC);
  Serial.print(" ");
  Serial.print(dayOfMonth, DEC);
  Serial.print("/");
  Serial.print(month, DEC);
  Serial.print("/");
  Serial.print(year, DEC);
  if (isSummerTime == 1) Serial.print(" Summertime"); else Serial.print(" Wintertime");
  Serial.print(" Day of week: ");
  switch (dayOfWeek) {
    case 1:
      Serial.println("Sunday");
      break;
    case 2:
      Serial.println("Monday");
      break;
    case 3:
      Serial.println("Tuesday");
      break;
    case 4:
      Serial.println("Wednesday");
      break;
    case 5:
      Serial.println("Thursday");
      break;
    case 6:
      Serial.println("Friday");
      break;
    case 7:
      Serial.println("Saturday");
      break;
  }
}

void turnOnNixie(byte whichNixie, byte figure) {
  digitalWrite(9, ((figure & 1) == 1));
  digitalWrite(6, ((figure & 2) == 2));
  digitalWrite(7, ((figure & 4) == 4));
  digitalWrite(8, ((figure & 8) == 8));

  digitalWrite(10, ((pos[whichNixie - 1] & 2) == 0));
  digitalWrite(11, ((pos[whichNixie - 1] & 4) == 0));
  digitalWrite(12, ((pos[whichNixie - 1] & 8) == 0));
  digitalWrite(13, ((pos[whichNixie - 1] & 16) == 0));

  if (clockMode == 1) {
    digitalWrite(4, 1);
    digitalWrite(3, secLed);
    digitalWrite(5, secLed);
  } else {
    digitalWrite(4, 0);
  }
}

void turnAllOff() {
  digitalWrite(6, HIGH);
  digitalWrite(7, HIGH);
  digitalWrite(8, HIGH);

  digitalWrite(10, HIGH);
  digitalWrite(11, HIGH);
  digitalWrite(12, HIGH);
  digitalWrite(13, HIGH);
}
