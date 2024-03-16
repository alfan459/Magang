//DS18B20
#include <OneWire.h>
#include <DallasTemperature.h>          // Library yang digunakan untuk DS18B20
#define DS 10                           // pin data ds18B20
OneWire oneWire(DS);
DallasTemperature sensors(&oneWire);
float temperatureds;                    // temperatureds => variabel pembacaan suhu air

//EC
#include "DFRobot_EC.h"                 // library yang digunakan untuk sensor EC
#include <EEPROM.h>
#define EC_PIN A2                       // pin data sensor EC terhubung ke A2
float ecValue;
int tds_val;                            // variable untuk pembacaan TDS
DFRobot_EC ec;
#define KVALUEADDR 0x0F

//PH
#include "DFRobot_PH.h"                 // library sensor pH
#define PH_PIN A0                       // pin data terhubung ke pin A0
float voltage, phValue;                 // variable voltage dan nilai pH
DFRobot_PH ph;

//bh1750
#include <BH1750.h>                     // library sensor Lux / cahaya
BH1750 lightMeter(0x23);                // untuk alamat SDA dan SCL
int lux;                                // variable pembacaan lux

//sht21
#include <SHT21.h>                      // library untuk sht21
SHT21 sht;                              
float Temp, Humidity;                   // varibale untuk suhu lingkungan dan kelembaban

//JSN
#include <NewPing.h>                            // library sensor JSN
#define trigPin 5                               // pin trigger di arduino di pin 5
#define echoPin 4                               // pin echo di pin 11
#define MAX_DISTANCE 200                        // max jarak pembacaan
NewPing sonar(trigPin, echoPin, MAX_DISTANCE);
int level_air, level_air2;

//LCD
#include <Wire.h>                               
#include <LiquidCrystal_I2C.h>                  // library i2c lcd
#define SDA_PIN 20                              // sda 20
#define SCL_PIN 21                              // scl 21    
LiquidCrystal_I2C lcd(0x27, 20, 4);             // alamat lcd 0x27

//RTC
#include <RtcDS3231.h>                          // library RTCDS3231
RtcDS3231<TwoWire> Rtc(Wire);
String datetime;                                // untuk tanggal
String timeonly;                                // untuk waktu
byte second, minute, hour, dayOfWeek, dayOfMonth, month, year;    // variable untuk RTC


String kirim, kondisi;
int pHrelaypin = 31, ECrelaypin = 29, pomparelay = 27, h;

//Timer
unsigned long lastTime = 0;
unsigned long minutes = 0;
unsigned long hours = 0;

void setup() {
  Serial.begin(9600);
  Serial3.begin(115200);

  // inisialisasi sensor sensor
  Wire.begin();         Wire.setClock(10000);
  sensors.begin();                      // DS18B20
  lightMeter.begin();                   // BH1750
  ec.begin();                           // EC
  for (byte i = 0; i < 8; i++) {
    EEPROM.write(KVALUEADDR + i, 0xFF);
  }
  ph.begin();                           // pH

  // Inisialisasi LCD
  lcd.init();
  lcd.backlight();
  
  //RTC ds3231
  Rtc.Begin();
  RtcDateTime compiled = RtcDateTime(__DATE__, __TIME__);
  if (!Rtc.IsDateTimeValid()) {
    if (Rtc.LastError() != 0) Serial.println(Rtc.LastError());
    else Rtc.SetDateTime(compiled);
  }
  if (!Rtc.GetIsRunning()) Rtc.SetIsRunning(true);
  RtcDateTime now = Rtc.GetDateTime();
  if (now < compiled) Rtc.SetDateTime(compiled);
  Rtc.Enable32kHzPin(false);
  Rtc.SetSquareWavePin(DS3231SquareWavePin_ModeNone);
  delay(250);
  
  // Relay
  pinMode(pHrelaypin, OUTPUT);  
  pinMode(ECrelaypin, OUTPUT);
  pinMode(pomparelay, OUTPUT);
  
  // Matikan semua relay, aktif jika LOW
  relay(1, 1, 1);
  
  waktuku();
  homeDisplay();
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

    // Sampling pertama: pH, suhu Air, dan suhu ruangan
    for (h = 0; h <= 20; h++) {
      PH();
      DSTEMP();
      sutemp();
      Serial.println(h);
      displayLcd();
    }

    // Sampling kedua: EC, suhu Air, dan suhu ruangan
    h = 0;
    for (h = 0; h <= 20; h++) {
      EC();
      DSTEMP();  
      sutemp();
      Serial.println(h);
      displayLcd();
    }
    h = 0;

    displayLcd();      
    kirim_data();       // Kirim data ke esp8266
    relay(1, 1, 1);     // Matikan semua relay
    minutes = 0;        // Reset nilai menit ke 0
    lcd.clear();        // Clear LCD untuk refresh
  }
}

