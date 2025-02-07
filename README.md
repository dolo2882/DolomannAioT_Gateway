# DolomannAIoT Gateway

## Description

The DolomannAIoT Gateway is an Arduino ESP8266-based device designed to collect sensor data via Modbus RTU over RS485, transmit the data to AWS IoT Core, and provide a web interface for configuration and firmware updates.

## Hardware

This project is designed for the Arduino ESP8266.

## Installation

1.  Obtain the ESP8266 board library from `http://arduino.esp8266.com/stable/package_esp8266com_index.json`.
2.  Extract the contents of `libraries.rar` and `libraries_SMB.rar` and place them into your Arduino libraries folder.

## Features

*   Collects sensor data via Modbus RTU over RS485.
*   Transmits data to AWS IoT Core.
*   Provides a web interface for WiFi configuration.
*   Supports firmware updates via a web interface.
*   Includes a reset button for clearing ROM and restarting the device.