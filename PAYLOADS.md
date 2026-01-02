# Sidewalk Asset Tracker Payload Specifications

This document describes the uplink and downlink message payloads used by the asset tracker.

**Updated for AWS IoT Core Device Location Integration:**
- Removed manual fragmentation logic - AWS IoT Core Device Location service handles larger payloads natively
- Payload sizes can now exceed the previous 19-byte limit (up to 255 bytes)
- WiFi payloads can include up to 32 access points in a single message
- GNSS payloads include complete NAV message data without fragmentation
- Simplified message structure for better cloud integration  

## Uplink Message Types

| TYPE Value | Name | Description |
| :--: | :--: | :-- |
| 0 | CONFIG | Describes the current configuration of the asset tracker |
| 1 | NOLOC | Current ambient data only.  Sent if no location data is available. |
| 2 | WIFI | Current ambient and WiFi location data | 
| 3 | GNSS | Current ambient and partial GNSS location data in a sequence of messages | 

The devices can send two different types of uplink messages: CONFIG and data (NOLOC, WIFI, GNSS).  The config message is sent at device start and on response to a config downlink.  The data messags are sent at regular intervals determined by the configuration or are triggered by the USER button on the WioTracker 1110.

### CONFIG Uplink Message Format
|      |       |       |       |       |       |       |       |       |       |
| :--- | :---: | :---: | :---: | :---: | :---: | :---: | :---: | :---: | :---: |
| **Name** | Type | Device Time | SW Version | Stored Rec Count | Link / Location Mode | Motion Period | Motion Sampling Freq | Motion Threshold | Static Sampling Freq |
| **Position** | 0x00 | Bytes 2 - 5 | Byte 6 | Bytes 7 - 8 | Byte 9 | Byte 10 | Byte 11 | Byte 12 | Byte 13 |

Description:

| Byte Offset | Name | Data Type | Description |
| :--: | :--  | :-------: | :---------- |
| 0 | Type | uint_8 | CONFIG message type (0) <br> *ex. 0x00 = CONFIG message* |
| 1-4 | Device Time | uint_32 | Epoche time from device in seconds.<br> *ex. 0x65073638 = 1694971448 = Sun, 17 Sep 2023 17:24:08 GMT* |
| 5 | SW Version | uint_8 | bit 7-4: Major SW Version<br>bit 3-0: Minor SW Version<br> *ex. 0x11 = SW v1.1* |
| 6-7 | Stored Record Count(n) | uint_16 | Current location record count stored in onboard flash.<br> *ex. 0x1234 = 4660 location records stored* |
| 8 | Sidewalk Link Type & Location Scanning Modes | uint_8 | bit 7-4: Sidewalk Link Type<br>-->0b0000 = 0x0 = BLE<br>-->0b0001 = 0x1 = FSK (not used/implemented)<br>-->0b0010 = 0x2 = LoRa<br> bit 3-0: Location Scanning Mode<br>-->0b0000 = 0x0 = WiFi Only<br>-->0b0001 = 0x1 = GNSS Only<br>-->0b0010 = 0x2 = WiFi + GNSS<br>*ex 0x22 = LoRa Link and WiFi + GNSS scanning* |
| 9 | Motion Period(min) | uint_8 | Period of time in minutes the device will stay in the motion state measured from when the motion threshold was last crossed.<br> *ex. 0x05 - 5min* |
|10 | Motion Sampling Frequency (s) | uint_8 | Frequency in seconds for sampling location data and attempting uplinks while device is in the motion state. <br>(Minimum value = 30s) <br> *ex. 0x3C = 60s* |
|11 | Motion Threshold(g) | uint_8 | Acceleration(g) setting for determining whether the tracker is static or in motion. <br> *ex. TBD* |
|12 | Static Sampling Frequecy (min) | uint_8 | Frequency in hours for sampling location data and attempting uplinks while device is in the static state. <br>(Minimum value = 15 - disabled/always in motion)<br> *ex. 0x3C = 60min sample and uplink frequecy* |


### NOLOC Uplink Message Format
|      |       |       |       |       |  
| :--- | :---: | :---: | :---: | :---: | 
| **Name** | Type | Reserved | Temp & Humidity | Motion State & Max Accel |
| **Position** | 0x10 | Byte 2 | Byte 3 | Byte 4 |

Description:
| Byte Offset | Name | Data Type | Description |
| :--: | :--  | :-------: | :---------- |
| 0 | Type | uint_8 | NOLOC message type<br> bit 7-6: TYPE = 1 (NOLOC)<br>bit 5-0: Unused <br> *ex. 0x40 = NOLOC type* |
| 1 | Reserved | uint_8 | Reserved |
| 2 | Temp & Humidity | uint_8 | bit 7-4: Temperature (C)<br>bit 3-0: Relative Humidity (%) <br> *ex. TBD* |
| 3 | Motion State & Max Accel | uint_8 | bit 7: Motion state<br>bit 6-0: Max accelleration since last record <br> *ex. TBD* |


### WIFI Location Uplink Message Format
|      |       |       |       |       |       | 
| :--- | :---: | :---: | :---: | :---: | :---: |
| **Name** | Type | Battery | Temp & Humidity | Motion State & Max Accel | WiFi AP Count | WiFi Location Data |
| **Position** | 0x80 | Byte 1 | Byte 2 | Byte 3 | Byte 4 | Byte 5 | Bytes 6+ |

