#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <Arduino.h> // 包含Arduino标准库，确保正确地包含String类/Users/yeguanliang/Documents/Allen/SMBNODIS_ESP07S_local_sensors_AirMachine_Model34/Define.h

#include "Define.h"
// #include <WiFi.h>     //ESP32
// #include <WebServer.h>//ESP32
#include <ESP8266mDNS.h>
#include <ESP8266WiFi.h>      //ESP8266 ESP07S
#include <ESP8266WebServer.h> //ESP8266 ESP07S
#include <ESP8266HTTPUpdateServer.h>
#include <EEPROM.h>

#include <PubSubClient.h>
#include <SoftwareSerial.h>
#include <TridentTD_LineNotify.h>
#include <stdlib.h>

// #include "global.h"
#include <ArduinoJson.h>
#include "secrets.h"
#include <WiFiClientSecure.h>

#include "NTPClient.h"
#include <time.h>
#include <ModbusMaster.h>
#include <SoftwareSerial.h>

#include <NTPClient.h>
#include <WiFiUdp.h>

#define TIME_ZONE (-5)



time_t now;
time_t nowish = 1510592825;
bool rs485DataReceived = true;
// 定义 ModbusMaster 对象 
/// DOC : https://github.com/4-20ma/ModbusMaster 
/// write the single register
/// public int writeSingleRegister(int slaveAddr,int regAddr,  ushort regVal)
ModbusMaster node;

// Modbus 从设备地址
const int slaveId = 1;  //PAKER 2  一般1
int APMode = 0;
// 读取寄存器的起始地址和数量
const int startAddress = 0;   //一般0   paker 4202   
const int numRegisters = 77; // 花生廠34    捷豹77     Hitachi 90   paker 8

volatile uint32_t isrCounter = 101;
volatile uint32_t RsCounter = 0;
volatile uint32_t WifiCounter = 0;
volatile uint32_t ModbusCounter = 110;
volatile uint32_t lastIsrAt = 0;


unsigned long lastMillis = 0;
unsigned long previousMillis = 0;
unsigned long lastRecordTime = 0;  
unsigned long current_time_ms = 0;   
unsigned long serial2_send_last_showtime_ms = 0; 
long interval = 5000;

// portMUX_TYPE mux0 = portMUX_INITIALIZER_UNLOCKED;

byte sendbuf[128] = {0};    // 串口发送数据，最大128字节缓冲
byte reciveData[256] = {0}; 
String display_mode = "";   // 工作模式命令
String display_status = ""; // 工作状态
String mqtt_out_data = "";  // mqtt发出的字符串，声明字符串变量
os_timer_t *timer0;         // 宣告硬體計時器物件指標
int iLoopIdx = 0;

int iControlEvent = EVENT_NULL;


// init
const char* host = "esp8266-webupdate";
String BaseString = "DM"; // ntut //aquaculture //jetsion //yespowerqy //AirMachine
String DeviceType = "SMB";
String CurrFMVersion = "V1.2";
String NewFMVersion = "V1.3";
String HeaderName = "JetsionAIoT " + DeviceType + " " + CurrFMVersion;
String FileName = DeviceType + "_" + CurrFMVersion + ".bin";
String NewFileName = DeviceType + "_" + NewFMVersion + ".bin";
int MethodCounter = 5; // 方法定義修正處    1飼料桶距離   2水質PH    3RFID   4yespowerqy AirMachine
int UsingAWS = 1;
int Counters = 0;          // 一進入就先讀 後面就不讀了
int RestartCounter = 2160; // 重啟螢幕刷新次數>2106重啟 避免螢幕有問題
int mqttConnectionAttempts = 0;

// task
unsigned long currentMillis = 0;
unsigned long previousMillisReset_1 = 0;
unsigned long previousMillisReset_2 = 0;
unsigned long previousMillisReset = 0;


// button
int enablePin = 0;                     // EN引脚连接的引脚（根据您的电路连接）
unsigned long intervalReset = 5000;    // 按钮按下超过5秒时执行重置操作
bool isResetting = false;              // 初始化为 false，表示按键未开始计时
unsigned long startMillisReset = 0;    // 记录按键开始计时的时间点
unsigned long startTime = 0;           // 開始時間
bool previous_button_15_status = HIGH; // HIGH
byte button_press_counter = 0;
int FirstTrigger = 0; // 確認是否第一次trigger連線

// 定義 HTTP 伺服器端口
char *apSSID = "JetsionAIoTBox";
char *apPassword = "password";
int eepromAddress = 0;
int eepromSize = 512;

SoftwareSerial rs485;
SoftwareSerial Serial01;
SoftwareSerial Serial02;

String storedSsid;     // 用于存储读取到的SSID
String storedPassword; // 用于存储读取到的密码

// WebServer server(80);//ESP32
ESP8266WebServer server(80);
ESP8266HTTPUpdateServer httpUpdater;


// ESP32
/*
enum WiFiMode {
  AP_MODE,
  STA_MODE
};
*/
// ESP8266
/*
enum WiFiMode {
  WIFI_AP,
  WIFI_STA
};
*/

// WiFiMode currentMode = AP_MODE;

#define LINE_TOKEN "e0fHT8xbjNdEgsBIbagSsbnD0s8C3JNYLQflK5JgVSz" // 
String msg;
String mqttCompanyName = ""; // 
String mqttAlertEmail = "allens0426@gmail.com"; //

String mqttSendTime = "3600000";                                      // 設備更新到Line時間
String mqttReciverTime = "60000";                                     // 設備更新到MQTT時間
String mqttShowTime = "1000";                                         // 設備刷新時間
String mqttDeviceName = "";

String mqttUpdateFM = "0";


SoftwareSerial swSer; // 定义一个软串口

String com_in_data = "";  // 串口接收的字符串，声明字符串变量
String com_out_data = ""; // 串口发送的字符串，声明字符串变量

// 发 01 03 02 00 00 01 85 B2，485发送的距离传感器查询命令
byte Serial02_send_data[8]; // Serial02发送数据，传感器1

byte part_dis_num = 0; // 局刷累计次数


// WiFi
char *ssid = "";        
char *password = ""; 

String MACID = String(WiFi.macAddress());
String deviceIdentifier = WiFi.macAddress();

#define AWS_IOT_PUBLISH_TOPIC (BaseString + "/" + deviceIdentifier + "/1")
#define AWS_IOT_SUBSCRIBE_TOPIC (BaseString + "/" + deviceIdentifier + "/0")




const char *mqtt_username = "texturemaker";
const char *mqtt_password = "texturemaker";
const int mqtt_port = 1883;