//--------------------All functions-------------------//
void waktuku() {
  if (!Rtc.IsDateTimeValid()) {
    if (Rtc.LastError() != 0) {
      Serial.print("RTC communications error = ");
      Serial.println(Rtc.LastError());
    } 
    else Serial.println("RTC lost confidence in the DateTime!");
  }
  RtcDateTime now = Rtc.GetDateTime();
  RtcTemperature temp = Rtc.GetTemperature();
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
  Temp = sht.getTemperature();            // untuk mendapatkan suhu lingkungan
  Humidity = sht.getHumidity();           // untuk mendapatkan kelembaban lingkungan
  delay(85);
}

void jsn() {
  delay(50);                              // tunggu 50ms antar pings
  level_air = sonar.ping_cm();            // Pembacaan sensor dalam cm
  // level_air = 120 - level_air;
  // if (level_air >= 120 || level_air <= 40) level_air = level_air2;
  // else level_air2 = level_air;
  // Serial.print(level_air);
  // Serial.println(" cm");
}

void cahaya() {
  lux = lightMeter.readLightLevel();                // memulai pembacaan sensor
  delay(1000);        
}

void PH() {
  relay(0, 1, 0);                                   // urutan relay: ph : ec : pompa
  voltage = analogRead(PH_PIN) / 1024.0 * 5000;     // membaca nilai voltase
  phValue = ph.readPH(voltage, temperatureds);      // konversi voltage ke pH dengan kompensasi nilai suhu
  Serial.print("pH:");
  Serial.println(phValue);
}

void DSTEMP() {
  sensors.requestTemperatures();                    // Perintah untuk mendapatkan suhu
  temperatureds = sensors.getTempCByIndex(0);       // gunakan function ByIndex sebagai index pembacaan sensor
}


void EC() {
  relay(1, 0, 0);                                   // urutan relay: ph : ec : pompa
  voltage = analogRead(EC_PIN) / 1024.0 * 5000;     // membaca nilai voltase
  ecValue = ec.readEC(voltage, temperatureds);      // konversi voltage ke EC dengan kompensasi nilai suhu
  tds_val = ecValue * 538;
}


void kirim_data() {
  kirim = "";
  kirim += datetime;        kirim += "&";
  kirim += timeonly;        kirim += "x";
  kirim += lux;             kirim += "!";
  kirim += Temp;            kirim += "@";
  kirim += Humidity;        kirim += "#";
  kirim += phValue;         kirim += "$";
  kirim += tds_val;         kirim += "%";
  kirim += temperatureds;   kirim += "&";
  kirim += level_air;       kirim += "x";
  Serial3.println(kirim);
  Serial.println(kirim);
}

void homeDisplay() {
  lcd.setCursor(7, 0);      lcd.print("MONIK");
  lcd.setCursor(4, 1);      lcd.print("SMK N 1 BAWEN");
  lcd.setCursor(0, 2);      lcd.print(datetime);
  lcd.setCursor(11, 2);     lcd.print(timeonly);
  delay(2000);
  lcd.clear();
}

void displayLcd() {
  lcd.setCursor(0, 0);      lcd.print(datetime);
  lcd.setCursor(11, 0);     lcd.print(timeonly);
  lcd.setCursor(0, 1);      lcd.print("Lx: ");      lcd.print(lux);
  lcd.setCursor(11, 1);     lcd.print("GT:");       lcd.print(Temp);          lcd.print((char)223);  
  lcd.setCursor(0, 2);      lcd.print("pH:");       lcd.print(phValue);
  lcd.setCursor(0, 3);      lcd.print("EC:");       lcd.print(tds_val);
  lcd.setCursor(7, 3);      lcd.print("ppm");
  lcd.setCursor(10, 2);     lcd.print(" WT:");      lcd.print(temperatureds);
  lcd.setCursor(18, 2);     lcd.print((char)223);   lcd.print("C");           lcd.print(" ");
  lcd.setCursor(11, 3);     lcd.print("BL:");       lcd.print(level_air);
  lcd.setCursor(16, 3);     if (level_air < 100) lcd.print(" ");    
  lcd.setCursor(16, 3);     lcd.print("cm");        lcd.print(" ");
}

void relay(int pH_state, int EC_state, int pompa_state) {
  digitalWrite(pHrelaypin, pH_state);
  digitalWrite(ECrelaypin, EC_state);
  digitalWrite(pomparelay, pompa_state);
}