Description:
| Byte Offset | Name | Data Type | Description |
| :--: | :--  | :-------: | :---------- |
| 0 | Type | uint_8 | WIFI message type<br> bit 7-6: TYPE = 2 (WIFI)<br>bit 5-0: Reserved<br>  *ex. 0x80 = WIFI type * |
| 1 | Battery | uint_8 | Battery level percentage |
| 2 | Temp & Humidity | uint_8 | bit 7-4: Temperature (C)<br>bit 3-0: Relative Humidity (%) <br> *ex. TBD* |
| 3 | Motion State & Max Accel | uint_8 | bit 7: Motion state<br>bit 6-0: Max accelleration since last record <br> *ex. TBD* |
| 4 | WiFi AP Count | uint_8 | Number of WiFi access points included (N) |
| 5+ | WiFi Location Data | N * 7 bytes | For each AP: 1 byte RSSI + 6 bytes MAC address<br>Repeated N times for all detected APs (up to 32) |

**Note:** Manual fragmentation removed. AWS IoT Core Device Location service accepts complete WiFi scan data in a single message.

### GNSS Location Uplink Message Format
|      |       |       |       |       |       |       |
| :--- | :---: | :---: | :---: | :---: | :---: | :---: |
| **Name** | Type | Battery | Temp & Humidity | Motion State & Max Accel | GNSS Data Size | Capture Time | NAV Message Data |
| **Position** | 0xC0 | Byte 1 | Byte 2 | Byte 3 | Byte 4 | Byte 5 | Bytes 6-9  | Bytes 10+ |

Description:
| Byte Offset | Name | Data Type | Description |
| :--: | :--  | :-------: | :---------- |
| 0 | Type | uint_8 | GNSS message type<br> bit 7-6: TYPE = 3 (GNSS)<br>bit 5-0: Reserved <br> *ex. 0xC0 = GNSS type * |
| 1 | Battery | uint_8 | Battery level percentage |
| 2 | Temp & Humidity | uint_8 | bit 7-4: Temperature (C)<br>bit 3-0: Relative Humidity (%) <br> *ex. TBD* |
| 3 | Motion State & Max Accel | uint_8 | bit 7: Motion state<br>bit 6-0: Max accelleration since last record <br> *ex. TBD* |
| 4 | GNSS Data Size | uint_8 | Size of the NAV message data in bytes |
| 5-8 | Capture Time | uint_32 | Timestamp when GNSS scan was captured (epoch seconds) |
| 9+ | NAV Message Data | variable | Complete GNSS NAV message (up to 512 bytes) |

**Note:** Manual fragmentation removed. AWS IoT Core Device Location service accepts complete GNSS NAV message data in a single payload (up to 512 bytes).


## Downlink Message Types
The asset tracker supports a single downlink message type (Type=0 - CONFIG) that can be used to update the configuration on the device and send lifecycle commands.

| TYPE Value | Name | Description |
| :--: | :--: | :-- |
| 0 | CONFIG | Config and commands for the asset tracker |

### CONFIG Downlink Message Format
|      |       |       |       |       |       |       |       |       |       |
| :--- | :---: | :---: | :---: | :---: | :---: | :---: | :---: | :---: | :---: |
| **Name** | Type & Command | Device Time | Max Records Stored | Link / Location Mode | Motion Period | Motion Sampling Freq | Motion Threshold | Static Sampling Freq |
| **Position** | Byte 1 | Bytes 2 - 3 | Bytes 4 - 5 | Bytes 7 - 8 | Byte 9 | Byte 10 | Byte 11 | Byte 12 | Byte 13 |

**TODO** Description:

| Byte Offset | Name | Data Type | Description |
| :--: | :--  | :-------: | :---------- |
| 0 | Type | uint_8 | CONFIG message type (0) <br> *ex. 0x00 = CONFIG message* |
| 1-4 | Device Time | uint_32 | Epoche time from device in seconds.<br> *ex. 0x65073638 = 1694971448 = Sun, 17 Sep 2023 17:24:08 GMT* |
| 5 | SW Version | uint_8 | bit 7-4: Major SW Version<br>bit 3-0: Minor SW Version<br> *ex. 0x11 = SW v1.1* |
| 6-7 | Stored Record Count(n) | uint_16 | Current location record count stored in onboard flash.<br> *ex. 0x1234 = 4660 location records stored* |
| 8 | Sidewalk Link Type & Location Scanning Modes | uint_8 | bit 7-4: Sidewalk Link Type<br>-->0b0000 = 0x0 = BLE<br>-->0b0001 = 0x1 = FSK (not used/implemented)<br>-->0b0010 = 0x2 = LoRa<br> bit 3-0: Location Scanning Mode<br>-->0b0000 = 0x0 = WiFi Only<br>-->0b0001 = 0x1 = GNSS Only<br>-->0b0010 = 0x2 = WiFi + GNSS<br>*ex 0x22 = LoRa Link and WiFi + GNSS scanning* |
| 9 | Motion Period(min) | uint_8 | Period of time in minutes the device will stay in the motion state measured from when the motion threshold was last crossed.<br> *ex. 0x05 - 5min* |
|10 | Motion Sampling Frequency (s) | uint_8 | Frequency in seconds for sampling location data and attempting uplinks while device is in the motion state. <br>(Minimum value = 30s) <br> *ex. 0x3C = 60s* |
|11 | Motion Threshold(g) | uint_8 | Acceleration(g) setting for determining whether the tracker is static or in motion. <br> *ex. TBD* |
|12 | Static Sampling Frequecy (hr) | uint_8 | Frequency in hours for sampling location data and attempting uplinks while device is in the static state. <br>(Minimum value = 0 - disabled/always in motion)<br> *ex. 0x01 = 1hr sample and uplink frequecy* |