char *up = "<!DOCTYPE html>"
           "<html>"
           "<head>"
           "<meta charset='UTF-8'>"
           "<h1>UpdateTitleName韌體更新</h1>"
           "<script src='https://ajax.googleapis.com/ajax/libs/jquery/3.2.1/jquery.min.js'></script>"
           "<style>"
           "body {"
           "  font-family: Arial, sans-serif;"
           "  background-color: #f5f5f5;"
           "}"
           ".container {"
           "  max-width: 400px;"
           "  margin: 0 auto;"
           "  padding: 20px;"
           "  background-color: #fff;"
           "  border: 1px solid #ccc;"
           "  border-radius: 5px;"
           "  box-shadow: 0 2px 5px rgba(0, 0, 0, 0.1);"
           "}"
           ".form-group {"
           "  margin-bottom: 20px;"
           "}"
           ".input {"
           "  width: 100%;"
           "  padding: 10px;"
           "  border: 1px solid #ccc;"
           "  border-radius: 4px;"
           "}"
           ".btn {"
           "  padding: 10px 20px;"
           "  background-color: #4CAF50;"
           "  color: white;"
           "  border: none;"
           "  border-radius: 4px;"
           "  cursor: pointer;"
           "}"
           "h1 {"
           "  text-align: center;"
           "}"
           "</style>"
           "</head>"
           "<body>"

           "<div class='container'>"
           "<form method='POST' action='#' enctype='multipart/form-data' id='upload_form'>"
           "<input type='file' name='update'>"
           "<input type='submit' value='Upload'>"
           "</form>"
           "<div id='prg'>progress: 0%</div>"
           "<button id='downloadButton' class='btn'>下載最新版本韌體</button>" // 新增的下载按钮
           "<BR>本頁面進入時下載的bin為目前版本之韌體，請點選上述按鈕下載最新版本韌體更新才為最新版"
           "<BR>若點選後新的檔案，表示您的版本已為最新版本"
           "<script>"
           "$(document).ready(function(){"
           "  var fileUrl = 'https://jetsion.com/up/UpdateFileName';"
           "  var fileName = fileUrl.substring(fileUrl.lastIndexOf('/')+1);"
           "  var a = document.createElement('a');"
           "  a.href = fileUrl;"
           "  a.download = fileName;"
           "  document.body.appendChild(a);"
           "  a.style = 'display: none';"
           "  a.click();"
           "  document.body.removeChild(a);"
           "});"
           "$('#downloadButton').click(function() {"
           "  var fileUrl = 'https://jetsion.com/up/NewFileName';" // 更改为您要下载的文件的 URL
           "  var a = document.createElement('a');"
           "  a.href = fileUrl;"
           "  a.download = fileUrl.substring(fileUrl.lastIndexOf('/')+1);"
           "  document.body.appendChild(a);"
           "  a.style = 'display: none';"
           "  a.click();"
           "  document.body.removeChild(a);"
           "});"
           "$('form').submit(function(e){"
           "  e.preventDefault();"
           "  var form = $('#upload_form')[0];"
           "  var data = new FormData(form);"
           "  $.ajax({"
           "    url: '/update',"
           "    type: 'POST',"
           "    data: data,"
           "    contentType: false,"
           "    processData:false,"
           "    xhr: function() {"
           "      var xhr = new window.XMLHttpRequest();"
           "      xhr.upload.addEventListener('progress', function(evt) {"
           "        if (evt.lengthComputable) {"
           "          var per = evt.loaded / evt.total;"
           "          $('#prg').html('progress: ' + Math.round(per*100) + '%');"
           "        }"
           "      }, false);"
           "      return xhr;"
           "    },"
           "    success:function(d, s) {"
           "      console.log('success!');"
           "    },"
           "    error: function (a, b, c) {"
           "    }"
           "  });"
           "});"
           "</script>"
           "</div>"
           "</body>"
           "</html>";

// String FileName="EPAPER_V1.1.bin";
// String HeaderName="JetsionAIoTEPaper";






String readEEPROMString(int address, int length)
{
    String result;
    for (int i = 0; i < length; ++i)
    {
        char c = EEPROM.read(address + i);
        if (c == '\0')
            break; // 如果遇到null字符，停止读取
        result += c;
    }
    return result;
}

// ESP8266
void ClearRom()
{
    EEPROM.begin(EEPROM_SIZE);
    for (int i = EEPROM_SSID_ADDRESS; i < EEPROM_SIZE; ++i)
    {
        EEPROM.write(i, 0); // 將每個位元組設為 0
    }
    EEPROM.commit();
    EEPROM.end();
    delay(1000);
}

// 计算字符串中子字符串的出现次数
int countOccurrences(String str, char target)
{
    int count = 0;
    for (int i = 0; i < str.length(); i++)
    {
        if (str[i] == target)
        {
            count++;
        }
    }
    return count;
}

// 检查字符串是否包含子字符串
bool contains(String str, String target)
{
    return str.indexOf(target) != -1;
}

char *replaceKeywords(char *original, char *keyword, char *replacement)
{
    // 计算替换后的字符串长度
    int originalLength = strlen(original);
    int keywordLength = strlen(keyword);
    int replacementLength = strlen(replacement);
    int newLength = originalLength + (replacementLength - keywordLength);

    // 分配足够的内存来存储替换后的字符串
    char *newString = new char[newLength + 1];

    // 在原始字符串中查找关键字并替换为新内容
    const char *foundKeyword = strstr(original, keyword);
    if (foundKeyword != nullptr)
    {
        strncpy(newString, original, foundKeyword - original);
        newString[foundKeyword - original] = '\0'; // 添加字符串结尾
        strcat(newString, replacement);
        strcat(newString, foundKeyword + keywordLength);
    }
    else
    {
        strcpy(newString, original); // 如果找不到关键字，则直接复制原始字符串
    }

    return newString;
}
////空壓機加入區段
void RS485Fun()
{
  // 初始化串口
  // Serial.begin(9600);
  // 配置 ModbusMaster
  //rs485.begin(9600, SWSERIAL_8E1, 13, 15); 
  //rs485.begin(9600, SWSERIAL_8E1, 13, 15); 
    Serial01.begin(9600, SWSERIAL_8E1, SERIAL1_RXD, SERIAL1_TXD);   // 花生廠SWSERIAL_8N1  捷豹 SWSERIAL_8E1   Hitachi  SWSERIAL_8N1   PAKER SWSERIAL_8E1
    Serial02.begin(9600, SWSERIAL_8E1, Serial02_RXD, Serial02_TXD); // 花生廠SWSERIAL_8N1  捷豹 SWSERIAL_8E1   Hitachi  SWSERIAL_8N1   PAKER SWSERIAL_8E1

     //Serial01.begin(9600, SWSERIAL_8N1, SERIAL1_RXD, SERIAL1_TXD);   // 花生廠SWSERIAL_8N1  捷豹 SWSERIAL_8E1    Hitachi  SWSERIAL_8N1
     //Serial02.begin(9600, SWSERIAL_8N1, Serial02_RXD, Serial02_TXD); // 花生廠SWSERIAL_8N1  捷豹 SWSERIAL_8E1    Hitachi  SWSERIAL_8N1
  //const int slaveId = 1; //一般1 Paker2
  node.begin(slaveId, Serial02);
}

void RestartFun()
{
  if ((RsCounter) > 900)
  {
      RsCounter = 0;
      Serial.println("Restart");
      ESP.restart();
  }
  /*
  if (xSemaphoreTake(timerSemaphore, 0) == pdTRUE)
  {
    portENTER_CRITICAL(&timerMux);
    portEXIT_CRITICAL(&timerMux);
  }
  */
}

// NTP 服務器地址
const char *ntpServer = "europe.pool.ntp.org";
// 時區偏移（GMT+8 為例）
const long utcOffsetInSeconds = 8 * 3600;
// WiFiUDP udp;

// const char numRegisters = 33;
// const char startAddress = 1;
int16_t RS485data[numRegisters];
int16_t SP600data[numRegisters];
float h;
float t;
time_t current;
int currentn = 0;
int currentn_ex = -125;
int currentn_wificheck = 0;
int currentn_restart = 0;
int sn = 0;
int fake5 = 5000;
int fake6 = 30;
int fake13 = 357;
int fake8 = 200;
int fake1 = 10;
int fake2 = 10;

// WiFiClient espClient;
// PubSubClient client(espClient);

WiFiClientSecure net;

BearSSL::X509List cert(cacert);
BearSSL::X509List client_crt(client_cert);
BearSSL::PrivateKey key(privkey);

PubSubClient client(net); // 如果使用AWS 這邊要打開 下面關掉 AWS OPENAWS
// PubSubClient client(espClient);    //如果用平台 這邊要打開 上面要關掉 AWS CLOSEAWS

// NTPClient timeClient(udp);
//  Define NTP Client to get time
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org");

void messageReceived(char *topic, byte *payload, unsigned int length)
{
    Serial.print("Received [");
    Serial.print(topic);
    Serial.print("]: ");
    for (int i = 0; i < length; i++)
    {
        Serial.print((char)payload[i]);
    }
    Serial.println();
}

// 全局变量，用于存储从 MQTT 消息中提取的十六进制数据
String hexPayload;

