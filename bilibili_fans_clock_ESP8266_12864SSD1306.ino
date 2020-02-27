/* 代码修改：UP：Hans叫泽涵  bilibili UID:15481541
 *  感谢UP:会飞的阿卡林  bilibili UID:751219
 *  感谢UP:黄埔皮校大校长  bilibili UID:403928979
 *  感谢UP:F_KUN  bilibili UID:8515147
 *  感谢UP:啊泰_  bilibili UID:23106193
 *  感谢UP:硬核拆解  bilibili UID:427494870
 *  
 * 本代码适用于ESP8266 NodeMCU + 12864 SSD1306 OLED显示屏
 * 
 * 7pin SPI引脚，正面看，从左到右依次为GND、VCC、D0、D1、RES、DC、CS
 *    ESP8266 ---  OLED
 *      3V    ---  VCC
 *      G     ---  GND
 *      D7    ---  D1
 *      D5    ---  D0
 *      D2orD8---  CS
 *      D1    ---  DC
 *      RST   ---  RES
 *      
 * 4pin IIC引脚，正面看，从左到右依次为GND、VCC、SCL、SDA
 *      ESP8266  ---  OLED
 *      3.3V     ---  VCC
 *      G (GND)  ---  GND
 *      D1(GPIO5)---  SCL
 *      D2(GPIO4)---  SDA
 */
 
#include <TimeLib.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <SPI.h>
#include <U8g2lib.h>
#include <Wire.h>

U8G2_SSD1306_128X64_NONAME_F_4W_HW_SPI u8g2(U8G2_R0, /* cs=*/4, /* dc=*/5, /* reset=*/3);

//---------------修改此处""内的信息---------------------------------------------
const char ssid[] = "Hans";                       //WiFi名
const char pass[] = "your password";                   //WiFi密码
String biliuid = "15481541";         //bilibili UID
//-----------------------------------------------------------------------------

static const char ntpServerName[] = "ntp1.aliyun.com"; //NTP服务器，阿里云
const int timeZone = 8;                                //时区，北京时间为+8

const unsigned long HTTP_TIMEOUT = 5000;
WiFiClient client;
HTTPClient http;
WiFiUDP Udp;
unsigned int localPort = 8888; // 用于侦听UDP数据包的本地端口

time_t getNtpTime();
void sendNTPpacket(IPAddress& address);
void oledClockDisplay();
void sendCommand(int command, int value);
void initdisplay();

bool getJson();
bool parseJson(String json);
void parseJson1(String json);

boolean isNTPConnected = false;

const unsigned char xing[] U8X8_PROGMEM = {
  0x00, 0x00, 0xF8, 0x0F, 0x08, 0x08, 0xF8, 0x0F, 0x08, 0x08, 0xF8, 0x0F, 0x80, 0x00, 0x88, 0x00,
  0xF8, 0x1F, 0x84, 0x00, 0x82, 0x00, 0xF8, 0x0F, 0x80, 0x00, 0x80, 0x00, 0xFE, 0x3F, 0x00, 0x00
};  /*星*/
const unsigned char liu[] U8X8_PROGMEM = { 
  0x40, 0x00, 0x80, 0x00, 0x00, 0x01, 0x00, 0x01, 0x00, 0x00, 0xFF, 0x7F, 0x00, 0x00, 0x00, 0x00,
  0x20, 0x02, 0x20, 0x04, 0x10, 0x08, 0x10, 0x10, 0x08, 0x10, 0x04, 0x20, 0x02, 0x20, 0x00, 0x00
};  /*六*/


//24*24小电视
const unsigned char bilibilitv_24u[] U8X8_PROGMEM = {0x00, 0x00, 0x02, 0x00, 0x00, 0x03, 0x30, 0x00, 0x01, 0xe0, 0x80, 0x01,
                                                     0x80, 0xc3, 0x00, 0x00, 0xef, 0x00, 0xff, 0xff, 0xff, 0x03, 0x00, 0xc0, 0xf9, 0xff, 0xdf, 0x09, 0x00, 0xd0, 0x09, 0x00, 0xd0, 0x89, 0xc1,
                                                     0xd1, 0xe9, 0x81, 0xd3, 0x69, 0x00, 0xd6, 0x09, 0x91, 0xd0, 0x09, 0xdb, 0xd0, 0x09, 0x7e, 0xd0, 0x0d, 0x00, 0xd0, 0x4d, 0x89, 0xdb, 0xfb,
                                                     0xff, 0xdf, 0x03, 0x00, 0xc0, 0xff, 0xff, 0xff, 0x78, 0x00, 0x1e, 0x30, 0x00, 0x0c
                                                    };


String response;
String responseview;
int follower = 0;
int view = 0;
const int slaveSelect = 5;
const int scanLimit = 7;

int times = 0;


