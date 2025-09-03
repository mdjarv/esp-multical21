/*
 Copyright (C) 2020 chester4444@wolke7.net
 This program is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.
 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 You should have received a copy of the GNU General Public License
 along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "WaterMeter.h"

WaterMeter::WaterMeter(PubSubClient &mqtt)
  : mqttClient  (mqtt)
  , mqttEnabled (false)
{
}

void WaterMeter::enableMqtt(bool enabled)
{
  mqttEnabled = enabled;
}

// ChipSelect assert
inline void WaterMeter::selectCC1101(void)
{
#if defined(ESP32)
  digitalWrite(CC1101_CS, LOW);
#else
  digitalWrite(SS, LOW);
#endif
}

// ChipSelect deassert
inline void WaterMeter::deselectCC1101(void)
{
#if defined(ESP32)
  digitalWrite(CC1101_CS, HIGH);
#else
  digitalWrite(SS, HIGH);
#endif
}

// wait for MISO pulling down
inline void WaterMeter::waitMiso(void)
{
#if defined(ESP32)
  while (digitalRead(CC1101_MISO) == HIGH)
    ;
#else
  while (digitalRead(MISO) == HIGH)
    ;
#endif
}

// write a single register of CC1101
void WaterMeter::writeReg(uint8_t regAddr, uint8_t value)
{
  selectCC1101();
  waitMiso();            // Wait until MISO goes low
  SPI.transfer(regAddr); // Send register address
  SPI.transfer(value);   // Send value
  deselectCC1101();
}

// send a strobe command to CC1101
void WaterMeter::cmdStrobe(uint8_t cmd)
{
  selectCC1101();
  delayMicroseconds(5);
  waitMiso();        // Wait until MISO goes low
  SPI.transfer(cmd); // Send strobe command
  delayMicroseconds(5);
  deselectCC1101();
}

// read CC1101 register (status or configuration)
uint8_t WaterMeter::readReg(uint8_t regAddr, uint8_t regType)
{
  uint8_t addr, val;

  addr = regAddr | regType;
  selectCC1101();
  waitMiso();               // Wait until MISO goes low
  SPI.transfer(addr);       // Send register address
  val = SPI.transfer(0x00); // Read result
  deselectCC1101();

  return val;
}

//
void WaterMeter::readBurstReg(uint8_t *buffer, uint8_t regAddr, uint8_t len)
{
  uint8_t addr, i;

  addr = regAddr | READ_BURST;
  selectCC1101();
  delayMicroseconds(5);
  waitMiso();         // Wait until MISO goes low
  SPI.transfer(addr); // Send register address
  for (i = 0; i < len; i++)
    buffer[i] = SPI.transfer(0x00); // Read result byte by byte
  delayMicroseconds(2);
  deselectCC1101();
}

// power on reset
void WaterMeter::reset(void)
{
  deselectCC1101();
  delayMicroseconds(3);

#if defined(ESP32)
  digitalWrite(CC1101_MOSI, LOW);
  digitalWrite(CC1101_SCK, HIGH); // see CC1101 datasheet 11.3
#else
  digitalWrite(MOSI, LOW);
  digitalWrite(SCK, HIGH); // see CC1101 datasheet 11.3
#endif

  selectCC1101();
  delayMicroseconds(3);
  deselectCC1101();
  delayMicroseconds(45); // at least 40 us

  selectCC1101();

  waitMiso();                // Wait until MISO goes low
  SPI.transfer(CC1101_SRES); // Send reset command strobe
  waitMiso();                // Wait until MISO goes low

  deselectCC1101();
}

// set IDLE state, flush FIFO and (re)start receiver
void WaterMeter::startReceiver(void)
{
  uint8_t regCount = 0;
  cmdStrobe(CC1101_SIDLE); // Enter IDLE state
  while (readReg(CC1101_MARCSTATE, CC1101_STATUS_REGISTER) != MARCSTATE_IDLE)
  {
    if (regCount++ > 100)
    {
      Serial.println("Enter idle state failed!\n");
      restartRadio();
    }
  }

  cmdStrobe(CC1101_SFRX); // flush receive queue
  delay(5);

  regCount = 0;
  cmdStrobe(CC1101_SRX); // Enter RX state
  delay(10);
  while (readReg(CC1101_MARCSTATE, CC1101_STATUS_REGISTER) != MARCSTATE_RX)
  {
    if (regCount++ > 100)
    {
      Serial.println("Enter RX state failed!\n");
      restartRadio();
    }
  }
}

// initialize all the CC1101 registers
void WaterMeter::initializeRegisters(void)
{
  writeReg(CC1101_IOCFG2, CC1101_DEFVAL_IOCFG2);
  writeReg(CC1101_IOCFG0, CC1101_DEFVAL_IOCFG0);
  writeReg(CC1101_FIFOTHR, CC1101_DEFVAL_FIFOTHR);
  writeReg(CC1101_PKTLEN, CC1101_DEFVAL_PKTLEN);
  writeReg(CC1101_PKTCTRL1, CC1101_DEFVAL_PKTCTRL1);
  writeReg(CC1101_PKTCTRL0, CC1101_DEFVAL_PKTCTRL0);
  writeReg(CC1101_SYNC1, CC1101_DEFVAL_SYNC1);
  writeReg(CC1101_SYNC0, CC1101_DEFVAL_SYNC0);
  writeReg(CC1101_ADDR, CC1101_DEFVAL_ADDR);
  writeReg(CC1101_CHANNR, CC1101_DEFVAL_CHANNR);
  writeReg(CC1101_FSCTRL1, CC1101_DEFVAL_FSCTRL1);
  writeReg(CC1101_FSCTRL0, CC1101_DEFVAL_FSCTRL0);
  writeReg(CC1101_FREQ2, CC1101_DEFVAL_FREQ2);
  writeReg(CC1101_FREQ1, CC1101_DEFVAL_FREQ1);
  writeReg(CC1101_FREQ0, CC1101_DEFVAL_FREQ0);
  writeReg(CC1101_MDMCFG4, CC1101_DEFVAL_MDMCFG4);
  writeReg(CC1101_MDMCFG3, CC1101_DEFVAL_MDMCFG3);
  writeReg(CC1101_MDMCFG2, CC1101_DEFVAL_MDMCFG2);
  writeReg(CC1101_MDMCFG1, CC1101_DEFVAL_MDMCFG1);
  writeReg(CC1101_MDMCFG0, CC1101_DEFVAL_MDMCFG0);
  writeReg(CC1101_DEVIATN, CC1101_DEFVAL_DEVIATN);
  writeReg(CC1101_MCSM1, CC1101_DEFVAL_MCSM1);
  writeReg(CC1101_MCSM0, CC1101_DEFVAL_MCSM0);
  writeReg(CC1101_FOCCFG, CC1101_DEFVAL_FOCCFG);
  writeReg(CC1101_BSCFG, CC1101_DEFVAL_BSCFG);
  writeReg(CC1101_AGCCTRL2, CC1101_DEFVAL_AGCCTRL2);
  writeReg(CC1101_AGCCTRL1, CC1101_DEFVAL_AGCCTRL1);
  writeReg(CC1101_AGCCTRL0, CC1101_DEFVAL_AGCCTRL0);
  writeReg(CC1101_FREND1, CC1101_DEFVAL_FREND1);
  writeReg(CC1101_FREND0, CC1101_DEFVAL_FREND0);
  writeReg(CC1101_FSCAL3, CC1101_DEFVAL_FSCAL3);
  writeReg(CC1101_FSCAL2, CC1101_DEFVAL_FSCAL2);
  writeReg(CC1101_FSCAL1, CC1101_DEFVAL_FSCAL1);
  writeReg(CC1101_FSCAL0, CC1101_DEFVAL_FSCAL0);
  writeReg(CC1101_FSTEST, CC1101_DEFVAL_FSTEST);
  writeReg(CC1101_TEST2, CC1101_DEFVAL_TEST2);
  writeReg(CC1101_TEST1, CC1101_DEFVAL_TEST1);
  writeReg(CC1101_TEST0, CC1101_DEFVAL_TEST0);
}

IRAM_ATTR void WaterMeter::instanceCC1101Isr()
{
  // set the flag that a package is available
  packetAvailable = true;
}

// static ISR method, that calls the right instance
IRAM_ATTR void WaterMeter::cc1101Isr(void *p)
{
  WaterMeter *ptr = (WaterMeter *)p;
  ptr->instanceCC1101Isr();
}

// should be called frequently, handles the ISR flag
// does the frame checkin and decryption
void WaterMeter::loop(void)
{
  static unsigned long lastDebugOutput = 0;

  // Check for packets via interrupt (GDO0 connected to GPIO3)
  if (packetAvailable)
  {
    //  Disable wireless reception interrupt
    detachInterrupt(digitalPinToInterrupt(CC1101_GDO0));

    // clear the flag
    packetAvailable = false;
    receive();

    // Enable wireless reception interrupt
    attachInterruptArg(digitalPinToInterrupt(CC1101_GDO0), cc1101Isr, this, FALLING);
  }

  // Periodic debug output every 10 seconds
  if (millis() - lastDebugOutput > 10000)
  {
    lastDebugOutput = millis();
    uint8_t marcState = readReg(CC1101_MARCSTATE, CC1101_STATUS_REGISTER);
    uint8_t rxBytes = readReg(CC1101_RXBYTES, CC1101_STATUS_REGISTER);
    uint8_t rssi = readReg(CC1101_RSSI, CC1101_STATUS_REGISTER);

    Serial.printf("CC1101 Status - MARC: 0x%02X, RX bytes: %d, RSSI: %d dBm\n",
                  marcState, rxBytes & 0x7F, (rssi >= 128) ? (rssi - 256) / 2 - 74 : rssi / 2 - 74);

    // Check if we're still in RX mode
    if (marcState != MARCSTATE_RX)
    {
      Serial.printf("Not in RX mode (state: 0x%02X), restarting receiver...\n", marcState);
      startReceiver();
    }

    // Check for FIFO overflow (shouldn't happen with proper GDO0 interrupt handling)
    if (rxBytes & 0x80)
    {
      Serial.println("Warning: RX FIFO overflow detected! Restarting receiver...");
      startReceiver();
    }
  }

  if (millis() - lastFrameReceived > RECEIVE_TIMEOUT)
  {
    // workaround: reset CC1101, since it stops receiving from time to time
    Serial.println("Receive timeout, restarting radio...");
    restartRadio();
  }
}

// Initialize CC1101 to receive WMBus MODE C1
void WaterMeter::begin(uint8_t *key, uint8_t *id)
{
  pinMode(SS, OUTPUT);         // SS Pin -> Output
  SPI.begin();                 // Initialize SPI interface
  pinMode(CC1101_GDO0, INPUT); // Config GDO0 as input

  memcpy(aesKey, key, sizeof(aesKey));
  aes128.setKey(aesKey, sizeof(aesKey));
  pinMode(SS, OUTPUT);                // SS Pin -> Output
  memcpy(meterId, id, sizeof(meterId));


  attachInterruptArg(digitalPinToInterrupt(CC1101_GDO0), cc1101Isr, this, FALLING);
  lastFrameReceived = millis();
}

void WaterMeter::restartRadio()
{
  Serial.println("Resetting CC1101...");
  Serial.printf("GDO0 interrupt pin configured on GPIO%d\n", CC1101_GDO0);

  reset(); // power on CC1101

  initializeRegisters(); // init CC1101 registers

  cmdStrobe(CC1101_SCAL);
  delay(1);

  startReceiver();
  lastFrameReceived = millis();
  Serial.println("CC1101 ready for WMBus reception");
}

bool WaterMeter::checkFrame(void)
{
#if DEBUG
  Serial.printf("frame serial ID: ");
  for (uint8_t i = 0; i < 4; i++)
  {
    Serial.printf("%02x", payload[7-i]);
  }
  Serial.printf(" - %d", length);
  Serial.println();
#endif

  // check meterId
  for (uint8_t i = 0; i < 4; i++)
  {
    if (meterId[i] != payload[7 - i])
    {
#if DEBUG
      Serial.println("Meter serial doesnt match!");
#endif
      return false;
    }
  }

#if DEBUG
  Serial.println("Frame payload:");
  for (uint8_t i = 0; i <= length; i++)
  {
    Serial.printf("%02x", payload[i]);
  }
  Serial.println();
#endif

  uint16_t crc = crcEN13575(payload, length - 1); // -2 (CRC) + 1 (L-field)
  if (crc != (payload[length - 1] << 8 | payload[length]))
  {
    Serial.println("CRC Error");
    Serial.printf("%04x - %02x%02x\n", crc, payload[length - 1], payload[length]);
    return false;
  }

  return true;
}

// Publish Home Assistant MQTT Discovery configuration
void WaterMeter::publishHomeAssistantDiscovery(void)
{
  if (!mqttEnabled) return;

#if DEBUG >= 1
  Serial.println("Publishing Home Assistant MQTT Discovery...");
#endif

  // Create unique device identifier based on ESP MAC address
  String deviceId = WiFi.macAddress();
  deviceId.replace(":", "");
  deviceId.toLowerCase();

  // Device configuration (shared by all entities)
  String deviceConfig = "\"device\":{";
  deviceConfig += "\"identifiers\":[\"" + deviceId + "\"],";
  deviceConfig += "\"name\":\"" + String(DEVICE_NAME) + "\",";
  deviceConfig += "\"model\":\"" + String(DEVICE_MODEL) + "\",";
  deviceConfig += "\"manufacturer\":\"" + String(DEVICE_MANUFACTURER) + "\",";
  deviceConfig += "\"sw_version\":\"" + String(DEVICE_SW_VERSION) + "\",";
  deviceConfig += "\"configuration_url\":\"http://" + WiFi.localIP().toString() + "\",";
  deviceConfig += "\"suggested_area\":\"Utility Room\"";
  deviceConfig += "}";

  // Origin configuration (required for device discovery)
  String originConfig = "\"origin\":{";
  originConfig += "\"name\":\"" + String(DEVICE_MANUFACTURER) + "\",";
  originConfig += "\"sw\":\"" + String(DEVICE_SW_VERSION) + "\",";
  originConfig += "\"url\":\"https://github.com/chester4444/esp-multical21\"";
  originConfig += "}";

  // Total Water Consumption Sensor
  String totalConfig = "{";
  totalConfig += "\"platform\":\"sensor\",";  // Required field
  totalConfig += "\"name\":\"Water Total Consumption\",";
  totalConfig += "\"unique_id\":\"" + deviceId + "_water_total\",";
  totalConfig += "\"state_topic\":\"" + String(MQTT_PREFIX) + String(MQTT_total) + "\",";
  totalConfig += "\"device_class\":\"water\",";
  totalConfig += "\"unit_of_measurement\":\"m³\",";
  totalConfig += "\"state_class\":\"total_increasing\",";
  totalConfig += "\"icon\":\"mdi:water\",";
  totalConfig += originConfig + ",";
  totalConfig += deviceConfig;
  totalConfig += "}";

  // Target Water Consumption Sensor
  String targetConfig = "{";
  targetConfig += "\"platform\":\"sensor\",";  // Required field
  targetConfig += "\"name\":\"Water Target Consumption\",";
  targetConfig += "\"unique_id\":\"" + deviceId + "_water_target\",";
  targetConfig += "\"state_topic\":\"" + String(MQTT_PREFIX) + String(MQTT_target) + "\",";
  targetConfig += "\"device_class\":\"water\",";
  targetConfig += "\"unit_of_measurement\":\"m³\",";
  targetConfig += "\"state_class\":\"total\",";
  targetConfig += "\"icon\":\"mdi:water-outline\",";
  targetConfig += originConfig + ",";
  targetConfig += deviceConfig;
  targetConfig += "}";

  // Flow Temperature Sensor
  String flowTempConfig = "{";
  flowTempConfig += "\"platform\":\"sensor\",";  // Required field
  flowTempConfig += "\"name\":\"Water Flow Temperature\",";
  flowTempConfig += "\"unique_id\":\"" + deviceId + "_flow_temp\",";
  flowTempConfig += "\"state_topic\":\"" + String(MQTT_PREFIX) + String(MQTT_ftemp) + "\",";
  flowTempConfig += "\"device_class\":\"temperature\",";
  flowTempConfig += "\"unit_of_measurement\":\"°C\",";
  flowTempConfig += "\"state_class\":\"measurement\",";
  flowTempConfig += "\"icon\":\"mdi:thermometer\",";
  flowTempConfig += originConfig + ",";
  flowTempConfig += deviceConfig;
  flowTempConfig += "}";

  // Ambient Temperature Sensor
  String ambientTempConfig = "{";
  ambientTempConfig += "\"platform\":\"sensor\",";  // Required field
  ambientTempConfig += "\"name\":\"Water Ambient Temperature\",";
  ambientTempConfig += "\"unique_id\":\"" + deviceId + "_ambient_temp\",";
  ambientTempConfig += "\"state_topic\":\"" + String(MQTT_PREFIX) + String(MQTT_atemp) + "\",";
  ambientTempConfig += "\"device_class\":\"temperature\",";
  ambientTempConfig += "\"unit_of_measurement\":\"°C\",";
  ambientTempConfig += "\"state_class\":\"measurement\",";
  ambientTempConfig += "\"icon\":\"mdi:thermometer-lines\",";
  ambientTempConfig += originConfig + ",";
  ambientTempConfig += deviceConfig;
  ambientTempConfig += "}";

  // Info Code Sensor
  String infoConfig = "{";
  infoConfig += "\"platform\":\"sensor\",";  // Required field
  infoConfig += "\"name\":\"Water Meter Info Code\",";
  infoConfig += "\"unique_id\":\"" + deviceId + "_info_code\",";
  infoConfig += "\"state_topic\":\"" + String(MQTT_PREFIX) + String(MQTT_info) + "\",";
  infoConfig += "\"icon\":\"mdi:information\",";
  infoConfig += originConfig + ",";
  infoConfig += deviceConfig;
  infoConfig += "}";

  // Publish discovery messages with correct topic format
  String discoveryPrefix = String(HA_DISCOVERY_PREFIX) + "/sensor/" + deviceId;

#if DEBUG >= 1
  Serial.println("Publishing Home Assistant MQTT Discovery messages:");
  Serial.printf("Device ID: %s\n", deviceId.c_str());
  Serial.printf("Discovery prefix: %s\n", discoveryPrefix.c_str());
#endif

  // Publish each discovery message with detailed logging
  String topics[] = {
    discoveryPrefix + "/water_total/config",
    discoveryPrefix + "/water_target/config",
    discoveryPrefix + "/flow_temp/config",
    discoveryPrefix + "/ambient_temp/config",
    discoveryPrefix + "/info_code/config"
  };

  String configs[] = {totalConfig, targetConfig, flowTempConfig, ambientTempConfig, infoConfig};
  String names[] = {"Water Total", "Water Target", "Flow Temp", "Ambient Temp", "Info Code"};

#if DEBUG >= 1
  Serial.printf("MQTT buffer size: %d bytes\n", mqttClient.getBufferSize());
#endif

  for (int i = 0; i < 5; i++) {
    size_t configSize = configs[i].length();

#if DEBUG >= 1
    Serial.printf("Publishing %s (%d bytes)...", names[i].c_str(), configSize);
#endif

    bool published = mqttClient.publish(topics[i].c_str(), configs[i].c_str(), true);

#if DEBUG >= 1
    Serial.printf(" %s\n", published ? "✓" : "✗");
    if (!published) {
      Serial.printf("  Topic: %s\n", topics[i].c_str());
      Serial.printf("  Payload size: %d bytes\n", configSize);
      Serial.printf("  Buffer size: %d bytes\n", mqttClient.getBufferSize());
      if (configSize > mqttClient.getBufferSize()) {
        Serial.println("  ERROR: Payload exceeds buffer size!");
      }
    }
#endif

    mqttClient.loop();
    delay(100);
  }

#if DEBUG >= 1
  Serial.println("Home Assistant MQTT Discovery publishing completed");
#endif
}

// Publish availability status
void WaterMeter::publishAvailability(bool online)
{
  if (!mqttEnabled) return;

  String availabilityTopic = String(MQTT_PREFIX) + "/availability";
  const char* payload = online ? "\"online\"" : "\"offline\"";

  mqttClient.publish(availabilityTopic.c_str(), payload, true);
  mqttClient.loop();

#if DEBUG >= 1
  Serial.printf("Published availability: %s\n", payload);
#endif
}

void WaterMeter::getMeterInfo(uint8_t *data, size_t len)
{
  // init positions for compact frame
  int pos_tt = 9;  // total consumption
  int pos_tg = 13; // target consumption
  int pos_ic = 7;  // info codes
  int pos_ft = 17; // flow temp
  int pos_at = 18; // ambient temp

  if (data[2] == 0x78) // long frame
  {
    // overwrite it with long frame positions
    pos_tt = 10;
    pos_tg = 16;
    pos_ic = 6;
    pos_ft = 22;
    pos_at = 25;
  }

  totalWater = data[pos_tt] + (data[pos_tt + 1] << 8) + (data[pos_tt + 2] << 16) + (data[pos_tt + 3] << 24);

  targetWater = data[pos_tg] + (data[pos_tg + 1] << 8) + (data[pos_tg + 2] << 16) + (data[pos_tg + 3] << 24);

  flowTemp = data[pos_ft];
  ambientTemp = data[pos_at];
  infoCodes = data[pos_ic];
}

void WaterMeter::publishMeterInfo()
{
  char total[12];
  snprintf(total, sizeof(total), "%d.%03d", totalWater/ 1000, totalWater % 1000);
  Serial.printf("total: %s m%c - ", total, 179);

  char target[12];
  snprintf(target, sizeof(target), "%d.%03d", targetWater / 1000, targetWater % 1000);
  Serial.printf("target: %s m%c - ", target, 179);

  char flow_temp[4];
  snprintf(flow_temp, sizeof(flow_temp), "%2d", flowTemp);
  Serial.printf("%s %cC - ", flow_temp, 176);

  char ambient_temp[4];
  snprintf(ambient_temp, sizeof(ambient_temp), "%2d", ambientTemp);
  Serial.printf("%s %cC - ", ambient_temp, 176);

  // Map info codes to meaningful states for Home Assistant
  char info_codes_display[16];
  char info_codes_mqtt[16];

  switch(infoCodes) {
    case 0x00:
      snprintf(info_codes_display, sizeof(info_codes_display), "normal");
      snprintf(info_codes_mqtt, sizeof(info_codes_mqtt), "normal");
      break;
    case 0x01:
      snprintf(info_codes_display, sizeof(info_codes_display), "dry");
      snprintf(info_codes_mqtt, sizeof(info_codes_mqtt), "dry");
      break;
    case 0x02:
      snprintf(info_codes_display, sizeof(info_codes_display), "reverse");
      snprintf(info_codes_mqtt, sizeof(info_codes_mqtt), "reverse");
      break;
    case 0x04:
      snprintf(info_codes_display, sizeof(info_codes_display), "leak");
      snprintf(info_codes_mqtt, sizeof(info_codes_mqtt), "leak");
      break;
    case 0x08:
      snprintf(info_codes_display, sizeof(info_codes_display), "burst");
      snprintf(info_codes_mqtt, sizeof(info_codes_mqtt), "burst");
      break;
    default:
      snprintf(info_codes_display, sizeof(info_codes_display), "0x%02x", infoCodes);
      snprintf(info_codes_mqtt, sizeof(info_codes_mqtt), "code_0x%02x", infoCodes);
      break;
  }

  Serial.printf("%s \n\r", info_codes_display);

  if (!mqttEnabled) return; // no MQTT broker connected, leave

  // Publish numeric values as JSON numbers (no quotes)
  char total_json[12];
  snprintf(total_json, sizeof(total_json), "%.3f", totalWater / 1000.0);

  char target_json[12];
  snprintf(target_json, sizeof(target_json), "%.3f", targetWater / 1000.0);

  char flow_temp_json[8];
  snprintf(flow_temp_json, sizeof(flow_temp_json), "%d", flowTemp);

  char ambient_temp_json[8];
  snprintf(ambient_temp_json, sizeof(ambient_temp_json), "%d", ambientTemp);

  // change the topics as you like
  mqttClient.publish(MQTT_PREFIX MQTT_total, total_json);
  mqttClient.publish(MQTT_PREFIX MQTT_target, target_json);
  mqttClient.loop();
  mqttClient.publish(MQTT_PREFIX MQTT_ftemp, flow_temp_json);
  mqttClient.publish(MQTT_PREFIX MQTT_atemp, ambient_temp_json);
  mqttClient.publish(MQTT_PREFIX MQTT_info, info_codes_mqtt);
  mqttClient.loop();
}

// reads a single byte from the RX fifo
uint8_t WaterMeter::readByteFromFifo(void)
{
  return readReg(CC1101_RXFIFO, CC1101_CONFIG_REGISTER);
}

// handles a received frame and restart the CC1101 receiver
void WaterMeter::receive()
{
  // read preamble
  uint8_t p1 = readByteFromFifo();
  uint8_t p2 = readByteFromFifo();

#if DEBUG >= 1
  Serial.printf("Packet received - Preamble: %02X%02X", p1, p2);
#endif

  // get length
  payload[0] = readByteFromFifo();

#if DEBUG >= 1
  Serial.printf(" Length: %02X", payload[0]);
#endif

  if (payload[0] < MAX_LENGTH)
  {
    // Read the rest of the data regardless of preamble
    for (int i = 0; i < payload[0] && i < MAX_LENGTH - 1; i++)
    {
      payload[i + 1] = readByteFromFifo();
    }

#if DEBUG >= 2
    // Show raw packet data only in verbose mode
    Serial.printf(" Raw packet (%d bytes): ", payload[0] + 3);
    Serial.printf("%02X%02X%02X", p1, p2, payload[0]);
    for (int i = 0; i < payload[0] && i < MAX_LENGTH - 1; i++)
    {
      Serial.printf("%02X", payload[i + 1]);
    }
    Serial.println();
#endif

    // Try to process any packet that looks like WMBus
    if (payload[0] >= 10) // Minimum reasonable packet size for WMBus
    {
#if DEBUG >= 1
      Serial.println(" - Processing...");
#endif

      // 3rd byte is payload length
      length = payload[0];

      // Try to decrypt and process the packet
      if (processWMBusPacket())
      {
#if DEBUG >= 1
        Serial.println("✓ Packet successfully processed!");
#endif
        lastPacketDecoded = millis();
        lastFrameReceived = millis();
      }
      else
      {
#if DEBUG >= 1
        Serial.println("✗ Packet processing failed");
#endif
      }
    }
#if DEBUG >= 1
    else
    {
      Serial.println(" - Packet too short, ignoring");
    }
#endif
  }
  else
  {
#if DEBUG >= 1
    Serial.printf(" Invalid length: %d (max: %d)\n", payload[0], MAX_LENGTH);
#endif
  }

  // flush RX fifo and restart receiver
  startReceiver();
}

// Process and decrypt WMBus packet regardless of preamble
bool WaterMeter::processWMBusPacket(void)
{
#if DEBUG >= 2
  // Print detailed packet analysis only in verbose mode
  Serial.printf("Processing packet - Length: %d bytes\n", length);
#endif

  // Check if this packet is for our meter
  if (length >= 8)
  {
    bool meterMatch = true;
    for (uint8_t i = 0; i < 4; i++)
    {
      if (meterId[i] != payload[7 - i])
      {
        meterMatch = false;
        break;
      }
    }

    if (!meterMatch)
    {
#if DEBUG >= 2
      // Print meter ID comparison only in verbose mode
      Serial.printf("Packet meter ID: ");
      for (uint8_t i = 0; i < 4; i++)
      {
        Serial.printf("%02X", payload[7-i]);
      }
      Serial.printf(" (expected: ");
      for (uint8_t i = 0; i < 4; i++)
      {
        Serial.printf("%02X", meterId[i]);
      }
      Serial.println(") - skipping");
#endif
      return false;
    }

#if DEBUG >= 2
    Serial.println("Meter ID matches! Attempting decryption...");
#endif
  }
  else
  {
#if DEBUG >= 1
    Serial.println("Packet too short for meter ID check");
#endif
    return false;
  }

  // Check if packet is long enough for encryption
  if (length < 18) // Need at least header + some encrypted data
  {
#if DEBUG >= 1
    Serial.println("Packet too short for encrypted data");
#endif
    return false;
  }

  // Calculate CRC and verify
  uint16_t crc = crcEN13575(payload, length - 1); // -2 (CRC) + 1 (L-field)
  uint16_t packetCrc = (payload[length - 1] << 8) | payload[length];

  if (crc != packetCrc)
  {
#if DEBUG >= 1
    Serial.printf("CRC mismatch: calculated=0x%04X, packet=0x%04X\n", crc, packetCrc);
#endif
    return false;
  }

#if DEBUG >= 2
  Serial.printf("CRC OK (0x%04X) - attempting decryption\n", crc);
#endif

  // Extract cipher data (starts at index 17, after header)
  uint8_t cipherLength = length - 2 - 16; // cipher starts at index 16, remove 2 crc bytes
  if (cipherLength > MAX_LENGTH - 17)
  {
#if DEBUG >= 1
    Serial.printf("Cipher too long: %d bytes\n", cipherLength);
#endif
    return false;
  }

  memcpy(cipher, &payload[17], cipherLength);

  // Build IV for decryption
  memset(iv, 0, sizeof(iv)); // padding with 0
  memcpy(iv, &payload[2], 8);  // M-field + A-field
  iv[8] = payload[11];         // CI-field
  memcpy(&iv[9], &payload[13], 4); // Access number + status + configuration

#if DEBUG >= 2
  // Show encryption details only in verbose mode
  Serial.printf("IV: ");
  for (int i = 0; i < 16; i++)
  {
    Serial.printf("%02X", iv[i]);
  }
  Serial.println();

  Serial.printf("Cipher (%d bytes): ", cipherLength);
  for (int i = 0; i < cipherLength; i++)
  {
    Serial.printf("%02X", cipher[i]);
  }
  Serial.println();
#endif

  // Decrypt the data
  aes128.setIV(iv, sizeof(iv));
  aes128.decrypt(plaintext, (const uint8_t *)cipher, cipherLength);

#if DEBUG >= 2
  Serial.printf("Plaintext (%d bytes): ", cipherLength);
  for (int i = 0; i < cipherLength; i++)
  {
    Serial.printf("%02X", plaintext[i]);
  }
  Serial.println();
#endif

  // Extract meter information from decrypted data
  getMeterInfo(plaintext, cipherLength);
  publishMeterInfo();

  return true;
}
