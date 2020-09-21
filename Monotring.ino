#include <Wire.h>
#include <WiFi.h>
#include <LiquidCrystal_I2C.h>
#include <MQ2.h>
#include <HTTPClient.h>
#include <MD_Parola.h>
#include <MD_MAX72xx.h>
#include <SPI.h>
#include "DHT.h"
#include "esp_wpa2.h"
#define TOUTCH_PIN T8
#define DHTPIN 4
#define DHTTYPE DHT22

const uint16_t WAIT_TIME = 1000;
#define SPEED_TIME  1000
#define PAUSE_TIME  100
#define HARDWARE_TYPE MD_MAX72XX::FC16_HW
#define MAX_DEVICES 4
#define CLK_PIN   14
#define DATA_PIN  12
#define CS_PIN    15
MD_Parola P = MD_Parola(HARDWARE_TYPE, DATA_PIN, CLK_PIN, CS_PIN, MAX_DEVICES);
#define MAX_MESG  20
unsigned long timeold;
char szMesg[MAX_MESG + 1] = "";
uint8_t degC[] = { 6, 3, 3, 56, 68, 68, 68 }; // Deg C


//-------------- if eduroam use this -----------------
//#define EAP_IDENTITY "ibrahim@students.undip.ac.id"
//#define EAP_PASSWORD "12QWqw=+"
//const char* ssid = "eduroam";
//------------ if normal wifi use this ---------------
const char* ssid = "BAYAR WIFI";
const char* pass = "bayar50k";

const int mqPin = 35;
const int butPin = 32;
int co, button, out;
int touch_value = 100;
double h, t;
String k;
DHT dht(DHTPIN, DHTTYPE);
WiFiClient client;
MQ2 mq2(mqPin);

//======================================================================================
const char* serverName = "http://deepgrow.id/post-esp-data.php";
String apiKeyValue = "tPmAT5Ab3j7F9";

String board = "ESP32";
String sensorLocation = "Office";
//======================================================================================

String thingSpeakAddress = "http://api.thingspeak.com/update?";
String writeAPIKey;
String tsfield1Name;
String request_string;
HTTPClient http;
long interval = 60000;
unsigned long delay1, delay2, delay3, curMill;

// Set the LCD address to 0x27 for a 16 chars and 2 line display
LiquidCrystal_I2C lcd(0x27, 16, 2);

void setup()
{
  P.begin(1);
  P.setInvert(false);
  P.setZone(0, 0, MAX_DEVICES);
  byte error = 0;
  timeold = 0;
  // initialize the LCD
  lcd.begin();
  dht.begin();
  mq2.begin();
  lcd.backlight();
  P.displayZoneText(0, szMesg, PA_CENTER, SPEED_TIME, PAUSE_TIME, PA_FADE, PA_FADE);
  P.addChar('$', degC);

  //------------ if normal wifi use this ---------------
  WiFi.begin(ssid, pass);
  while ((!(WiFi.status() == WL_CONNECTED))) {
    delay(300);
  }
  //----------------------------------------------------

  //-------------- if eduroam use this -----------------
  //  WiFi.disconnect(true);  //disconnect from wifi to set new wifi connection
  //  error += esp_wifi_sta_wpa2_ent_set_identity((uint8_t *)EAP_IDENTITY, strlen(EAP_IDENTITY));
  //  error += esp_wifi_sta_wpa2_ent_set_username((uint8_t *)EAP_IDENTITY, strlen(EAP_IDENTITY));
  //  error += esp_wifi_sta_wpa2_ent_set_password((uint8_t *)EAP_PASSWORD, strlen(EAP_PASSWORD));
  //  if (error != 0) {
  //    lcd.setCursor(0, 0);
  //    lcd.print("SORRY");
  //  }
  //  WiFi.enableSTA(true);
  //  esp_wpa2_config_t config = WPA2_CONFIG_INIT_DEFAULT();
  //  if (esp_wifi_sta_wpa2_ent_enable(&config) != ESP_OK) {
  //    lcd.clear();
  //    lcd.setCursor(0, 0);
  //    lcd.print("SORRY BRO");
  //  }
  //  WiFi.begin(ssid);
  //  WiFi.setHostname("RandomHostname");
  //  while (WiFi.status() != WL_CONNECTED) {
  //    delay(500);
  //    lcd.setCursor(0, 0);
  //    lcd.print("CONNECTING..");
  //  }
  //----------------------------------------------------

  lcd.setCursor(0, 0);
  lcd.print("   MONITORING");
  lcd.setCursor(0, 1);
  lcd.print(WiFi.localIP());
  delay(2000);
  lcd.clear();
}