void messageHandler(char *topic, byte *payload, unsigned int length)
{
  Serial.print("incoming: ");
  Serial.println(topic);

  StaticJsonDocument<200> doc;
  deserializeJson(doc, payload);
  const char *message = doc["message"];

  // 確保消息長度是偶數（每個字節2個十六進制字符）
  size_t messageLength = strlen(message);
  if (messageLength % 2 != 0)
  {
    Serial.println("Invalid message length.");
    return;
  }

  Serial.print("message (as hex): ");
  for (size_t i = 0; i < messageLength; i += 2)
  {
    char byteStr[3]; // 每個字節2個字符 + 1結束符號
    byteStr[0] = message[i];
    byteStr[1] = message[i + 1];
    byteStr[2] = '\0'; // 字符串結束符號
    // 將十六進制字符串轉換為整數
    byte byteValue = strtol(byteStr, NULL, 16);
    Serial.print("0x");
    if (byteValue < 16)
    {
        Serial.print("0"); // 如果字節值是單位數，補0
    }
    Serial.print(byteValue, HEX);
    if (i < messageLength - 2)
    {
        Serial.print(" ");
    }

    // 将十六进制数据存储到全局变量 hexPayload
    hexPayload += byteStr;
  }
  Serial.println();

  if (hexPayload.length() > 0)
  {
    // 将 hexPayload 发送到 Modbus 设备
    // 将 hexPayload 分割成两个字符一组，并将其转换成十进制数值，然后写入 Modbus 寄存器
    for (int i = 0; i < hexPayload.length(); i += 2)
    {
      String hexByte = hexPayload.substring(i, i + 2);
      uint8_t byteValue = (uint8_t)strtol(hexByte.c_str(), NULL, 16);
      uint16_t registerAddress = startAddress + i / 2; // 计算要写入的 Modbus 寄存器地址
      int writeResult = node.writeSingleRegister(registerAddress, byteValue);

      if (writeResult != node.ku8MBSuccess)
      {
        // 写入失败，您可以在此处添加错误处理逻辑
        Serial.print("Failed to write to Modbus register ");
        Serial.println(registerAddress);
      }
      else
      {
        // 写入成功
        Serial.print("Wrote 0x");
        Serial.print(byteValue, HEX);
        Serial.print(" to Modbus register ");
        Serial.println(registerAddress);
      }
    }

    // 清空 hexPayload，以准备接收下一个消息
    hexPayload = "";
  }
}

void RealTime()
{
  timeClient.begin();
  timeClient.setTimeOffset(28800);
  timeClient.forceUpdate();
}

void NTPConnect(void)
{
  Serial.print("Setting time using SNTP");
  configTime(TIME_ZONE * 3600, 0 * 3600, "pool.ntp.org", "time.nist.gov");
  now = time(nullptr);
  while (now < nowish)
  {
    delay(500);
    Serial.print(".");
    now = time(nullptr);
  }
  Serial.println("done!");
  struct tm timeinfo;
  gmtime_r(&now, &timeinfo);
  Serial.print("Current time: ");
  Serial.print(asctime(&timeinfo));
}

void (*ResetFunc)(void) = 0; // 定义一个函数指针

// EN按钮的中断服务程序
void IRAM_ATTR resetButtonISR()
{
  // 获取当前时间
  unsigned long currentMillis = millis();

  // 判断EN引脚被按下超过5秒
  if (currentMillis - previousMillisReset >= intervalReset)
  {
    Serial.println("More than 5 seconds pressed, clear ROM and restart");
    // 清除ROM
    ClearRom();
    // 重启ESP32
    ESP.restart();
  }
  else
  { // EN引脚被按下不足5秒
    Serial.println("Less than 5 seconds pressed, restart");
    // 重启ESP32
    ESP.restart();
  }
}



void Button2PressedEreseRomCheck()
{
  /// check no press pin then return
  if (digitalRead(enablePin) == HIGH)
  {
    return; 
  }

  // 如果按鈕被按下
  Serial.println("EN pressed button2");
  startTime = millis(); // 紀錄開始時間
  Serial.println("startTime...");
  Serial.println(startTime);
  while (digitalRead(enablePin) == LOW)
  { // 等待按鈕釋放
    Serial.println("millis...");
    Serial.println(millis());
    if (millis() - startTime >= intervalReset)
    { // 如果長按超過5秒
      Serial.println("EN pressed for more than 5 seconds, clear ROM and restart");
      ClearRom();
      ESP.restart(); // 重启ESP32
      // 執行長按動作
      Serial.println("按鈕被長按5秒");
      break; // 跳出while迴圈
    }
  }
}

void WifiEEpormSettingRead(String *ssid, String *password)
{
  int i = 0;
  char c;
  *ssid = "";
  *password = "";

  /// fetch the WIFI ssid / password
  EEPROM.begin(EEPROM_SIZE);
  /// check the i is start address is zero or one
  for (i = EEPROM_SSID_ADDRESS; i < EEPROM_SIZE; ++i)
  {
    c = EEPROM.read(i);
    if(c == '\0')
    {
      break;
    }
    *ssid = *ssid + char(EEPROM.read(i));
  }
  // 讀取密碼 ESP8266
  /// check the i is start address is zero or one
  for (i = EEPROM_PASSWORD_ADDRESS; i < EEPROM_SIZE; ++i)
  {
    c = EEPROM.read(i);
    if(c == '\0')
    {
      break;
    }
    *password = *password + char(EEPROM.read(i));
  }
  EEPROM.end();
}

void WifiApChangeSSidAndPassword(int bServerStart)
{
  String deviceIdentifier = WiFi.macAddress();
  WiFi.mode(WIFI_AP);
  char *apSSID = "JetsionAIoTBox";
  char *apPassword = "password";
  size_t apSSIDLength = strlen(apSSID);
  size_t deviceIdentifierLength = deviceIdentifier.length();
  size_t combinedLength = apSSIDLength + deviceIdentifierLength + 1; // 加1是为了空字符终止符

  // 创建一个新的缓冲区来存储连接后的SSID
  char *combinedSSID = new char[combinedLength];
  // 将apSSID复制到新缓冲区
  strcpy(combinedSSID, apSSID);
  // 使用strcat将deviceIdentifier连接到apSSID
  strcat(combinedSSID, deviceIdentifier.c_str());
  // 创建软AP
  String ssidString = String(combinedSSID); // 將 combinedSSID 轉換為 String
  ssidString.replace(":", "");              // 去除冒號

  Serial.print("ssidString");
  Serial.println(ssidString);
  WiFi.softAP(ssidString, apPassword);
  Serial.print("AP IP address: ");
  Serial.println(WiFi.softAPIP());

  if(bServerStart == 1)
  {
    // 建立Web伺服器路由
    server.on("/", handleRoot);
    server.on("/save", handleSave);

    // 啟動Web伺服器
    server.begin();
  }
}




void setup()
{
    // 清除ROM
    //ClearRom();
    // 重启ESP32
    //ESP.restart();
    
  int MethodCounter = 5; // 方法定義修正處    1飼料桶距離   2水質PH    3RFID   4yespowerqy AirMachine
  int i;
  String storedSsid;
  String storedPassword;
  bool AirMachine = false;
  bool AP = false;

  Button2PressedEreseRomCheck();
  RealTime();

  deviceIdentifier.replace(":", "");
  Serial.begin(115200);
  WifiEEpormSettingRead(&storedSsid, &storedPassword);
  pinMode(enablePin, INPUT_PULLUP); // 判斷Boot鈕
  Serial.println("setup00001");
  if ((storedSsid.length() > 0) && (storedPassword.length() > 0))
  {
    Serial.println("setup00002");
    Serial.println(storedSsid);
    Serial.println(storedPassword);
    APMode = 1;
    if (MethodCounter == 5)
    {
      Serial.println("setup001");
      AirMachine = true;
      AP = true;
    }
  }
  else if (MethodCounter == 5)
  {
    WifiApChangeSSidAndPassword(1);
  }

  if (AirMachine == true)
  {
    Serial.println("setup002");
    if (AP == true)
    {
      Serial.println("setup003");
      RS485Fun();
      connectAWS();
    }
  }
  // 這邊下面else沒有拿掉 AWS就連不上 但是 裡面邏輯是不會進來的
  else
  {
  }
}