void setup()
{
  Serial.begin(115200);
  while (!Serial)
    continue;
  Serial.println("bilibili fans NTP Clock version 1.0");
  Serial.println("by Hans");
  initdisplay();
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_unifont_t_chinese2);
  u8g2.setCursor(0, 14);
  u8g2.print("Waiting for WiFi");
  u8g2.setCursor(0, 30);
  u8g2.print("connection...");
  u8g2.setCursor(0, 47);
  u8g2.print("By Hans bilibili");
  u8g2.setCursor(0, 64);
  u8g2.print("fans Clock v1.0");
  u8g2.sendBuffer();


  Serial.print("Connecting WiFi...");
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, pass);
  while (WiFi.status() != WL_CONNECTED){
      delay(500);
      Serial.print(".");
  }
  
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  Serial.println("Starting UDP");
  Udp.begin(localPort);
  Serial.print("Local port: ");
  Serial.println(Udp.localPort());
  Serial.println("waiting for sync");
  setSyncProvider(getNtpTime);
  setSyncInterval(300); //每300秒同步一次时间
  isNTPConnected = true;
}


void loop()
{
  if (times == 0) {
    if (timeStatus() != timeNotSet)
    {
        oledClockDisplay();
    }
  }

  if (times == 5) {
    if (WiFi.status() == WL_CONNECTED){
        if (getJson()){
            if (parseJson(response)){
                parseJson1(responseview);
                  u8g2.clearBuffer();
                  u8g2.setFont(u8g2_font_unifont_t_chinese2);
                  u8g2.drawXBMP( 52 , 0 , 24 , 24 , bilibilitv_24u );     //根据你的图片尺寸修改
                  u8g2.setCursor(0, 40);
                  u8g2.print("fans:");
                  u8g2.setFont(u8g2_font_unifont_t_chinese2);
                  u8g2.setCursor(45, 40);
                  u8g2.println(follower);
                  u8g2.setFont(u8g2_font_unifont_t_chinese2);
                  u8g2.setCursor(0, 60);
                  u8g2.print("view:");
                  u8g2.setFont(u8g2_font_unifont_t_chinese2);
                  u8g2.setCursor(45, 60);
                  u8g2.println(view);
                  u8g2.sendBuffer();
            }
         }
         delay(1000);
    }
  }
  
  times += 1;
  if (times >= 10) {
    times = 0;
  }
  delay(1000);
}


bool getJson()
{
    bool r = false;
    http.setTimeout(HTTP_TIMEOUT);

    //followe
    http.begin("http://api.bilibili.com/x/relation/stat?vmid=" + biliuid);
    int httpCode = http.GET();
    if (httpCode > 0){
        if (httpCode == HTTP_CODE_OK){
            response = http.getString();
            Serial.println(response);
            r = true;
        }
    }else{
        Serial.printf("[HTTP] GET JSON failed, error: %s\n", http.errorToString(httpCode).c_str());
        r = false;
    }
    http.end();
    //return r;

    //view
    http.begin("http://api.bilibili.com/x/space/upstat?mid=" + biliuid);
    int httpCodeview = http.GET();
    if (httpCodeview > 0){
        if (httpCodeview == HTTP_CODE_OK){
            responseview = http.getString();
            Serial.println(responseview);
            r = true;
        }
    }else{
        Serial.printf("[HTTP] GET JSON failed, error: %s\n", http.errorToString(httpCode).c_str());
        r = false;
    }
    http.end();
    return r;
}

bool parseJson(String json)
{
    const size_t capacity = JSON_OBJECT_SIZE(4) + JSON_OBJECT_SIZE(5) + 70;
    DynamicJsonDocument doc(capacity);
    deserializeJson(doc, json);

    int code = doc["code"];
    const char *message = doc["message"];

    if (code != 0){
        Serial.print("[API]Code:");
        Serial.print(code);
        Serial.print(" Message:");
        Serial.println(message);

        return false;
    }

    JsonObject data = doc["data"];
    unsigned long data_mid = data["mid"];
    int data_follower = data["follower"];
    if (data_mid == 0){
        Serial.println("[JSON] FORMAT ERROR");
        return false;
    }
    Serial.print("UID: ");
    Serial.print(data_mid);
    Serial.print(" follower: ");
    Serial.println(data_follower);

    follower = data_follower;
    return true;
}

void parseJson1(String json)
{
    const size_t capacity = JSON_OBJECT_SIZE(4) + JSON_OBJECT_SIZE(5) + 70;
    DynamicJsonDocument doc(capacity);
    deserializeJson(doc, json);

    int code = doc["code"];
    const char *message = doc["message"];

    JsonObject data = doc["data"];
    JsonObject archive = data["archive"];
    int data_view = archive["view"];

    Serial.print("vive: ");
    Serial.println(data_view);

    view = data_view;
}



void initdisplay()
{
  u8g2.begin();
  u8g2.enableUTF8Print();
}