void loop()
{
  static uint8_t  display = 0;
  curMill = millis();
  //  static bool flasher = false;  // seconds passing flasher
  float* values = mq2.read(false);
  touch_value = touchRead(TOUTCH_PIN);
  h = dht.readHumidity();
  t = dht.readTemperature();
  co = mq2.readCO();
  if (isnan(h) || isnan(t)) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(F("Failed sensor!"));
    return;
  }
  if (t > 40 || co > 1000 || touch_value < 25) {
    k = "DANGER";
    out = 1;
  }
  else {
    k = "SAFE";
    out = 0;
  }
  LCD();

  P.displayAnimate();

  if (P.getZoneStatus(0))
  {
    switch (display)
    {
      case 0: //Temp:
        P.setTextEffect(0, PA_PRINT, PA_PRINT);
        display++;
        strcpy(szMesg, "Temp: ");
        break;

      case 1: // Temperature deg C
        P.setTextEffect(0, PA_PRINT, PA_PRINT);
        display++;
        dtostrf(t, 3, 1, szMesg);
        strcat(szMesg, " $");
        break;

      case 2: //Humidity:
        P.setTextEffect(0, PA_PRINT, PA_PRINT);
        display++;
        strcpy(szMesg, "Hum: ");
        break;

      case 3: // Relative Humidity
        P.setTextEffect(0, PA_PRINT, PA_PRINT);
        display++;
        dtostrf(h, 3, 1, szMesg);
        strcat(szMesg, " %");
        break;

      case 4: //Output Monitoring
        P.setTextEffect(0, PA_PRINT, PA_PRINT);
        display++;
        strcpy(szMesg, "CO: ");
        break;

      case 5: //Output Monitoring
        P.setTextEffect(0, PA_PRINT, PA_PRINT);
        display = 0;
        dtostrf(co, 3, 0, szMesg);
        strcat(szMesg, " ppm");
        break;

//      default:  // Calendar
//        P.setTextEffect(0, PA_PRINT, PA_PRINT);
//        display = 0;
//        strcpy(szMesg, " WAIT ");
//        break;
    }
    P.displayReset(0);
  }

  //  Matrix();

  if (curMill - delay1 >= interval) {
    delay1 = curMill;
//    kirim();
    postMysql();
  }
}

void postMysql(){
    if(WiFi.status()== WL_CONNECTED){
    HTTPClient http;
    
    // Your Domain name with URL path or IP address with path
    http.begin(serverName);
    
    // Specify content-type header
    http.addHeader("Content-Type", "application/x-www-form-urlencoded");
    
    // Prepare your HTTP POST request data
    String httpRequestData = "api_key=" + apiKeyValue + "&board=" + board
                          + "&location=" + sensorLocation + "&value1=" + String(t)
                          + "&value2=" + String(h) + "&value3=" + String(co)
                          + "&value4=" + String(out) + "&value5=" + String(touch_value) + "";
    Serial.print("httpRequestData: ");
    Serial.println(httpRequestData);
    
    // You can comment the httpRequestData variable above
    // then, use the httpRequestData variable below (for testing purposes without the BME280 sensor)
    //String httpRequestData = "api_key=tPmAT5Ab3j7F9&sensor=BME280&location=Office&value1=24.75&value2=49.54&value3=1005.14";

    // Send HTTP POST request
    int httpResponseCode = http.POST(httpRequestData);
     
    // If you need an HTTP request with a content type: text/plain
    //http.addHeader("Content-Type", "text/plain");
    //int httpResponseCode = http.POST("Hello, World!");
    
    // If you need an HTTP request with a content type: application/json, use the following:
    //http.addHeader("Content-Type", "application/json");
    //int httpResponseCode = http.POST("{\"value1\":\"19\",\"value2\":\"67\",\"value3\":\"78\"}");
        
    if (httpResponseCode>0) {
      Serial.print("HTTP Response code: ");
      Serial.println(httpResponseCode);
    }
    else {
      Serial.print("Error code: ");
      Serial.println(httpResponseCode);
    }
    // Free resources
    http.end();
  }
  else {
    Serial.println("WiFi Disconnected");
  }
  //Send an HTTP POST request every 30 seconds
  //delay(30000);  
}
void kirim() {
  if (client.connect("api.thingspeak.com", 80)) {
    writeAPIKey = "key=W5RC4KPVOI54G2A2";
    tsfield1Name = "&field1=10";
    request_string = thingSpeakAddress;
    request_string += "key=";
    request_string += "W5RC4KPVOI54G2A2";
    request_string += "&";
    request_string += "field1";
    request_string += "=";
    request_string += t;
    request_string += "&";
    request_string += "field2";
    request_string += "=";
    request_string += h;
    request_string += "&";
    request_string += "field3";
    request_string += "=";
    request_string += co;
    request_string += "&";
    request_string += "field4";
    request_string += "=";
    request_string += out;
    request_string += "&";
    request_string += "field5";
    request_string += "=";
    request_string += touch_value;
    http.begin(request_string);
    http.GET();
    http.end();
  }
}

void LCD() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("T:");
  lcd.setCursor(2, 0);
  lcd.print(t);
  lcd.setCursor(6, 0);
  lcd.print("C");
  lcd.setCursor(9, 0);
  lcd.print("H:");
  lcd.setCursor(11, 0);
  lcd.print(h);
  lcd.setCursor(15, 0);
  lcd.print("%");
  lcd.setCursor(0, 1);
  lcd.print("K:");
  lcd.setCursor(2, 1);
  lcd.print(k);
  lcd.setCursor(9, 1);
  lcd.print(co);
  lcd.setCursor(13, 1);
  lcd.print("ppm");
}

