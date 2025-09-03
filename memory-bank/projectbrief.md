# ESP32 Multical21 Water Meter Reader Project

## Project Overview
This project implements a wireless water meter reader for Kamstrup Multical21 meters using an ESP32-C3 microcontroller and CC1101 radio module. The system receives encrypted WMBus packets from the water meter, decrypts them using AES-128, and publishes the data via MQTT.

## Core Requirements
- Read encrypted WMBus packets from Kamstrup Multical21 water meters
- Decrypt packets using AES-128 encryption with user-provided key
- Extract water consumption data (total, target, temperatures, info codes)
- Publish data via MQTT for home automation integration
- Support WiFi connectivity with multiple credential options

## Hardware Setup
- ESP32-C3 microcontroller
- CC1101 868MHz radio transceiver module
- SPI communication between ESP32 and CC1101
- GPIO3 used for packet reception interrupt (GDO0)

## Key Technical Components
- WMBus Mode C1 packet reception
- AES-128 CTR mode decryption
- CRC validation using EN13575 standard
- MQTT publishing with configurable topics
- OTA firmware updates support

## Current Status
The project was experiencing issues with packet decryption where packets were being received but not processed due to strict preamble filtering. The code has been updated to process packets with any preamble and provide detailed debugging information for troubleshooting encryption issues.