//////////////////////////////////
WiFiClient http_client;
WiFiClient mqtt_client;

void loop()
{
    String storedPassword;
    String storedSsid;
    timeClient.update();

    //Serial.println("Loop0000GOGOGO");
    int MethodCounter = 5; // 方法定義修正處    1飼料桶距離   2水質PH    3RFID   4yespowerqy AirMachine

    WifiEEpormSettingRead(&storedSsid, &storedPassword);
  
    if ((storedSsid.length() > 0) && (storedPassword.length() > 0))
    {
      //Serial.println("LoopPrintstoredSsid");
      //Serial.println(storedSsid);
      //Serial.println("LoopPrintstoredPassword");
      //Serial.println(storedPassword);
      APMode = 1;
    }

    timeClient.update();
    checkPress();
    now = time(nullptr);

    if (!client.connected())
    {
      if (MethodCounter == 5 && storedSsid.length() > 0 && storedPassword.length() > 0)
      {
         connectAWS();
      }
    }
    else
    {
      client.loop();

      lastMillis = millis();
      // publishMessageTest();
      //Serial.println("Loop00006");
      if (MethodCounter == 5) // 空壓機的原本邏輯
      {
        //Serial.println("Loop00007");
        if (APMode == 0)
        {
          //Serial.println("Loop00008");
          // LoopAirMachine();
        }
        else
        {
         //Serial.println("Loop00009");
                if (millis() - lastRecordTime >= 60000 || Counters==0)
                {
                    LoopAirMachine();
                   
                    lastRecordTime = millis();
                    Counters++;
                }          
        }
      }
      WifiEEpormSettingRead(&storedSsid, &storedPassword);
      if ((MethodCounter == 5) && (storedSsid.length() > 0) && (storedPassword.length() > 0))
      {
        //Serial.println("Loop00010");
        //Serial.println("LoopAirMachine");
        if (millis() - lastRecordTime >= 60000 || Counters==0)
                {
                    LoopAirMachine();
                    lastRecordTime = millis();
                    Counters++;
                }          
        //Serial.println("Loop00011");
      }
    }

    server.handleClient();

    // 檢查是否需要切換MODE
    // if (currentMode == AP_MODE) {//ESP32
    // Serial.println(WiFi.getMode());
    if (WiFi.getMode() == 2)
    { // ESP8266
      // 檢查是否有儲存的WiFi設定

      // 如果有儲存的WiFi設定，切換到STA模式並連接

      // 讀取SSID ESP8266
      WifiEEpormSettingRead(&storedSsid, &storedPassword);

      if (storedSsid.length() > 0 && storedPassword.length() > 0)
      {
        WiFi.begin(storedSsid.c_str(), storedPassword.c_str());
        Serial.println(storedSsid.c_str());
        Serial.println(storedPassword.c_str());
        Serial.println("Connecting to WiFi6...");
        int x = 0;
        while (WiFi.status() != WL_CONNECTED)
        {
          x++;
          delay(1000);
          Serial.println("Connecting to WiFi1...");

          if (x > 100)
          {
              // ClearRom();
            ResetFunc(); // 调用重置函数
          }
        }

        Serial.println("Connected to WiFi");
        Serial.print("IP address: ");
        Serial.println(WiFi.localIP());

        // 切換到STA模式
        // currentMode = STA_MODE;       // { //ESP32
        if (WiFi.getMode() == WIFI_STA)
        { // ESP8266 WIFI_AP,WIFI_STA
        }

        // 停用AP模式
        WiFi.softAPdisconnect(true);

        // 儲存MODE設定到EEPROM中
        EEPROM.begin(eepromSize);
        // EEPROM.write(eepromAddress, currentMode); //ESP32
        EEPROM.put(eepromAddress, WiFi.getMode()); // ESP8266
        EEPROM.commit();
        EEPROM.end();
      }
    }
    else // STA
    {
        String storedSsid;
        for (int i = EEPROM_SSID_ADDRESS; i < EEPROM_SIZE && EEPROM.read(i) != '\0'; ++i)
        {
            storedSsid += char(EEPROM.read(i));
        }
        // 讀取密碼 ESP8266
        String storedPassword;
        for (int i = EEPROM_PASSWORD_ADDRESS; i < EEPROM_SIZE && EEPROM.read(i) != '\0'; ++i)
        {
            storedPassword += char(EEPROM.read(i));
        }
        EEPROM.end();

        MethodCounter = 5;
        if (storedSsid.length() > 0 && storedPassword.length() > 0 && MethodCounter == 5)
        {
          int x = 0;

          while (WiFi.status() != WL_CONNECTED)
          {
            delay(1000);
            Serial.println("Connecting to WiFi2...");
            x++;
            if (x > 100)
            {
                // ClearRom();
                ResetFunc(); // 调用重置函数
            }
          }
        }
        else
        {
        }
    }

    

    //------------------------------------
    client.loop();
}

////END////



void LoadMsgUpdateFirmware()
{
    // 读取存储的SSID和密码
    EEPROM.begin(eepromSize);
    // storedSsid = EEPROM.readString(eepromAddress);    //ESP32
    // storedPassword = EEPROM.readString(eepromAddress + storedSsid.length() + 1);//ESP32

    // 讀取SSID ESP8266
    String storedSsid;
    for (int i = EEPROM_SSID_ADDRESS; i < EEPROM_SIZE && EEPROM.read(i) != '\0'; ++i)
    {
        storedSsid += char(EEPROM.read(i));
    }

    EEPROM.end();

    msg = "";
    msg = msg + "設備韌體更新整通知" + "\r\n";
    msg = msg + "親愛的" + mqttCompanyName + "您好" + "\r\n";
    msg = msg + "您目前的設備號：" + mqttDeviceName + "\r\n";
    msg = msg + "已進入設備已進入更新模式\r\n";
    msg = msg + "目前設備連線SSID:" + storedSsid + "\r\n";
    msg = msg + "請進入下列網址更新本設備韌體http://" + WiFi.localIP().toString().c_str() + "/up\r\n";
    msg = msg + "更新後本設備將自動重啟 謝謝您" + "\r\n";
}









void handleRoot()
{
    String html = "<!DOCTYPE html>";
    html += "<html>";
    html += "<head>";
    html += "<meta charset='UTF-8'>";
    html += "<title>JetsionAioT網路設定</title>";
    html += "<style>";
    html += "body {";
    html += "  font-family: Arial, sans-serif;";
    html += "  background-color: #f5f5f5;";
    html += "}";

    html += ".container {";
    html += "  max-width: 400px;";
    html += "  margin: 0 auto;";
    html += "  padding: 20px;";
    html += "  background-color: #fff;";
    html += "  border: 1px solid #ccc;";
    html += "  border-radius: 5px;";
    html += "  box-shadow: 0 2px 5px rgba(0, 0, 0, 0.1);";
    html += "}";

    html += ".form-group {";
    html += "  margin-bottom: 20px;";
    html += "}";

    html += ".input {";
    html += "  width: 100%;";
    html += "  padding: 10px;";
    html += "  border: 1px solid #ccc;";
    html += "  border-radius: 4px;";
    html += "}";

    html += ".btn {";
    html += "  padding: 10px 20px;";
    html += "  background-color: #4CAF50;";
    html += "  color: white;";
    html += "  border: none;";
    html += "  border-radius: 4px;";
    html += "  cursor: pointer;";
    html += "}";

    html += "h1 {";
    html += "  text-align: center;";
    html += "}";

    html += "</style>";
    html += "</head>";
    html += "<body>";
    html += "<h1>JetsionAioT網路設定</h1>";
    html += "<div class='container'>";
    html += "<form action='/save' method='POST'>";
    html += "<div class='form-group'>";
    html += "<label for='ssid'>SSID:</label>";
    html += "<select name='ssid' class='input'>";

    // 取得掃描到的WiFi網路清單
    int numNetworks = WiFi.scanNetworks();
    for (int i = 0; i < numNetworks; i++)
    {
        html += "<option value='" + WiFi.SSID(i) + "'>" + WiFi.SSID(i) + "</option>";
    }

    html += "</select>";
    html += "</div>";
    html += "<div class='form-group'>";
    html += "<label for='password'>密碼:</label>";
    html += "<input type='text' name='password' class='input'>";
    html += "</div>";
    html += "<div class='form-group'>";
    html += "<button type='submit' class='btn'>儲存</button>";
    html += "</div>";
    html += "</form>";
    html += "</div>";
    html += "</body>";
    html += "</html>";

    server.send(200, "text/html", html);
}


