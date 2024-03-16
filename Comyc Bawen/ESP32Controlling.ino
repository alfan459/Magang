#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "FS.h"
#include "SD.h"
#include "SPI.h"
// #include "esp_task_wdt.h"

#define LED 2

const char* ssid = "IoT_Bawen2";
const char* password = "WWW.omahiot.NET";
const char* serverName = "https://arridhofarm.com/api/getSelada2";

float dsl8_selada2, tds_selada2, ph_selada2, jsn_selada2, temp, hum, light, ph, ec, jsn, suhu_air, dsl8;
String date_server, time_server, accumulatedSensorData;
int httpCodeWeather, httpCodeControlling;

unsigned long lastTime = 0;
unsigned long timerDelay = 300000;

//controlling
const int relayPh = 32;
const int relayNutA = 25;
const int relayNutB = 33;
const int relayblower = 26;

bool pumpA = false;
bool pumpB = false;
bool pumpPh = false;
unsigned long prevMilPh = 0;  // a variable to store the previous time
unsigned long prevMilA = 0;   // a variable to store the previous time
unsigned long prevMilB = 0;   // a variable to store the previous time

// Function SDCard
void appendFile(fs::FS& fs, const char* path, const char* message) {
  Serial.printf("Appending to file: %s\n", path);

  File file = fs.open(path, FILE_APPEND);
  if (!file) {
    Serial.println("Failed to open file for appending");
    return;
  }
  if (file.print(message)) Serial.println("Message appended");
  else Serial.println("Append failed");
  file.close();
}

void readFile(fs::FS& fs, const char* path) {
  Serial.printf("Reading file: %s\n", path);

  File file = fs.open(path);
  if (!file) {
    Serial.println("Failed to open file for reading");
    return;
  }

  Serial.print("Read from file: ");
  while (file.available()) {
    Serial.write(file.read());
  }
  file.close();
}

void writeFile(fs::FS& fs, const char* path, const char* message) {
  Serial.printf("Writing file: %s\n", path);

  File file = fs.open(path, FILE_WRITE);
  if (!file) {
    Serial.println("Failed to open file for writing");
    return;
  }
  if (file.print(message)) {
    Serial.println("File written");
  } else {
    Serial.println("Write failed");
  }
  file.close();
}

void setup() {
  Serial.begin(115200);
  digitalWrite(15, LOW);
  pinMode(LED, OUTPUT);
  pinMode(relayPh, OUTPUT);
  pinMode(relayNutA, OUTPUT);
  pinMode(relayNutB, OUTPUT);
  pinMode(relayblower, OUTPUT);

  digitalWrite(relayNutA, HIGH);
  digitalWrite(relayNutB, HIGH);
  digitalWrite(relayPh, HIGH);

  // Mulai koneksi WiFi
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  digitalWrite(LED, HIGH);
  Serial.println("WiFi connected");

  if (!SD.begin()) {
    Serial.println("Card Mount Failed");
    return;
  }
  uint8_t cardType = SD.cardType();

  if (cardType == CARD_NONE) {
    Serial.println("No SD card attached");
    return;
  }

  Serial.print("SD Card Type: ");
  if (cardType == CARD_MMC) Serial.println("MMC");
  else if (cardType == CARD_SD) Serial.println("SDSC");
  else if (cardType == CARD_SDHC) Serial.println("SDHC");
  else Serial.println("UNKNOWN");

  uint64_t cardSize = SD.cardSize() / (1024 * 1024);
  Serial.printf("SD Card Size: %lluMB\n", cardSize);
}


void loop() {
  // Feed the watchdog timer to prevent it from resetting the ESP32
  // esp_task_wdt_reset();

  
  if((millis() - lastTime) > timerDelay){
    if (WiFi.status() == WL_CONNECTED) {
      // delay(60000);  // Tunggu 1 menit sebelum mengambil data lagi
      // Mengambil data dari API controlling
      getDataFromControllingAPI();
      

      accumulatedSensorData += "Date        : " + String(date_server) + "\n"
                              "Time         : " + String(time_server) + "\n"
                              "Light        : " + String(light) + "\n"
                              "pH           : " + String(ph) + "\n"
                              "JSN          : " + String(jsn) + "\n"
                              "Ec           : " + String(ec) + "\n"
                              "dsl8         : " + String(dsl8) + "\n"
                              "hum          : " + String(hum) + "\n"
                              "temp         : " + String(temp) + "\n"
                              "pH Max       : " + String(ph_selada2) + "\n"
                              "Tds Max      : " + String(tds_selada2) + "\n"
                              "Suhu Air max : " + String(dsl8_selada2) + "\n"
                              "TDS Max      : " + String(jsn_selada2) + "\n" + "\n";

      if (!accumulatedSensorData.isEmpty()) {
        // JSONVar jsonSensorData;  // Create a JSON object
        DynamicJsonDocument doc(1024);
        // Add each sensor's data to the JSON object as strings with two decimal places
        doc["date_server"] = date_server;
        doc["time_server"] = time_server;
        doc["light"] = light;
        doc["ph"] = ph;
        doc["light"] = light;
        doc["jsn"] = jsn;
        doc["ec"] = ec;
        doc["dsl8"] = dsl8;
        doc["hum"] = hum;
        doc["temp"] = temp;
        doc["ph_selada2"] = ph_selada2;
        doc["tds_selada2"] = tds_selada2;
        doc["dsl8_selada2"] = dsl8_selada2;
        doc["jsn_selada2"] = jsn_selada2;

        Serial.println(jsn_selada2);
        // Write the formatted JSON string to the file
        String jsonString;
        serializeJson(doc, jsonString);
        appendFile(SD, "/selada_json.json", jsonString.c_str());
        writeFile(SD, "/selada.txt", accumulatedSensorData.c_str());
        appendFile(SD, "/selada.txt", accumulatedSensorData.c_str());
        
        if(temp > 32.00) digitalWrite(relayblower, LOW);
        else digitalWrite(relayblower, HIGH);
      }
    } else {
      Serial.println("WiFi Disconnected");
    }
    
    if(httpCodeControlling == 200){
      controllingTDSA();
      digitalWrite(relayNutA, HIGH);  
      pumpA = false;                  
      Serial.println("Relay dimatikan");
      controllingTDSB();
      digitalWrite(relayNutB, HIGH);  
      pumpB = false;                  
      Serial.println("Relay dimatikan");
      controllingPH();
      digitalWrite(relayPh, HIGH);  
      pumpPh = false;               
      Serial.println("Relay dimatikan");
    }
    
    else{
      digitalWrite(relayNutA, HIGH);  
      pumpA = false;                  
      Serial.println("Relay NutA dimatikan");
      digitalWrite(relayNutB, HIGH);  
      pumpB = false;                  
      Serial.println("Relay NutB dimatikan");
      digitalWrite(relayPh, HIGH);  
      pumpPh = false;               
      Serial.println("Relay Ph dimatikan");
    }
    lastTime = millis();
    // ESP.restart();
  }
  if (WiFi.status() != WL_CONNECTED) {
    ESP.restart();
  }
}

