
#ifndef _DEFINE_H_
#define _DEFINE_H_

//----------------------------------------------------------------------//
// UART Define
//----------------------------------------------------------------------//
// 软串口用于接TTL输入，接入甲醛传感器，波特率9600,主动上传，ESP32只用RX就行
#define SW_SERIAL_TXD -1
#define SW_SERIAL_RXD 5
// 串口1用于转接RS232，接入PM2.5传感器，波特率9600,主动上传，ESP32只用RX就行
#define SERIAL1_TXD 33
#define SERIAL1_RXD 25
// 串口2用于转接RS485，接入温湿度传感器，波特率9600，半双工应答式，一问一答
// #define Serial02_TXD 26   //ESP32
// #define Serial02_RXD 27   //ESP32
#define Serial02_TXD 15 // ESP8266
#define Serial02_RXD 13 // ESP8266

#define BAUDRATE9600 9600 // 距離 9600
#define BAUDRATE4800 4800 // PH 4800

#define BAUDRATE115200 115200 // RFID 115200


//----------------------------------------------------------------------//
// LCD Define
//----------------------------------------------------------------------//
#define TOTAL_REFRESH_COUNTER 20 * 1000       // ms，即20S，全刷间隔时间

/*
 * increase the packet size value from 256 to the size you need.
 * I had faced the same problem while sending data running on 8883,
 * the broker was getting broken data.
 */

#define MQTT_MAX_PACKET_SIZE 65000 // MQTT最大接收缓存




///--------------------------------------------------------///
/// event flag
///--------------------------------------------------------///
#define EVENT_NULL (0x00)
#define EVENT_TIMER_FETCH_CMD (0x01)
#define EVENT_START_CMD (0x02)
#define EVENT_STOP_CMD (0x04)

///--------------------------------------------------------///
/// EEPROM
///--------------------------------------------------------///
#define EEPROM_SIZE 512                                    // EEPROM 大小
#define EEPROM_SSID_ADDRESS 0                              // 存储SSID的起始地址
#define EEPROM_PASSWORD_ADDRESS (EEPROM_SSID_ADDRESS + 32) // 存储密码的起始地址，假设密码最长为32个字符
#define IMAGE_DATA_ADDRESS (EEPROM_PASSWORD_ADDRESS + 64)  // 存储图像数据的起始地址，根据需要调整
#define IMAGE_SIZE 15000                                   // 图像数据大小，根据实际情况调整

#endif  ///< _DEFINE_H_