const char *loginIndex =
    "<form name='loginForm'>"
    "<table width='20%' bgcolor='A09F9F' align='center'>"
    "<tr>"
    "<td colspan=2>"
    "<center><font size=4><b>Jetsion AIoT Login Page</b></font></center>"
    "<br>"
    "</td>"
    "<br>"
    "<br>"
    "</tr>"
    "<tr>"
    "<td>Username:</td>"
    "<td><input type='text' size=25 name='userid'><br></td>"
    "</tr>"
    "<br>"
    "<br>"
    "<tr>"
    "<td>Password:</td>"
    "<td><input type='Password' size=25 name='pwd'><br></td>"
    "<br>"
    "<br>"
    "</tr>"
    "<tr>"
    "<td><input type='submit' onclick='check(this.form)' value='Login'></td>"
    "</tr>"
    "</table>"
    "</form>"
    "<script>"
    "function check(form)"
    "{"
    "if(form.userid.value=='admin' && form.pwd.value=='admin')"
    "{"
    "window.open('/up')"
    "}"
    "else"
    "{"
    " alert('Error Password or Username')/*displays error message*/"
    "}"
    "}"
    "</script>";

/*
 * Server Index Page
 */

void StartToUpdateFM()
{
  MDNS.begin(host);
  httpUpdater.setup(&server);
  server.begin();
  MDNS.addService("http", "tcp", 80);
  Serial.printf("HTTPUpdateServer ready! Open http://%s.local/update in your browser\n", WiFi.localIP());
  Serial.println(WiFi.localIP());  
  
}

void handleSave()
{
    String newSsid = server.arg("ssid");
    String newPassword = server.arg("password");

    Serial.println(newSsid);
    Serial.println(newPassword);

    // 清空存储变量
    String storedSsid = "";
    String storedPassword = "";

    // 儲存新的WiFi設定到EEPROM中
    EEPROM.begin(EEPROM_SIZE);
    for (int i = 0; i < newSsid.length(); ++i)
    {
        EEPROM.write(EEPROM_SSID_ADDRESS + i, newSsid[i]);
    }
    EEPROM.write(EEPROM_SSID_ADDRESS + newSsid.length(), '\0'); // 终止符
    for (int i = 0; i < newPassword.length(); ++i)
    {
        EEPROM.write(EEPROM_PASSWORD_ADDRESS + i, newPassword[i]);
    }
    EEPROM.write(EEPROM_PASSWORD_ADDRESS + newPassword.length(), '\0'); // 终止符
    EEPROM.commit();
    EEPROM.end();

    // 写入EEPROM之后添加延迟
    delay(500); // 等待500毫秒，以确保EEPROM中的数据完全提交
    EEPROM.begin(EEPROM_SIZE);
    // 重新读取存储的SSID
    for (int i = EEPROM_SSID_ADDRESS; i < eepromSize && EEPROM.read(i) != '\0'; ++i)
    {
        storedSsid += char(EEPROM.read(i));
    }

    // 重新读取存储的密码
    for (int i = EEPROM_PASSWORD_ADDRESS; i < eepromSize && EEPROM.read(i) != '\0'; ++i)
    {
        storedPassword += char(EEPROM.read(i));
    }

    Serial.println(storedSsid);
    Serial.println(storedPassword);

    delay(500); // 等待500毫秒，以确保EEPROM中的数据完全提交
    // 重新启动
    ESP.restart();
}

void connectAWS()
{
    // ClearRom();
    // ESP.restart();

    EEPROM.begin(EEPROM_SIZE);
    deviceIdentifier.replace(":", "");
    Serial.begin(115200);
    String storedSsid;
    for (int i = EEPROM_SSID_ADDRESS; i < EEPROM_SIZE && EEPROM.read(i) != '\0'; ++i)
    {
        storedSsid += char(EEPROM.read(i));
    }
    // 讀取密碼 ESP8266
    String storedPassword;
    for (int i = EEPROM_PASSWORD_ADDRESS; i < EEPROM_SIZE && EEPROM.read(i) != '\0'; ++i)
    {
        storedPassword += char(EEPROM.read(i));
    }

    bool AirMachine = false;
    bool AP = false;

    EEPROM.end();

    delay(3000);
    WiFi.mode(WIFI_STA);
    Serial.print("storedSsid===>");
    Serial.println(storedSsid.c_str());
    Serial.print("storedPassword===>");
    Serial.println(storedPassword.c_str());
    WiFi.begin(storedSsid, storedPassword);

    Serial.println(String("Attempting to connect to SSID: ") + String(storedSsid));

    while (WiFi.status() != WL_CONNECTED)
    {
        Serial.print("**");
        Serial.println(WiFi.status());
        Serial.print("..");
        delay(1000);
        Button2PressedEreseRomCheck();
    }

    NTPConnect();

    net.setTrustAnchors(&cert);
    net.setClientRSACert(&client_crt, &key);

    client.setServer(MQTT_HOST, 8883);
    client.setCallback(messageReceived);
    Serial.println(MQTT_HOST);
    Serial.println("Connecting to AWS IOT");
    Serial.println(THINGNAME);

    while (!client.connect(THINGNAME))
    {
        
        Serial.print(".");
        delay(1000);
    }

    if (!client.connected())
    {
        Serial.println("AWS IoT Timeout!");
        return;
    }
    // Subscribe to a topic
    String publishTopic = AWS_IOT_PUBLISH_TOPIC;
    String subscribeTopic = AWS_IOT_SUBSCRIBE_TOPIC;
    publishTopic.replace(":", "");
    subscribeTopic.replace(":", "");
    client.subscribe(subscribeTopic.c_str());

    Serial.println("AWS IoT Connected!");
}
String getISO8601Time()
{
    // 獲取時間信息
    time_t epochTime = timeClient.getEpochTime();
    struct tm *ptm = gmtime((time_t *)&epochTime);

    int currentYear = ptm->tm_year + 1900; // 因為 tm_year 保存的是自 1900 年以來的年份
    int currentMonth = ptm->tm_mon + 1;
    int monthDay = ptm->tm_mday;
    int currentHour = timeClient.getHours();
    int currentMinute = timeClient.getMinutes();
    int currentSecond = timeClient.getSeconds();

    // 將數據格式化為 ISO 8601 時間戳
    String isoTime = String(currentYear) + "-" +
                     (currentMonth < 10 ? "0" : "") + String(currentMonth) + "-" +
                     (monthDay < 10 ? "0" : "") + String(monthDay) + "T" +
                     (currentHour < 10 ? "0" : "") + String(currentHour) + ":" +
                     (currentMinute < 10 ? "0" : "") + String(currentMinute) + ":" +
                     (currentMinute < 10 ? "0" : "") + String(currentSecond) + "Z";

    return isoTime;
}