void getDataFromControllingAPI() {
  HTTPClient http;
  // Mulai koneksi ke server Controlling
  http.begin(serverName);
  httpCodeControlling = http.GET();

  if (httpCodeControlling > 0) {
    // Jika koneksi berhasil, baca response
    String payload = http.getString();
    Serial.println("Controlling Response: " + payload);

    // Parsing JSON
    DynamicJsonDocument doc(1024);
    DeserializationError error = deserializeJson(doc, payload);
    if (error) {
      Serial.print("deserializeJson() failed: ");
      Serial.println(error.c_str());
      return;
    }

    // Parsing data JSON
    date_server = doc["date_server"].as<String>();
    time_server = doc["time_server"].as<String>();
    light = doc["light"].as<int>();
    ph = doc["ph"].as<float>();
    jsn = doc["jsn"].as<int>();
    ec = doc["ec"].as<int>();
    dsl8 = doc["dsl8"].as<int>();
    hum = doc["hum"].as<int>();
    temp = doc["temp"].as<float>();
    ph_selada2 = doc["ph_selada2"].as<float>();
    tds_selada2 = doc["tds_selada2"].as<int>();
    dsl8_selada2 = doc["dsl8_selada2"].as<int>();
    jsn_selada2 = doc["jsn_selada2"].as<int>();
    

  } else {
    Serial.print("Error on Controlling HTTP request: ");
    Serial.println(httpCodeControlling);
  }
  Serial.println(httpCodeControlling);
  // Putuskan koneksi
  http.end();
}

// Function Controlling
void controllingPH() {
  float intervalConPH = (((ph - ph_selada2) - 0.1887 + (0.0038 * jsn_selada2)) / 0.122) * 1000;  // interval in millisecondsconst
  if (intervalConPH < 0) intervalConPH = 0;
  // float intervalPhAkhir = intervalConPH + 5000;
  if (ph > ph_selada2 && !pumpPh) {
    // Aktifkan relay jika ambang terlampaui dan relay belum aktif
    digitalWrite(relayPh, LOW);  // Nyalakan relay
    pumpPh = true;               // Tandai bahwa relay aktif
    Serial.println("Relay Ph diaktifkan");
    delay(intervalConPH);
  } else {
    digitalWrite(relayPh, HIGH);  // Matikan relay
    pumpPh = false;               // Tandai bahwa relay dimatikan
    Serial.println("Relay Ph dimatikan");
  }
}

void controllingTDSA() {
  float intervalConTDSA = ((((tds_selada2 - ec) / 2) - 14.148 + (0.22564 * jsn_selada2)) / 3.312) * 1000;  // interval in milliseconds
  if (ec < tds_selada2 && !pumpA) {
    // Aktifkan relay jika ambang terlampaui dan relay belum aktif
    digitalWrite(relayNutA, LOW);  // Nyalakan relay
    pumpA = true;                  // Tandai bahwa relay aktif
    Serial.println("Relay NutA diaktifkan");
    delay(intervalConTDSA);
  } else {
    digitalWrite(relayNutA, HIGH);  // Matikan relay
    pumpA = false;                  // Tandai bahwa relay dimatikan
    Serial.println("Relay NutA dimatikan");
  }
}

void controllingTDSB() {
  float intervalConTDSB = ((((tds_selada2 - ec) / 2) - 9.019 + (0.1414 * jsn_selada2)) / 1.781) * 1000;  // interval in milliseconds
  if (ec < tds_selada2 && !pumpB) {
    // Aktifkan relay jika ambang terlampaui dan relay belum aktif
    digitalWrite(relayNutB, LOW);  // Nyalakan relay
    pumpB = true;                  // Tandai bahwa relay aktif
    Serial.println("Relay NutB diaktifkan");
    delay(intervalConTDSB);
  } else {
    digitalWrite(relayNutB, HIGH);  // Matikan relay
    pumpB = false;                  // Tandai bahwa relay dimatikan
    Serial.println("Relay NutB dimatikan");
  }
}
