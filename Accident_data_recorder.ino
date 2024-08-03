#include <Wire.h>
#include <HX710.h>
#include <stdio.h>
#include <RTClib.h>
#include <I2Cdev.h>
#include <DS3231.h>
#include <WiFiUdp.h>
#include <MPU6050.h>
#include <NTPClient.h>
#include <ArduinoJson.h>
#include <ESP8266WiFi.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>

#define WIFI_SSID ""
#define WIFI_PASSWORD ""
#define BOT_TOKEN ""
#define CHAT_ID ""

DS3231 myRTC;
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "in.pool.ntp.org", 19800);
HX710 ps;
RTC_DS3231 rtc;
MPU6050 mpu(0x69);
Adafruit_SSD1306 display(128, 64, &Wire, -1);  
X509List cert(TELEGRAM_CERTIFICATE_ROOT);
WiFiClientSecure secured_client;
UniversalTelegramBot bot(BOT_TOKEN, secured_client);

float ax, ay, az;    
float gx, gy, gz;    
uint32_t tPressure;  
int8_t sAngle; 
int temp;            
uint8_t count = 0, prev_count = count;
bool flag1 = false, flag2 = false;

String date() {
  DateTime now = rtc.now();
  return (String)now.day() + "/" + (String)now.month() + "/" + (String)now.year();
}

String time() {
  DateTime now = rtc.now();
  return (String)now.hour() + ":" + (String)now.minute() + ":" + (String)now.second();
}

void ICACHE_RAM_ATTR button() {
  flag1 = true;
}

void ICACHE_RAM_ATTR event() {
  prev_count = count;
  flag2 = true;
}

void print() {
  switch (count) {
    case 0:
      display.clearDisplay();
      display.setTextColor(SSD1306_WHITE);
      display.setTextSize(2);
      display.setCursor(10, 15);
      display.println(time());
      display.setCursor(10, 35);
      display.println(date());
      display.display();
      delay(100);
      break;

    case 1:
      display.clearDisplay();
      display.setTextColor(SSD1306_WHITE);
      display.setTextSize(1);
      display.setCursor(0, 0);
      display.println("Acceleration");
      display.setTextSize(2);
      display.setCursor(0, 15);
      display.println("x : " + (String)ax + "\ny : " + (String)ay + "\nz : " + (String)az);
      display.display();
      delay(100);
      break;

    case 2:
      display.clearDisplay();
      display.setTextColor(SSD1306_WHITE);
      display.setTextSize(1);
      display.setCursor(0, 0);
      display.println("Gyroscope");
      display.setTextSize(2);
      display.setCursor(0, 15);
      display.println("x : " + (String)gx + "\ny : " + (String)gy + "\nz : " + (String)gz);
      display.display();
      delay(100);
      break;

    case 3:
      display.clearDisplay();
      display.setTextColor(SSD1306_WHITE);
      display.setTextSize(1);
      display.setCursor(0, 0);
      display.println("Temperature");
      display.setTextSize(3);
      display.setCursor(45, 30);
      display.println(temp);
      display.display();
      delay(100);
      break;

    case 4:
      display.clearDisplay();
      display.setTextColor(SSD1306_WHITE);
      display.setTextSize(1);
      display.setCursor(0, 0);
      display.println("Tyre Pressure");
      display.setTextSize(3);
      display.setCursor(45, 30);
      display.println(tPressure);
      display.display();
      delay(100);
      break;

    case 5:
      display.clearDisplay();
      display.setTextColor(SSD1306_WHITE);
      display.setTextSize(1);
      display.setCursor(0, 0);
      display.println("Steering Angle");
      display.setTextSize(3);
      display.setCursor(40, 30);
      display.println(sAngle);
      display.display();
      delay(100);
      break;

    default:
      count = prev_count;
      display.clearDisplay();
      display.setTextColor(SSD1306_WHITE);
      display.setTextSize(2);
      display.setCursor(0, 15);
      display.println("Accident");
      display.setCursor(0, 35);
      display.println("Warning!!!");
      display.display();
      bot.sendMessage(CHAT_ID, "Accident Warning !!!", "");
      delay(100);
      break;
  }
}

void setup() {
  Serial.begin(115200);
  Wire.begin();

  //OLED Initialization
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    while (1);
  }

  //HX710b initialization
  ps.initialize(D4, D3);  

  //MPU6050 Initialization
  mpu.initialize();
  if (!mpu.testConnection()) {
    while (1);
  }

  pinMode(D5, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(D5), event, FALLING);

  pinMode(D6, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(D6), button, RISING);

  //Wifi Initialization
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  secured_client.setTrustAnchors(&cert);  
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor(0, 20);
  display.println("Waiting for WIFI");
  display.setCursor(0, 35);
  display.println("connection .....");
  display.display();
  while (WiFi.status() != WL_CONNECTED) delay(10);
  configTime(0, 0, "pool.ntp.org");  // get UTC time via NTP
  time_t now = time(nullptr);
  while (now < 24 * 3600) {
    delay(10);
    now = time(nullptr);
  }

  //RTC Initialization
  timeClient.update();
  myRTC.setSecond(timeClient.getSeconds());
  myRTC.setMinute(timeClient.getMinutes());
  myRTC.setHour(timeClient.getHours());
  myRTC.setDoW(timeClient.getDay());
  time_t epochTime = timeClient.getEpochTime();
  struct tm *ptm = gmtime ((time_t *)&epochTime); 
  myRTC.setDate(ptm->tm_mday);
  myRTC.setMonth(ptm->tm_mon);
  myRTC.setYear((ptm->tm_year+1900)%100);
  if (!rtc.begin()) {
    while (1);
  }
}

void loop() {
  if (flag1) {
    count = (count + 1) % 6;
    flag1 = false;
  } else if (flag2) {
    prev_count = count;
    count = 6;
    flag2 = false;
  }
  print();
  ax = (float)mpu.getAccelerationX() / 16384.0;
  ay = (float)mpu.getAccelerationY() / 16384.0;
  az = (float)mpu.getAccelerationZ() / 16384.0;
  gx = (float)mpu.getRotationX() / 131.0;
  gy = (float)mpu.getRotationY() / 131.0;
  gz = (float)mpu.getRotationZ() / 131.0;
  while (!ps.isReady());
  ps.readAndSelectNextData(HX710_DIFFERENTIAL_INPUT_40HZ);
  tPressure = map(ps.getLastDifferentialInput(), 0, 8388607, 0, 40);
  temp = rtc.getTemperature();
  sAngle = map(analogRead(A0), 0, 1023, -100, 100);
  DateTime now = rtc.now();
  char buffer[50];
  sprintf(buffer, "%u/%u/%u\t%u:%u:%u\t%.2f\t%.2f\t%.2f\t%.2f\t%.2f\t%.2f\t%d\t%u\t%d", now.day(), now.month(), now.year(), now.hour(), now.minute(), now.second(), ax, ay, az, gx, gy, gz, temp, tPressure, sAngle);
  Serial.println(buffer);
}
