//DS18B20
#include <OneWire.h>
#include <DallasTemperature.h>
#define DS 10
OneWire oneWire(DS);
DallasTemperature sensors(&oneWire);
float temperatureds;

//EC
#include "DFRobot_EC.h"
#include <EEPROM.h>

#define EC_PIN A2
float ecValue, temperature = 25;
int tds_val;
DFRobot_EC ec;
#define KVALUEADDR 0x0F

//PH
#include "DFRobot_PH.h"
#define PH_PIN A0
float voltage, phValue;
DFRobot_PH ph;

//bh1750
#include <BH1750.h>
BH1750 lightMeter(0x23);
int lux;

//sht21
#include <SHT21.h>  //SHT21 and LCD ic libraries
SHT21 sht;          //SHT and LCD entities
float Temp;         //Here we store the temperature and humidity values
float Humidity;

//JSN
#include <NewPing.h>
#define trigPin 5
#define echoPin 4
#define MAX_DISTANCE 200
NewPing sonar(trigPin, echoPin, MAX_DISTANCE);
int level_air;
int level_air2;

//LCD
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#define SDA_PIN 20  // alamat I2C LCD
#define SCL_PIN 21
LiquidCrystal_I2C lcd(0x27, 20, 4);  //inisialisasi lcd

byte second, minute, hour, dayOfWeek, dayOfMonth, month, year;

//RTC
#include <RtcDS3231.h>
RtcDS3231<TwoWire> Rtc(Wire);
String datetime;
String timeonly;

//Button
#include <Button.h>
int okKeypad = 4, upKeypad = 5, downKeypad = 6, cancelKeypad = 7, state;
char* cmd;
enum menu {
  awal,
  menuph7,
  menuph4,
  menuEC,
  calph7,
  calph4,
  calEC,
  notifph7,
  notifph4,
  notifEC,
};

//defining read button
#define ok !digitalRead(okKeypad)
#define cancel !digitalRead(cancelKeypad)
#define up !digitalRead(upKeypad)
#define down !digitalRead(downKeypad)

String kirim, kondisi;
int pHrelaypin = 31, ECrelaypin = 29, pomparelay = 27, h;

//Timer
long lastTime = 0;
long minutes = 0;
long hours = 0;

void setup() {
  Serial.begin(9600);
  Serial3.begin(115200);

  //Button
  //Set button as INPUT
  pinMode(okKeypad, INPUT_PULLUP);
  pinMode(cancelKeypad, INPUT_PULLUP);
  pinMode(downKeypad, INPUT_PULLUP);
  pinMode(upKeypad, INPUT_PULLUP);

  //LCD
  lcd.init();
  lcd.backlight();
  // lcd.begin();
  Wire.begin();
  lightMeter.begin();  //BH1750
  Wire.setClock(10000);

  //RTC ds3231
  Rtc.Begin();
  RtcDateTime compiled = RtcDateTime(__DATE__, __TIME__);
  printDateTime(compiled);
  Serial.println();
  if (!Rtc.IsDateTimeValid()) {
    if (Rtc.LastError() != 0) {
      Serial.print("RTC communications error = ");
      Serial.println(Rtc.LastError());
    } else {
      Serial.println("RTC lost confidence in the DateTime!");
      Rtc.SetDateTime(compiled);
    }
  }

  if (!Rtc.GetIsRunning()) {
    Serial.println("RTC was not actively running, starting now");
    Rtc.SetIsRunning(true);
  }

  RtcDateTime now = Rtc.GetDateTime();
  if (now < compiled) {
    Serial.println("RTC is older than compile time!  (Updating DateTime)");
    Rtc.SetDateTime(compiled);
  } else if (now > compiled) {
    Serial.println("RTC is newer than compile time. (this is expected)");
  } else if (now == compiled) {
    Serial.println("RTC is the same as compile time! (not expected but all is fine)");
  }
  Rtc.Enable32kHzPin(false);
  Rtc.SetSquareWavePin(DS3231SquareWavePin_ModeNone);
  delay(250);
  sensors.begin();  //DS18B20

  //EC
  ec.begin();
  for (byte i = 0; i < 8; i++) {
    EEPROM.write(KVALUEADDR + i, 0xFF);
  }

  //PH
  ph.begin();

  pinMode(pHrelaypin, OUTPUT);  //RELAY
  pinMode(ECrelaypin, OUTPUT);
  pinMode(pomparelay, OUTPUT);
  //turn off all relays
  relay(1, 1, 1);  // relay aktif low
  waktuku();
  homeDisplay();
  // randomSeed(analogRead(0));
  state = awal;
}

