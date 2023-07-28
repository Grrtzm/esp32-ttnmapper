# ESP32-ttnmapper
## Use an ESP32 as a LoRaWAN tracker
What do you use this for? \
You can use it to measure coverage of 'The Things Network' LoRaWAN network, more specific: \
You can [measure the signal strength](https://ttnmapper.org) from [your own LoRaWAN devices or gateway](https://ttnmapper.org/advanced-maps/). \
Take a look at the [pictures below](https://github.com/Grrtzm/esp32-ttnmapper#ttnmapper-advanced-maps-examples-for-one-of-my-own-devices).

I use a "LILYGO® TTGO T-Beam V1.1 ESP32 LoRa 868MHz". This model isn't for sale anymore, but the V1.2 is. \
They often also mention "Meshtastic" with the model name. \
See [https://github.com/LilyGO/TTGO-T-Beam](https://github.com/LilyGO/TTGO-T-Beam) and the [picture below](https://github.com/Grrtzm/esp32-ttnmapper#lilygo-ttgo-t-beam). \
If you use another board, make sure you use the correct pins for GPS TX/RX and make sure you use the correct power management chip.

## Installation
Make sure that you have [ESP32 board support](https://espressif-docs.readthedocs-hosted.com/projects/arduino-esp32/en/latest/installing.html) version 1.06 [installed in the Arduino Boards Manager](https://github.com/Grrtzm/esp32-ttnmapper/blob/main/Configure_Arduino_IDE.png). \
The TTN/LMIC code requires ESP32 version 1.0.6 board support. It doesn't work with version 2+ \
[Select board 'T-beam' from the ESP32 boards collection.](https://github.com/Grrtzm/esp32-ttnmapper/blob/main/Configure_Arduino_IDE.png)

## Usage
You must select if you want it to connect to 'The Things Network' using OTAA or ABP. With OTAA it first needs to authenticate with the network (more secure), with ABP it doesn't.
If the device can't connect to TTN using OTAA (as when using it in the UK; not much TTN coverage) it will not send GPS data, so i changed it to ABP. I don't need the security anyhow.

This program will only send GPS location to [https://TTNmapper.org](https://TTNmapper.org) when moving. 
* Below a speed of 18kmh (walking, cycling) it will transmit once every 100m distance travelled. 
* At walking speed 5kmh it will transmit every 100/(5/3,6) = 72 seconds. 
* 18kmh or 5m/s means exactly 100m in 20 seconds, so you will get a fairly even distribution of data points. 
* Above 18kmh by default it will transmit once every 20 seconds, unless you set TX mode to another interval. 
* Using T=20s at 100kmh it transmits every 556m.  (100/3,6)*20=556 meters. 

The display will show an icon (a flash in a circle) in the upper right corner after transmitting. 

### Button and display
Using the middle button, you can scroll through the 5 display pages. 
1. The first display frame shows the uptime, GPS location, GPS date, time and HDOP, the distance and speed travelled since last transmission. 
    * Hold the button for 1 second (=long press) to manually transmit the current GPS location, this works on first and second display frame. 
2. The second display frame displays number of messages sent, total airtime and dutycycle. 
3. In the third frame you can set transmission to manual (only transmit after long press) or select a transmission interval: (manual, 7s, 20s, 30s, 60s). 
    * Long press to enter setup mode. Use single clicks to select a value and long press to confirm. 
4. The fourth frame will let you select TX Power (2, 8, 10, 14dBm). 
5. The fifth frame will let you select the Spreading Factor (SF7..12). 

## Technical stuff
TTNmapper.org uses [HDOP for location accuracy](https://docs.ttnmapper.org/integration/tts-integration-v3.html). 
[This explains what HDOP is](https://en.wikipedia.org/wiki/Dilution_of_precision_(navigation)). \
If the HDOP value is lower than 2, [your data might not show up on ttnmapper.org](https://docs.ttnmapper.org/FAQ.html). 


[This is used to calculate the maximum airtime](https://www.thethingsnetwork.org/airtime-calculator) (based on 1% maximum allowed duty cycle). \
The payload of each transmission of this program is 18 bytes data [plus 13 bytes overhead](https://developers.mydevices.com/cayenne/docs/lora/#lora-cayenne-low-power-payload).


Using SF7 and BW 125kHz this is 92.4ms => 1% duty cycle means maximum rate is 1 transmission every 9.24 seconds. This includes overhead.
TTN Fair Use Policy (FUP) allows up to 30sec transmission time per device each day; 30/0.0924=324 messages per day
At a maximum of 3 messages per minute, this allows for 324/3 = 108 minutes / 1h:48m hours of mapping each day.

Parts of this sketch are borrowed from: 
* [https://github.com/rgot-org/TheThingsNetwork_esp32/blob/master/examples/ttn-otaa/ttn-otaa.ino](https://github.com/rgot-org/TheThingsNetwork_esp32/blob/master/examples/ttn-otaa/ttn-otaa.ino) 
* and / or [https://github.com/rgot-org/TheThingsNetwork_esp32/blob/master/examples/ttn_abp/ttn_abp.ino](https://github.com/rgot-org/TheThingsNetwork_esp32/blob/master/examples/ttn_abp/ttn_abp.ino) 
* [https://github.com/ricaun/esp32-ttnmapper-gps/tree/master/esp32-ttnmapper-gps](https://github.com/ricaun/esp32-ttnmapper-gps/tree/master/esp32-ttnmapper-gps) (various sketches) 
* [https://github.com/LilyGO/TTGO-T-Beam/blob/master/GPS-T22_v1.0-20190612/GPS-T22_v1.0-20190612.ino](https://github.com/LilyGO/TTGO-T-Beam/blob/master/GPS-T22_v1.0-20190612/GPS-T22_v1.0-20190612.ino)

This version is for T22_v01 20190612 board. \
For the power management chip it requires [the axp20x library](https://github.com/lewisxhe/AXP202X_Library). \
You must import it as gzip in sketch submenu in Arduino IDE. \
It is required to power up the GPS module, before you can read from it. 

### Used libraries:
1. [AceButton@1.10.1]
2. [AXP202X_Library@1.1.2]
3. [ESP8266 and ESP32 OLED driver for SSD1306 displays@4.1.0 ESP8266 and ESP32 OLED driver for SSD1306 displays@4.4.0]
4. [TinyGPSPlus@1.0.3 TinyGPSPlus-ESP32@0.0.2]
5. [TTN_esp32@0.1.6]

## Settings in the TTN Console
I'm not going to instruct you how to create a new TTN mapper application/device/webhook etc. \
Use [this](https://docs.ttnmapper.org/) as a starting point.\
You need a script that translates the data which is sent from your TTN mapper device to something the TTN network understands.
In order to do that, you need to install a "Payload formatter" script. You can do that on "Application" or "Device" level.\
Copy the contents of ["TTN_Uplink_Payload_formatter.txt"](https://github.com/Grrtzm/esp32-ttnmapper/blob/main/TTN_Uplink_Payload_formatter.txt) to an Uplink Payload formatter (custom Javascript).
In this case it's not possible to use a standard Cayenne LPP payload formatter because the data format i use is custom made.

# LILYGO TTGO T-Beam
![LILYGO-TTGO-Meshtastic-T-Beam-V1-2-ESP32-LoRa-915MHz-433MHz-868MHz-923MHz-WiFi-BLE-GPS](https://github.com/Grrtzm/esp32-ttnmapper/blob/main/LILYGO-TTGO-Meshtastic-T-Beam-V1-2-ESP32-LoRa-915MHz-433MHz-868MHz-923MHz-WiFi-BLE-GPS.png)

# TTNmapper "Advanced Maps" examples
You can click on any line, sphere or gateway to see signal strength, distance and other properties \
(note: this is before i added hdop, # of satellites etc). \
I took a trip on a ferry from IJmuiden to Newcastle. This shows a connection between IJmuiden and Alnmouth, near the Scottish border; 487km !!:
![Overview map](https://github.com/Grrtzm/esp32-ttnmapper/blob/main/ttnmapper_advanced_map_1.png) \
More detailed map, showing spheres of different colors (blue: weak signal, red: very strong) across part of the Netherlands over the course of several months:
![More detailed map](https://github.com/Grrtzm/esp32-ttnmapper/blob/main/ttnmapper_advanced_map_2.png) 