void oledClockDisplay()
{
  int years, months, days, hours, minutes, seconds, weekdays;
  years = year();
  months = month();
  days = day();
  hours = hour();
  minutes = minute();
  seconds = second();
  weekdays = weekday();
  Serial.printf("%d/%d/%d %d:%d:%d Weekday:%d\n", years, months, days, hours, minutes, seconds, weekdays);
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_unifont_t_chinese2);
  u8g2.setCursor(0, 14);
  if (isNTPConnected)
    u8g2.print("当前时间 (UTC+8)");
  else
    u8g2.print("无网络!"); //如果上次对时失败，则会显示无网络
  String currentTime = "";
  if (hours < 10)
    currentTime += 0;
  currentTime += hours;
  currentTime += ":";
  if (minutes < 10)
    currentTime += 0;
  currentTime += minutes;
  currentTime += ":";
  if (seconds < 10)
    currentTime += 0;
  currentTime += seconds;
  String currentDay = "";
  currentDay += years;
  currentDay += "/";
  if (months < 10)
    currentDay += 0;
  currentDay += months;
  currentDay += "/";
  if (days < 10)
    currentDay += 0;
  currentDay += days;

  u8g2.setFont(u8g2_font_logisoso24_tr);
  u8g2.setCursor(0, 44);
  u8g2.print(currentTime);
  u8g2.setCursor(0, 61);
  u8g2.setFont(u8g2_font_unifont_t_chinese2);
  u8g2.print(currentDay);
  u8g2.drawXBM(80, 48, 16, 16, xing);
  u8g2.setCursor(95, 62);
  u8g2.print("期");
  if (weekdays == 1)
    u8g2.print("日");
  else if (weekdays == 2)
    u8g2.print("一");
  else if (weekdays == 3)
    u8g2.print("二");
  else if (weekdays == 4)
    u8g2.print("三");
  else if (weekdays == 5)
    u8g2.print("四");
  else if (weekdays == 6)
    u8g2.print("五");
  else if (weekdays == 7)
    u8g2.drawXBM(111, 49, 16, 16, liu);
  u8g2.sendBuffer();
}

/*-------- NTP 代码 ----------*/

const int NTP_PACKET_SIZE = 48;     // NTP时间在消息的前48个字节里
byte packetBuffer[NTP_PACKET_SIZE]; // 输入输出包的缓冲区


time_t getNtpTime()
{
  IPAddress ntpServerIP;          // NTP服务器的地址

  while (Udp.parsePacket() > 0);  // 丢弃以前接收的任何数据包
  Serial.println("Transmit NTP Request");
  // 从池中获取随机服务器
  WiFi.hostByName(ntpServerName, ntpServerIP);
  Serial.print(ntpServerName);
  Serial.print(": ");
  Serial.println(ntpServerIP);
  sendNTPpacket(ntpServerIP);
  uint32_t beginWait = millis();
  while (millis() - beginWait < 1500)
  {
    int size = Udp.parsePacket();
    if (size >= NTP_PACKET_SIZE)
    {
      Serial.println("Receive NTP Response");
      isNTPConnected = true;
      Udp.read(packetBuffer, NTP_PACKET_SIZE); // 将数据包读取到缓冲区
      unsigned long secsSince1900;
      // 将从位置40开始的四个字节转换为长整型，只取前32位整数部分
      secsSince1900 = (unsigned long)packetBuffer[40] << 24;
      secsSince1900 |= (unsigned long)packetBuffer[41] << 16;
      secsSince1900 |= (unsigned long)packetBuffer[42] << 8;
      secsSince1900 |= (unsigned long)packetBuffer[43];
      Serial.println(secsSince1900);
      Serial.println(secsSince1900 - 2208988800UL + timeZone * SECS_PER_HOUR);
      return secsSince1900 - 2208988800UL + timeZone * SECS_PER_HOUR;
    }
  }
  Serial.println("No NTP Response :-("); //无NTP响应
  isNTPConnected = false;
  return 0; //如果未得到时间则返回0
}


// 向给定地址的时间服务器发送NTP请求
void sendNTPpacket(IPAddress& address)
{
  memset(packetBuffer, 0, NTP_PACKET_SIZE);
  packetBuffer[0] = 0b11100011; // LI, Version, Mode
  packetBuffer[1] = 0;          // Stratum, or type of clock
  packetBuffer[2] = 6;          // Polling Interval
  packetBuffer[3] = 0xEC;       // Peer Clock Precision
  // 8 bytes of zero for Root Delay & Root Dispersion
  packetBuffer[12] = 49;
  packetBuffer[13] = 0x4E;
  packetBuffer[14] = 49;
  packetBuffer[15] = 52;
  Udp.beginPacket(address, 123); //NTP需要使用的UDP端口号为123
  Udp.write(packetBuffer, NTP_PACKET_SIZE);
  Udp.endPacket();
}