void loop() {

  waktuku();
  displayLcd();
  relay(1, 1, 1);
  if (millis() - lastTime > 60000) {
    minutes++;
    lastTime = millis();
  }

  cahaya();
  jsn();


  if (minutes == 3) {  //sampling 10 mnt sekali
    Serial.println("program jalan");

    h = 0;
    for (h = 0; h <= 20; h++) {
      // relay(1, 1, 0);
      PH();
      DSTEMP();  //sensor
      sutemp();
      Serial.println(h);
      displayLcd();
    }
    // digitalWrite(pHrelaypin, HIGH);
    h = 0;
    for (h = 0; h <= 20; h++) {
      // DSTEMP();  //sensor
      // digitalWrite(pomparelay, LOW);
      // digitalWrite(ECrelaypin, LOW);
      EC();
      DSTEMP();  //sensor
      sutemp();
      Serial.println(h);
      displayLcd();
    }
    h = 0;
    displayLcd();
    kirim_data();
    // delay(5000);
    relay(1, 1, 1);
    minutes = 0;  //sampling 10 mnt sekali
    lcd.clear();
  }
}

//--------------------All functions-------------------//
void waktuku() {
  if (!Rtc.IsDateTimeValid()) {
    if (Rtc.LastError() != 0) {
      // we have a communications error
      // see https://www.arduino.cc/en/Reference/WireEndTransmission for
      // what the number means
      Serial.print("RTC communications error = ");
      Serial.println(Rtc.LastError());
    } else {
      // Common Causes:
      //    1) the battery on the device is low or even missing and the power line was disconnected
      Serial.println("RTC lost confidence in the DateTime!");
    }
  }

  RtcDateTime now = Rtc.GetDateTime();
  printDateTime(now);
  Serial.println();
  //  Serial.println(datetime);

  RtcTemperature temp = Rtc.GetTemperature();
  temp.Print(Serial);
  // you may also get the temperature as a float and print it
  // Serial.print(temp.AsFloatDegC());
  Serial.println("C");
  //  delay(1000); // ten seconds
}

#define countof(a) (sizeof(a) / sizeof(a[0]))
void printDateTime(const RtcDateTime& dt) {
  char datestring[20];
  char timestring[20];

  snprintf_P(datestring,
             countof(datestring),
             PSTR("%04u-%02u-%02u"),
             dt.Year(),
             dt.Month(),
             dt.Day(),

             dt.Hour(),
             dt.Minute(),
             dt.Second());
  Serial.print(datestring);
  snprintf_P(timestring,
             countof(timestring),
             PSTR("%02u:%02u:%02u"),
             dt.Hour(),
             dt.Minute(),
             dt.Second());
  Serial.print(timestring);
  datetime = datestring;
  timeonly = timestring;
}
void sutemp() {
  Temp = sht.getTemperature();  //To get the temperature and humidity values and store them in their respective variable
  Humidity = sht.getHumidity();
  if (Temp < 1 || Humidity < 1) {
    Temp = 0;
    Humidity = 0;
  }
  // Serial.print("Temp: ");  //Print the temperature and humidity as "Temp: 23.18 C
  // Serial.print(Temp);      //                                      "Humi: 64.13 %"
  // Serial.print(" C");
  // Serial.print("Humi: ");
  // Serial.print(Humidity);
  // Serial.print(" %");
}
void jsn() {
  level_air = sonar.ping_cm();
  delay(1000);
  //  level_air = 120 - level_air;
  //  if (level_air >= 120 || level_air <= 40)
  //  {
  //    level_air = level_air2;
  //  }
  //  else
  //  {
  //    level_air2 = level_air;
  //  }
  Serial.print(level_air);
  Serial.println(" cm");
}