word CalcCrc(byte *nData, word wLength)
{
  byte nTemp;
  word wCRCWord = 0xFFFF;
  word wCRCTable[] = 
  {
   0X0000, 0XC0C1, 0XC181, 0X0140, 0XC301, 0X03C0, 0X0280, 0XC241,
   0XC601, 0X06C0, 0X0780, 0XC741, 0X0500, 0XC5C1, 0XC481, 0X0440,
   0XCC01, 0X0CC0, 0X0D80, 0XCD41, 0X0F00, 0XCFC1, 0XCE81, 0X0E40,
   0X0A00, 0XCAC1, 0XCB81, 0X0B40, 0XC901, 0X09C0, 0X0880, 0XC841,
   0XD801, 0X18C0, 0X1980, 0XD941, 0X1B00, 0XDBC1, 0XDA81, 0X1A40,
   0X1E00, 0XDEC1, 0XDF81, 0X1F40, 0XDD01, 0X1DC0, 0X1C80, 0XDC41,
   0X1400, 0XD4C1, 0XD581, 0X1540, 0XD701, 0X17C0, 0X1680, 0XD641,
   0XD201, 0X12C0, 0X1380, 0XD341, 0X1100, 0XD1C1, 0XD081, 0X1040,
   0XF001, 0X30C0, 0X3180, 0XF141, 0X3300, 0XF3C1, 0XF281, 0X3240,
   0X3600, 0XF6C1, 0XF781, 0X3740, 0XF501, 0X35C0, 0X3480, 0XF441,
   0X3C00, 0XFCC1, 0XFD81, 0X3D40, 0XFF01, 0X3FC0, 0X3E80, 0XFE41,
   0XFA01, 0X3AC0, 0X3B80, 0XFB41, 0X3900, 0XF9C1, 0XF881, 0X3840,
   0X2800, 0XE8C1, 0XE981, 0X2940, 0XEB01, 0X2BC0, 0X2A80, 0XEA41,
   0XEE01, 0X2EC0, 0X2F80, 0XEF41, 0X2D00, 0XEDC1, 0XEC81, 0X2C40,
   0XE401, 0X24C0, 0X2580, 0XE541, 0X2700, 0XE7C1, 0XE681, 0X2640,
   0X2200, 0XE2C1, 0XE381, 0X2340, 0XE101, 0X21C0, 0X2080, 0XE041,
   0XA001, 0X60C0, 0X6180, 0XA141, 0X6300, 0XA3C1, 0XA281, 0X6240,
   0X6600, 0XA6C1, 0XA781, 0X6740, 0XA501, 0X65C0, 0X6480, 0XA441,
   0X6C00, 0XACC1, 0XAD81, 0X6D40, 0XAF01, 0X6FC0, 0X6E80, 0XAE41,
   0XAA01, 0X6AC0, 0X6B80, 0XAB41, 0X6900, 0XA9C1, 0XA881, 0X6840,
   0X7800, 0XB8C1, 0XB981, 0X7940, 0XBB01, 0X7BC0, 0X7A80, 0XBA41,
   0XBE01, 0X7EC0, 0X7F80, 0XBF41, 0X7D00, 0XBDC1, 0XBC81, 0X7C40,
   0XB401, 0X74C0, 0X7580, 0XB541, 0X7700, 0XB7C1, 0XB681, 0X7640,
   0X7200, 0XB2C1, 0XB381, 0X7340, 0XB101, 0X71C0, 0X7080, 0XB041,
   0X5000, 0X90C1, 0X9181, 0X5140, 0X9301, 0X53C0, 0X5280, 0X9241,
   0X9601, 0X56C0, 0X5780, 0X9741, 0X5500, 0X95C1, 0X9481, 0X5440,
   0X9C01, 0X5CC0, 0X5D80, 0X9D41, 0X5F00, 0X9FC1, 0X9E81, 0X5E40,
   0X5A00, 0X9AC1, 0X9B81, 0X5B40, 0X9901, 0X59C0, 0X5880, 0X9841,
   0X8801, 0X48C0, 0X4980, 0X8941, 0X4B00, 0X8BC1, 0X8A81, 0X4A40,
   0X4E00, 0X8EC1, 0X8F81, 0X4F40, 0X8D01, 0X4DC0, 0X4C80, 0X8C41,
   0X4400, 0X84C1, 0X8581, 0X4540, 0X8701, 0X47C0, 0X4680, 0X8641,
   0X8201, 0X42C0, 0X4380, 0X8341, 0X4100, 0X81C1, 0X8081, 0X4040 };


   while (wLength--)
   {
      nTemp = *nData++ ^ wCRCWord;
      wCRCWord >>= 8;
      wCRCWord  ^= wCRCTable[nTemp];
   }
   return wCRCWord;
} 

int Rs485WritePage(byte *pData, int len,byte *pOutData)
{
  int i = 0 ;
  int x = 0;

  int data_flag = 0;
  int data_len = 0;

  /// write data 
  for (i = 0 ; i < len; i++)
  {
    Serial.printf("Write [%d]:%X\r\n",i, pData[i]);
    Serial02.write(pData[i]);
  }
  /// read back 
  delay(100);
  while (Serial02.available() > 0)  
  {
    pOutData[data_len] = char(Serial02.read());
    data_len=data_len+1;
    delay(10);
    data_flag = 1;
  }
  
  if(data_flag == 1)
  {
    Serial.printf("Rs485WritePage rec data (%d)\r\n", data_len);
    return data_len;
  }
  else
  {
    return 0;
  }
}

int RS485ReadHoldingRegisters(uint16_t startAddress,uint16_t numRegisters)
{
  word crc;
  int size = 0;
 
  sendbuf[0] = slaveId;
  sendbuf[1] = 0x03;
  sendbuf[2] = ((startAddress >> 8)& 0xFF);
  sendbuf[3] = (startAddress & 0xFF);
  sendbuf[4] = ((numRegisters >> 8)& 0xFF);
  sendbuf[5] = (numRegisters & 0xFF);

  crc = CalcCrc(sendbuf, 6);
  sendbuf[6] = (crc & 0xFF);
  sendbuf[7] = ((crc >> 8) & 0xFF);
   
  Serial.println("RS485ReadHoldingRegisters...");
  return Rs485WritePage(sendbuf, 8, reciveData);
}

void publishMessageTest1(int a, int b)
{
  StaticJsonDocument<1024> doc;
  char jsonBuffer[1024];
  String topicDeviceIdentifierString = deviceIdentifier;
  
  if (digitalRead(enablePin) == LOW)
  { // 如果按鈕被按下
      Serial.println("EN pressed button2");
      startTime = millis(); // 紀錄開始時間
      Serial.println("startTime...");
      Serial.println(startTime);
      while (digitalRead(enablePin) == LOW)
      { // 等待按鈕釋放
          Serial.println("millis...");
          Serial.println(millis());
          if (millis() - startTime >= intervalReset)
          { // 如果長按超過5秒
              Serial.println("EN pressed for more than 5 seconds, clear ROM and restart");
              ClearRom();
              ESP.restart(); // 重启ESP32
              // 執行長按動作
              Serial.println("按鈕被長按5秒");
              break; // 跳出while迴圈
          }
          else
          {
              // ESP.restart(); // 重启ESP32
          }
      }
  }

  topicDeviceIdentifierString.replace(":", "");
  doc["code"] = topicDeviceIdentifierString;
  doc["model"] = "nantou";
  doc["lat"] = 23.96115942932117;
  doc["lon"] = 120.96485903857014;
  doc["timestamp"] = getISO8601Time();

  // 填充 data 對象
  JsonObject dataObject = doc.createNestedObject("data");
  for (int i = a; i < b; i++)
  {
      // 生成 1 到 200 之間的隨機數並填充到 JSON 中
      dataObject[String(i)] = random(1, 201);
  }

  // 序列化 JSON 數據到字符串
  serializeJson(doc, jsonBuffer);

  // 打印 JSON 字串和 MQTT 主題
  Serial.println(AWS_IOT_PUBLISH_TOPIC);
  Serial.println(jsonBuffer);

  // 發佈 MQTT 消息
  String publishTopic = AWS_IOT_PUBLISH_TOPIC;
  publishTopic.replace(":", "");
  // client.publish(publishTopic.c_str(), jsonBuffer); // 发布消息
  client.publish_P(publishTopic.c_str(), (const uint8_t *)jsonBuffer, strlen(jsonBuffer), false); // 发布消息，不保留
  Serial.print("已上传到 MQTT：");
  Serial.println(publishTopic);
  delay(1000); // 延遲 10 秒
}

