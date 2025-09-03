# Hardware Documentation

## Current Hardware Setup

This project currently uses the following hardware components:

### Microcontroller: ESP32-C3 Super Mini

The ESP32-C3 Super Mini is a compact development board based on the ESP32-C3 SoC (System on Chip). Key specifications:

- **CPU**: 32-bit RISC-V single-core processor, up to 160 MHz
- **Memory**: 400 KB SRAM, 384 KB ROM
- **Flash**: 4 MB (typical)
- **Connectivity**: Wi-Fi 802.11 b/g/n, Bluetooth 5 (LE)
- **GPIO**: Multiple digital I/O pins
- **Interfaces**: SPI, I2C, UART
- **Power**: 3.3V operating voltage
- **Form Factor**: Ultra-compact design suitable for space-constrained applications

### Radio Module: CC1101

The CC1101 is a low-power sub-1 GHz RF transceiver designed for very low-power wireless applications. Key specifications:

- **Frequency Range**: 300-348 MHz, 387-464 MHz, and 779-928 MHz
- **Modulation**: 2-FSK, 4-FSK, GFSK, MSK, OOK, ASK
- **Data Rate**: Up to 500 kBaud
- **Power Consumption**: Very low power consumption in all modes
- **Interface**: SPI interface for configuration and data transfer
- **Sensitivity**: High receiver sensitivity
- **Output Power**: Programmable output power up to +12 dBm

## Hardware Connections

### SPI Interface Wiring

The ESP32-C3 Super Mini is connected to the CC1101 module via SPI (Serial Peripheral Interface):

| ESP32-C3 Pin | CC1101 Pin | Function |
|--------------|------------|----------|
| GPIO4        | SCK        | SPI Clock |
| GPIO6        | MOSI       | Master Out Slave In |
| GPIO5        | MISO       | Master In Slave Out |
| GPIO7        | CSN        | Chip Select (Active Low) |
| 3.3V         | VCC        | Power Supply |
| GND          | GND        | Ground |

*Note: Pin assignments may vary depending on your specific wiring configuration. Verify connections match your code configuration.*

### Additional Connections

- **GDO0**: General Digital Output 0 from CC1101 (used for interrupt/status signaling)
- **GDO2**: General Digital Output 2 from CC1101 (optional, for additional status signals)

## Power Requirements

- **Operating Voltage**: 3.3V for both modules
- **Current Consumption**:
  - ESP32-C3: ~20-40mA during active operation, <10µA in deep sleep
  - CC1101: ~15mA in RX mode, ~30mA in TX mode, <1µA in power-down mode

## Physical Considerations

- Ensure proper antenna connection to CC1101 for optimal RF performance
- Keep RF traces short and use proper ground planes
- Consider EMI/EMC requirements for your application
- Provide adequate power supply decoupling capacitors

## Usage Notes

This hardware combination is well-suited for:
- Low-power wireless sensor networks
- Remote monitoring applications
- IoT devices requiring sub-GHz communication
- Battery-powered applications with Wi-Fi connectivity

The ESP32-C3 handles the main application logic and Wi-Fi connectivity, while the CC1101 provides reliable sub-GHz RF communication for sensor data transmission.
