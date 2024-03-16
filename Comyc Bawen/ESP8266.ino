// #include <SoftwareSerial.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>
// SoftwareSerial SerialWe (D6, D0);

const char* dbase = "http://arridhofarm.com/api/selada"; //database
const char* ssid = "IoT_Bawen2";
const char* pass = "WWW.omahiot.NET";

WiFiClient client;
String TDS, temp, PH, hum, lux, level_air, temperatureds, datetime, timeonly;
void wifi() {
  WiFi.begin(ssid, pass);
  Serial.print("Connecting...");
  while (WiFi.status() != WL_CONNECTED) { //Loop which makes a point every 500ms until the connection process has finished
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  Serial.print("Connected! IP-Address: ");
  Serial.println(WiFi.localIP());
}
void setup() {
  Serial.begin(115200);
  wifi();
}

void loop() {
  String msg = "";
  while (Serial.available() > 0) {
    datetime = Serial.readStringUntil('&');
    timeonly = Serial.readStringUntil('x');
    lux = Serial.readStringUntil('!');
    temp = Serial.readStringUntil('@');
    hum = Serial.readStringUntil('#');
    PH = Serial.readStringUntil('$');
    TDS = Serial.readStringUntil('%');
    temperatureds = Serial.readStringUntil('&');
    level_air = Serial.readStringUntil('x');
    Serial.println(datetime);
    Serial.println(timeonly);
    Serial.println(temp);
    Serial.println(TDS);
    Serial.println(PH);
    Serial.println(hum);
    Serial.println(lux);
    Serial.println(level_air);
    Serial.println("pemisah");
    if (checkContain(datetime) && checkContain(timeonly) && checkContain(lux) && checkContain(temp) && checkContain(hum) && checkContain(PH) && checkContain(TDS) && checkContain(temperatureds) && checkContain(level_air)) {
      Serial.println(datetime);
      Serial.println(timeonly);
      Serial.println(temp);
      Serial.println(TDS);
      Serial.println(PH);
      Serial.println(hum);
      Serial.println(lux);
      Serial.println(level_air);
      kirimDataKeServer();
      delay(1000);
    }
  }
   if (WiFi.status() != WL_CONNECTED) {
    wifi();
  }

// testing
//  lux =  "20141";
//  temp =  "43.2";
//  hum =  "25.31";
//  PH =  "7.8";
//  TDS =  "676";
//  temperatureds =  "38.0";
//  level_air =  "29";
//  shum1 =  "10";
//  shum2 =  "10";
//  shum3 = "10";
//  shum4 =  "10";
//  tempbox = "10";
//  Serial.println(datetime);
//  Serial.println(temp);
//  Serial.println(TDS);
//  Serial.println(PH);
//  Serial.println(hum);
//  Serial.println(lux);
//  Serial.println(level_air);
//  Serial.println(temperatureds);
//  kirimDataKeServer();
//  delay(1000);

}

void kirimDataKeServer() {
  //--------------start to transmit data sensor to server----------//
  if ((WiFi.status() == WL_CONNECTED))
  {
    Serial.print("[HTTP] send data to server begin...\n");
    delay(3000);
    WiFiClient client;
    HTTPClient http;
    // lux =  "20000";
    // temp =  "23.6";
    // hum =  "99.70";
    // PH =  "6.9";
    // TDS =  "708";
    // temperatureds =  "38.0";
    // level_air =  "20";
    String postStr = dbase;
    postStr += "?dgw=";
    // postStr += thn;
    postStr += datetime;
    // postStr += bln;
    // postStr += "-";
    // postStr += tgl;
    postStr += "&tgw=";
    // postStr += jam;
    postStr += timeonly;
    // postStr += mnt;
    // postStr += ":";
    // postStr += dtk;
    postStr += "&delay_gw_server=100";
    postStr += "&light=";
    postStr += lux;
    postStr += "&ph=";
    postStr += PH;
    postStr += "&jsn=";
    postStr += level_air;
    postStr += "&ec=";
    postStr += TDS;
    postStr += "&dsl8=";
    postStr += temperatureds;
    postStr += "&hum=";
    postStr += hum;
    postStr += "&temp=";
    postStr += temp;
    postStr += "&sn=2021100003";

    Serial.println(postStr);
    http.begin(client, postStr);
    int httpCodeSent = http.GET();
    Serial.printf("[HTTP] GET... code: %d\n", httpCodeSent);


    if (httpCodeSent == 200)
    { //kode 200
      String payload = http.getString();
      Serial.println("Data is sent successfully");
      Serial.println(payload);
    }
    else
    {
      Serial.printf("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCodeSent).c_str());
    }
    http.end();
    delay(3000);
  }
}

String splitString(String data, char separator, int index)
{ 
  int found = 0;
  int strIndex[] = { 0, -1 };
  int maxIndex = data.length() - 1;

  for (int i = 0; i <= maxIndex && found <= index; i++) {
    if (data.charAt(i) == separator || i == maxIndex) {
      found++;
      strIndex[0] = strIndex[1] + 1;
      strIndex[1] = (i == maxIndex) ? i + 1 : i;
    }
  }
  return found > index ? data.substring(strIndex[0], strIndex[1]) : "";
}

bool checkContain(String text) {
  bool status = true;
  if (text.indexOf("x") > 0 || text.indexOf("!") > 0 || text.indexOf("@") > 0 
  || text.indexOf("#") > 0 || text.indexOf("$") > 0 || text.indexOf("%") > 0 
  || text.indexOf("&") > 0 || text.indexOf("x") > 0) {
    status = false;
  }
  return status;
}