void publishMessageTest(int a, int b)
{
  StaticJsonDocument<1024> doc;
  char jsonBuffer[1024];
  String topicDeviceIdentifierString = deviceIdentifier;
  
  if (digitalRead(enablePin) == LOW)
  { // 如果按鈕被按下
      Serial.println("EN pressed button2");
      startTime = millis(); // 紀錄開始時間
      Serial.println("startTime...");
      Serial.println(startTime);
      while (digitalRead(enablePin) == LOW)
      { // 等待按鈕釋放
          Serial.println("millis...");
          Serial.println(millis());
          if (millis() - startTime >= intervalReset)
          { // 如果長按超過5秒
              Serial.println("EN pressed for more than 5 seconds, clear ROM and restart");
              ClearRom();
              ESP.restart(); // 重启ESP32
              // 執行長按動作
              Serial.println("按鈕被長按5秒");
              break; // 跳出while迴圈
          }
          else
          {
              // ESP.restart(); // 重启ESP32
          }
      }
  }

  topicDeviceIdentifierString.replace(":", "");
  doc["code"] = topicDeviceIdentifierString;
  doc["model"] = "nantou";
  doc["lat"] = 23.96115942932117;
  doc["lon"] = 120.96485903857014;
  doc["timestamp"] = getISO8601Time();

  // 填充 data 對象
  JsonObject dataObject = doc.createNestedObject("data");
  for (int i = a; i < b; i++)
  {
      // 生成 1 到 200 之間的隨機數並填充到 JSON 中
      dataObject[String(i)] = random(1, 201);
  }

  // 序列化 JSON 數據到字符串
  serializeJson(doc, jsonBuffer);

  // 打印 JSON 字串和 MQTT 主題
  Serial.println(AWS_IOT_PUBLISH_TOPIC);
  Serial.println(jsonBuffer);

  // 發佈 MQTT 消息
  String publishTopic = AWS_IOT_PUBLISH_TOPIC;
  publishTopic.replace(":", "");
  // client.publish(publishTopic.c_str(), jsonBuffer); // 发布消息
  client.publish_P(publishTopic.c_str(), (const uint8_t *)jsonBuffer, strlen(jsonBuffer), false); // 发布消息，不保留
  Serial.print("已上传到 MQTT：");
  Serial.println(publishTopic);
  delay(1000); // 延遲 10 秒
}

void LoopAirMachine()
{

  // Modbus 从设备地址
  const int slaveId = 1;       //一般1    parker 2
  // 读取寄存器的起始地址和数量
  const int startAddress = 0;  //一般0    parker 4202
  const int numRegisters = 77; // 花生廠34   捷豹77   hitachi 33   parker 8

  ///////////////////////////////////
  // 测试串口2转RS485
  current_time_ms = millis();

  if ((current_time_ms - serial2_send_last_showtime_ms) < mqttShowTime.toInt()) // 如果当前时刻距离上次发送查询命令间隔大于等于1s
  {
    return;
  }

  serial2_send_last_showtime_ms = current_time_ms;

  // 在这里添加Modbus通信的调试代码
  if ((ModbusCounter) <= 100)
  {
    return;
  }

  // ModbusCounter = 0;
  ModbusCounter = 110;
  // 发送读取请求并等待响应


//============================================================================
#if 0
  //Paker區域開始
  uint8_t resultPaker = node.readHoldingRegisters(4202, 2);

  // 检查响应是否成功
  if (resultPaker == node.ku8MBSuccess) {
    Serial.println("Success");
    // 获取读取到的数据
      RS485data[0] = node.getResponseBuffer(0);
      // Serial.print("寄存器 ");
      Serial.print(0);
      Serial.print(" is ");
      Serial.println(RS485data[0]);
  } else {
    Serial.print("Error");
    Serial.println(resultPaker);
  }
        uint8_t result4 = node.readHoldingRegisters(4210, 2);

  // 检查响应是否成功
  if (result4 == node.ku8MBSuccess) {
    Serial.println("Success");
    // 获取读取到的数据
      RS485data[1] = node.getResponseBuffer(0);
      // Serial.print("寄存器 ");
      Serial.print(1);
      Serial.print(" is ");
      Serial.println(RS485data[1]);
  } else {
    Serial.print("Error");
    Serial.println(resultPaker);
  }
      uint8_t result3 = node.readHoldingRegisters(4220, 2);

  // 检查响应是否成功
  if (result3 == node.ku8MBSuccess) {
    Serial.println("Success");
    // 获取读取到的数据
      RS485data[2] = node.getResponseBuffer(0);
      // Serial.print("寄存器 ");
      Serial.print(2);
      Serial.print(" is ");
      Serial.println(RS485data[2]);
  } else {
    Serial.print("Error");
    Serial.println(resultPaker);
  }
      uint8_t result2 = node.readCoils(1280, numRegisters);

  // 检查响应是否成功
  if (result2 == node.ku8MBSuccess) {
    Serial.println("Success");
    // 获取读取到的数据
    RS485data[9] = node.getResponseBuffer(0);
          Serial.print(9);
      Serial.print(" is ");
      Serial.println(RS485data[9]);
    for (int i = 0; i <8; i++) {
      RS485data[10+i] = RS485data[9]&0x1;
      RS485data[9]>>=1;
      // Serial.print("寄存器 ");
      Serial.print(10+i);
      Serial.print(" is ");
      Serial.println(RS485data[10+i]);
    }
  } else {
    Serial.print("Error");
    Serial.println(resultPaker);
  }


  //因為晶片一次最多送10個topic 不然會溢位
  String ThisDateTime=getISO8601Time();
  Serial.println(ThisDateTime);
  publishMessagePaker(RS485data, 0, 10,ThisDateTime);
  publishMessagePaker(RS485data, 10, 18,ThisDateTime);
  //publishMessageTest(0,10);
  Serial.print("已上传到 MQTT完成");
  //Paker區域結束
  //======================================================
  #endif

//============================================================================
#if 0
  //Hitachi區域開始

uint8_t result = node.readHoldingRegisters(startAddress, numRegisters);

  // 检查响应是否成功
  if (result == node.ku8MBSuccess) {
    Serial.println("Success");
    // 获取读取到的数据
    for (int i = startAddress; i < numRegisters+1; i++) {
      RS485data[i] = node.getResponseBuffer(i);
      // Serial.print("寄存器 ");
      Serial.print(i);
      Serial.print(" is ");
      Serial.println(RS485data[i]);
    }
  } else {
    Serial.print("Error");
    Serial.println(result);
  }

 //因為晶片一次最多送10個topic 不然會溢位
  String ThisDateTime=getISO8601Time();
  Serial.println(ThisDateTime);
  publishMessageHitachi(RS485data, 0, 10,ThisDateTime,"N","N2");
  publishMessageHitachi(RS485data, 10, 20,ThisDateTime,"N","N2");
  publishMessageHitachi(RS485data, 20, 30,ThisDateTime,"N","N2");
  publishMessageHitachi(RS485data, 30, 33,ThisDateTime,"N","N2");
  //publishMessageHitachi(RS485data, 0, 10,ThisDateTime,"G1","G1");
  //publishMessageHitachi(RS485data, 10, 20,ThisDateTime,"G1","G1");
  //publishMessageHitachi(RS485data, 20, 30,ThisDateTime,"G1","G1");
  
  //publishMessageTest(0,10);
  Serial.print("已上传到 MQTT完成");
  
  //Hitachi區域結束
  //======================================================
  #endif


  
  
#if 1
    //=============================================================================
  //區域開始
  int startAddr = 0;  // 一般0 paker 4202
  const int numRegister = 77; // 花生廠34    捷豹77    hitachi 90    paker 8
  // 获取读取到的数据
  uint8_t result = 0;
  int16_t RS485data[numRegister]= {0}; // 存储接收到的数据
  int16_t wdata = 0;
  int16_t cnt = 10;
  int16_t unprocessedRegisters = numRegister - startAddr;
  bool islastRound = false;
  String ThisDateTime = getISO8601Time();
  Serial.println(ThisDateTime);

  while(unprocessedRegisters > 0)
  { 
    if(unprocessedRegisters < 10)
    {
      if (islastRound == true) break;

      cnt = unprocessedRegisters;
      islastRound = true;
    }

    result = RS485ReadHoldingRegisters(startAddr, cnt);
    
    if(result != 0)
    {
      for (int i = 0; i < cnt; i++)
      {
	      wdata = (reciveData[3 + i*2] << 8) + reciveData[3 + i*2 + 1];
        RS485data[startAddr + i] = wdata;
        Serial.printf("RS485data %d: %X \r\n", startAddr + i, RS485data[startAddr + i]);
      }
    }
    else
    {
      Serial.printf("Rs485WritePage Fail range : %d - %d \r\n", startAddr, (startAddr + cnt));
    }
    publishMessage2(RS485data, startAddr, (startAddr + cnt), ThisDateTime);
    
    startAddr = cnt + startAddr;
    unprocessedRegisters = numRegister - startAddr;
    Serial.println(unprocessedRegisters);
  }
  
 //區域結束
 //======================================================
  #endif
   if (Counters >= RestartCounter) // 重啟設備
   {
                Counters = 0; // 重計數
                ESP.restart();
   }

}

