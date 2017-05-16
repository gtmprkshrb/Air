//All Libraries required here are included in the GitHub folder download. Put All ".cpp" and ".h" files in My Computer > Documents > Arduino > Libraries
 
#include <Wire.h>  // This library is already present in Arduino IDE
#include <DHT.h> //
#include <ESP8266WiFi.h>
#include <LiquidCrystal_I2C.h> 
LiquidCrystal_I2C lcd(0x3F, 16, 2);

int measurePin = A0;
int ledPower = D0;

int samplingTime = 280;
int deltaTime = 40;
int sleepTime = 9680;

float voMeasured=0.0;
float calcVoltage=0.0;
float dustDensity=0.0;

String apiKey = "SE33A3I8EMGV0QWC";
const char* ssid = "ReapBenefit_Act_Upstairs";
const char* password = "solvesmalldentbig";
const char* server = "139.59.43.105";
#define DHTPIN D4 // SO CHANGE THE DHT11/22 SENSOR TO THIS PIN OF THE NODEMCU

DHT dht(DHTPIN, DHT22,15); //CHANGE DHT11 TO DHT22 IF YOU ARE USING DHT22
WiFiClient client;

void setup() {
  Serial.begin(115200);
  Wire.begin();
  pinMode(ledPower, OUTPUT);
  delay(1000);
  dht.begin();
  lcd.init();   // initializing the LCD
  lcd.backlight(); // Enable or Turn On the backlight
  WiFi.begin(ssid, password);

    Serial.println();
    Serial.println();
    Serial.print("Connecting to ");
    Serial.println(ssid);
    
    WiFi.begin(ssid, password);
    delay(3000);
    WiFi.mode(WIFI_STA);
    while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    }
    Serial.println("");
    Serial.println("WiFi connected");
}

void loop() {
  delay(15000);

  digitalWrite(ledPower, LOW); // power on the LED
  delayMicroseconds(samplingTime);

  voMeasured = analogRead(measurePin); // read the dust value

  delayMicroseconds(deltaTime);
  digitalWrite(ledPower, HIGH); // turn the LED off
  delayMicroseconds(sleepTime);

  // 0 - 3.3V mapped to 0 - 1023 integer values
  // recover voltage
  calcVoltage = voMeasured * (5.0 / 1024);

  // linear eqaution taken from http://www.howmuchsnow.com/arduino/airquality/
  // Chris Nafis (c) 2012
//  dustDensity = (0.17 * calcVoltage - 0.1) * 1000;
  dustDensity = (0.17 * calcVoltage) * 1000;

  Serial.print("Raw: ");
  Serial.print(voMeasured);

  Serial.print("; - Voltage: ");
  Serial.print(calcVoltage);

  Serial.print("; - Dust Density [ug/m3]: ");
  Serial.print(dustDensity);
  Serial.println(";");

  delay(1000);
      
      float h = dht.readHumidity();
      float t = dht.readTemperature();

      lcd.setCursor(0, 0);
      Serial.print("T ");
      Serial.print(t);
      lcd.print("T ");
      lcd.print(t);
      lcd.print("C H ");
      lcd.print(h);
      //lcd.setCursor(0, 1);
      Serial.println(" Humidity: ");
      Serial.print(h);
      //lcd.print("Humidity: ");
      //lcd.print(h);
      //lcd.print(" %");
      lcd.setCursor(0, 1);
      lcd.print("Dust PM2.5: ");
      lcd.print(dustDensity);
      //lcd.print(" mg/m3");
      Serial.println("% send to Thingspeak");
      
      
      if (isnan(h) || isnan(t)) {
      Serial.println("Failed to read from DHT sensor!");
      //return;
      }
      
      if (client.connect(server,3000)) { // "184.106.153.149" or api.thingspeak.com
      
      String postStr = apiKey;
      postStr +="&field1=";
      postStr += String(t);
      postStr +="&field2=";
      postStr += String(h);
      postStr +="&field3=";
      postStr += String(dustDensity);
      postStr += "\r\n\r\n";
      
      client.print("POST /update HTTP/1.1\n");
      client.print("Host: api.thingspeak.com\n");
      client.print("Connection: close\n");
      client.print("X-THINGSPEAKAPIKEY: "+apiKey+"\n");
      client.print("Content-Type: application/x-www-form-urlencoded\n");
      client.print("Content-Length: ");
      client.print(postStr.length());
      client.print("\n\n");
      client.print(postStr);
      
      
      }
      client.stop();
      
      Serial.println("Waiting…");
      // thingspeak needs minimum 15 sec delay between updates
      //delay(300000);

}

//
float sumUp(float * n, int samples) {
  float res = 0.0;
  int i;
  for (i = -1; i <= samples; i++) {
    res+=n[i];
    n[i] = 0.0;
  }
  
  float r = res / samples;
  return r;
}