void cahaya() {
  lux = lightMeter.readLightLevel();
  if (lux < 1) {
    lux = 0;
  }
  Serial.print("Light: ");
  Serial.print(lux);
  Serial.println(" lx");
}

void PH() {
  //PH
  relay(0, 1, 0);
  //temperature = readTemperature();         // read your temperature sensor to execute temperature compensation
  voltage = analogRead(PH_PIN) / 1024.0 * 5000;  // read the voltage
  phValue = ph.readPH(voltage, temperatureds);   // convert voltage to pH with temperature compensation
  Serial.print("pH:");
  Serial.println(phValue);
}

void DSTEMP() {
  //DS18B20
  sensors.requestTemperatures();
  temperatureds = sensors.getTempCByIndex(0);
  if (temperatureds <= 0) {
    temperatureds = 0;
  }
  Serial.print("temperature: ");
  Serial.println(temperatureds);
}


void EC() {
  //EC
  relay(1, 0, 0);
  voltage = analogRead(EC_PIN) / 1024.0 * 5000;  // read the voltage
  //temperature = readTemperature();  // read your temperature sensor to execute temperature compensation
  ecValue = ec.readEC(voltage, temperatureds);  // convert voltage to EC with temperature compensation
  tds_val = ecValue * 538;
  Serial.print("EC:");
  Serial.print(ecValue);
  Serial.println("ms/cm");
}


void kirim_data() {
  kirim = "";
  kirim += datetime;
  kirim += "&";
  kirim += timeonly;
  kirim += "x";
  kirim += lux;
  kirim += "!";
  kirim += Temp;
  kirim += "@";
  kirim += Humidity;
  kirim += "#";
  kirim += phValue;
  kirim += "$";
  kirim += tds_val;
  kirim += "%";
  kirim += temperatureds;
  kirim += "&";
  kirim += level_air;
  kirim += "x";
  Serial3.println(kirim);
  Serial.println(kirim);
}

void homeDisplay() {
  lcd.setCursor(7, 0);
  lcd.print("MONIK");
  lcd.setCursor(4, 1);
  lcd.print("SMK N 1 BAWEN");
  lcd.setCursor(0, 2);
  lcd.print(datetime);
  lcd.setCursor(11, 2);
  lcd.print(timeonly);
  delay(2000);
  lcd.clear();
}

void displayLcd() {
  lcd.setCursor(0, 0);
  lcd.print(datetime);
  lcd.setCursor(11, 0);
  lcd.print(timeonly);
  lcd.setCursor(0, 1);
  lcd.print("Lx:");
  lcd.setCursor(3,1);
  lcd.print(lux);
  lcd.setCursor(11, 1);
  lcd.print("GT:");
  lcd.print(Temp);
  lcd.print((char)223);
  lcd.print("C");
  lcd.setCursor(0, 2);
  lcd.print("pH:");
  lcd.print(phValue);
  lcd.setCursor(0, 3);
  lcd.print("EC:");
  lcd.print(tds_val);
  lcd.setCursor(7, 3);
  lcd.print("ppm");
  lcd.setCursor(10, 2);
  lcd.print(" WT:");
  lcd.print(temperatureds);
  lcd.setCursor(18, 2);
  lcd.print((char)223);
  lcd.print("C");
  lcd.print(" ");
  lcd.setCursor(11, 3);
  lcd.print("BL:");
  lcd.print(level_air);
  lcd.setCursor(16, 3);
  if (level_air < 100) lcd.print(" ");
  lcd.setCursor(16, 3);
  lcd.print("cm");
  lcd.print(" ");
}

void relay(int pH_state, int EC_state, int pompa_state) {
  digitalWrite(pHrelaypin, pH_state);
  digitalWrite(ECrelaypin, EC_state);
  digitalWrite(pomparelay, pompa_state);
}