void checkPress()
{
    if (digitalRead(enablePin) == LOW)
    { // 如果按鈕被按下
        Serial.println("EN pressed button2");
        startTime = millis(); // 紀錄開始時間
        Serial.println("startTime...");
        Serial.println(startTime);
        while (digitalRead(enablePin) == LOW)
        { // 等待按鈕釋放
            Serial.println("millis...");
            Serial.println(millis());
            if (millis() - startTime >= intervalReset)
            { // 如果長按超過5秒
                Serial.println("EN pressed for more than 5 seconds, clear ROM and restart");
                ClearRom();
                ESP.restart(); // 重启ESP32
                // 執行長按動作
                Serial.println("按鈕被長按5秒");
                break; // 跳出while迴圈
            }
            else
            {
              Serial.println("NONONO");
                // ESP.restart(); // 重启ESP32 ClearRom
                //StartToUpdateFM();
            }
        }
    }
}

void publishMessagePaker(int16_t *data, int a, int b ,String ThisDateTime)
{
    StaticJsonDocument<1024> doc;
    String topicDeviceIdentifierString = deviceIdentifier;
    doc["macid"] = topicDeviceIdentifierString;  
    doc["model"] = "pa";
    
    topicDeviceIdentifierString.replace(":", "");
    doc["code"] = "ker";    
    doc["lat"] = 25.063173;
    doc["lon"] = 121.272976;
    doc["timestamp"] = ThisDateTime;
    // doc["timestamp"] = timeClient.getFormattedTime();//timeClient.getFormattedTime();
    
    // 创建 JSON 文档中的 data 对象
    JsonObject dataObject = doc.createNestedObject("data");
    
    // 如果 data 不是空指针，则处理数据
    if (data != nullptr) {
        // 填充 data 对象
        for (int i = a; i < b; i++) {
            if (data[i] != 0) { // 如果 data[i] 有数据
                dataObject[String(i)] = data[i];
                Serial.println(String(i));
                Serial.println(data[i]);
            }else
            {
               dataObject[String(i)] = 0;
            }
        }
    } else {
        Serial.println("Error: data pointer is null.");
    }
  
    // 将 JSON 数据序列化为字符串
    String jsonString;
    serializeJson(doc, jsonString);

    // 打印 JSON 数据
    Serial.println("JSON 数据:");
    Serial.println(jsonString);

    // 发布 MQTT 消息
    String publishTopic = AWS_IOT_PUBLISH_TOPIC;
    publishTopic.replace(":", "");
    Serial.println(publishTopic.c_str());

    // 转换 jsonString 为 const char* 以便发布
    client.publish(publishTopic.c_str(), jsonString.c_str(), false); // 发布消息，不保留
    
    Serial.print("已上传到 MQTT：");
    Serial.println(publishTopic);
}


void publishMessageHitachi(int16_t *data, int a, int b ,String ThisDateTime,String Model,String Code)
{
    StaticJsonDocument<1024> doc;
    String topicDeviceIdentifierString = deviceIdentifier;
    topicDeviceIdentifierString.replace(":", "");
    doc["macid"] = topicDeviceIdentifierString;      
    doc["model"] = Model;        
    doc["code"] = Code;    
    doc["lat"] = 25.063173;
    doc["lon"] = 121.272976;
    doc["timestamp"] = ThisDateTime;

    
    // 创建 JSON 文档中的 data 对象
    JsonObject dataObject = doc.createNestedObject("data");
    
    // 如果 data 不是空指针，则处理数据
    if (data != nullptr) {
        // 填充 data 对象
        for (int i = a; i < b; i++) {
            if (data[i] != 0) { // 如果 data[i] 有数据
                dataObject[String(i)] = data[i];
                Serial.println(String(i));
                Serial.println(data[i]);
            }else
            {
               dataObject[String(i)] = 0;
            }
        }
    } else {
        Serial.println("Error: data pointer is null.");
    }
  
    // 将 JSON 数据序列化为字符串
    String jsonString;
    serializeJson(doc, jsonString);

    // 打印 JSON 数据
    Serial.println("JSON 数据:");
    Serial.println(jsonString);

    // 发布 MQTT 消息
    String publishTopic = AWS_IOT_PUBLISH_TOPIC;
    publishTopic.replace(":", "");
    Serial.println(publishTopic.c_str());

    // 转换 jsonString 为 const char* 以便发布
    client.publish(publishTopic.c_str(), jsonString.c_str(), false); // 发布消息，不保留
    
    Serial.print("已上传到 MQTT：");
    Serial.println(publishTopic);
}


void publishMessage2(int16_t *data, int a, int b, String ThisDateTime)
{
    StaticJsonDocument<1024> doc;
    String topicDeviceIdentifierString = deviceIdentifier;
    topicDeviceIdentifierString.replace(":", "");
    doc["code"] = topicDeviceIdentifierString;
    doc["model"] = "DolomannJBTesting";
    doc["lat"] = 25.062785639021385;
    doc["lon"] = 121.28532285434154;
    doc["timestamp"] = ThisDateTime;
    JsonObject dataObject = doc.createNestedObject("data");

    // 检查 data 是否为空指针
    if (data != nullptr) {
        // 填充 data 对象
        for (int i = a; i < b; i++) {
            dataObject[String(i)] = data[i];
            Serial.print("Register ");
            Serial.print(i);
            Serial.print(": ");
            Serial.println(data[i]);
        }
    } else {
        Serial.println("Error: data pointer is null.");
        // 如果 data 是空指针，填充默认数据
        for (int i = a; i < b; i++) {
            dataObject[String(i)] = 0;
        }
    }

    // 将 JSON 数据序列化为字符串
    String jsonString;
    serializeJson(doc, jsonString);

    // 打印 JSON 数据
    Serial.println("JSON 数据:");
    Serial.println(jsonString);

    // 发布 MQTT 消息
    String publishTopic = AWS_IOT_PUBLISH_TOPIC;
    publishTopic.replace(":", "");
    client.publish(publishTopic.c_str(), jsonString.c_str()); // 发布消息
    Serial.print("已上传到 MQTT2：");
    Serial.println(publishTopic);
}
