/* TTN Mapper for https://github.com/LilyGO/TTGO-T-Beam
  First version 16 april 2022, this version 25 juli 2023: https://github.com/Grrtzm/esp32-ttnmapper
  
  Make sure that you have ESP version 1.06 installed in the Arduino Boards Manager.
  The TTN/LMIC code requires ESP32 version 1.0.6 board support. It doesn't work with version 2+

  //At power up it connects to TTN using OTAA.// 25-7-2023: not any more; if the device can't connect to TTN it will not send GPS data, so changed it to ABP. Don't need the security anyhow.
  At power up it connects to TTN using ABP (25-7-2023)
  This program will only send GPS location to https://TTNmapper.org when moving.
  Below a speed of 18kmh (walking, cycling) it will transmit once every 100m distance travelled.
  At walking speed 5kmh it will transmit every 100/(5/3,6) = 72 seconds.
  18kmh or 5m/s means exactly 100m in 20 seconds, so you will get a fairly even distribution of data points.
  Above 18kmh by default it will transmit once every 20 seconds, unless you set TX mode to another interval.
  Using T=20s at 100kmh it transmits every 556m.  (100/3,6)*20=556 meters.
  The display will show a flash in the upper right corner after transmitting.

  Using the button, you can scroll through the 5 display pages.
  The first display frame shows the uptime, GPS location, GPS date, time and HDOP, the distance and speed travelled since last transmission.
  Hold the button for 1 second (=long press) to manually transmit the current GPS location, this works on first and second display.
  The second display frame displays number of messages sent, total airtime and dutycycle.
  In the third frame you can set transmission to manual (no transmission) or select a tranmission interval.
  Long press to enter setup mode. Use single clicks to select a value and long press to coinfirm.
  The fourth frame will let you select TX Power (2, 8, 10, 14dBm)
  The fifth frame will let you select the Spreading Factor.

  TTNmapper.org uses HDOP for location accuracy:
  https://docs.ttnmapper.org/integration/tts-integration-v3.html
  HDOP explained: https://en.wikipedia.org/wiki/Dilution_of_precision_(navigation)
  If the HDOP value is lower than 2, your data might not show up on ttnmapper.org: 
  https://docs.ttnmapper.org/FAQ.html 

  Use this to calculate the maximum airtime (based on 1% maximum allowed duty cycle):
  https://www.thethingsnetwork.org/airtime-calculator
  The payload of each transmission of this program is 18 bytes data plus 13 bytes overhead;
  https://developers.mydevices.com/cayenne/docs/lora/#lora-cayenne-low-power-payload
  Using SF7 and BW 125kHz this is 92.4ms => 1% duty cycle means maximum rate is 1 transmission every 9.24 seconds. This includes overhead.
  TTN Fair Use Policy (FUP) allows up to 30sec transmission time per device each day; 30/0.0924=324 messages per day
  At a maximum of 3 messages per minute, this allows for 324/3 = 108 minutes / 1h:48m hours of mapping each day.

  Parts of this sketch are borrowed from:
  https://github.com/rgot-org/TheThingsNetwork_esp32/blob/master/examples/ttn-otaa/ttn-otaa.ino
  and / or https://github.com/rgot-org/TheThingsNetwork_esp32/blob/master/examples/ttn_abp/ttn_abp.ino 
  https://github.com/ricaun/esp32-ttnmapper-gps/tree/master/esp32-ttnmapper-gps (various sketches)
  https://github.com/LilyGO/TTGO-T-Beam/blob/master/GPS-T22_v1.0-20190612/GPS-T22_v1.0-20190612.ino

  This version is for T22_v01 20190612 board.
  For the power management chip it requires the axp20x library:
  https://github.com/lewisxhe/AXP202X_Library
  You must import it as gzip in sketch submenu in Arduino IDE.
  This way, it is required to power up the GPS module, before trying to read it.
  Also get TinyGPS++ library from:
  https://github.com/mikalhart/TinyGPSPlus
  And get thet SSD1306 oled library from:
  https://github.com/ThingPulse/esp8266-oled-ssd1306
******************************************/

//#define debug // enable debugging via Serial.print() statements

#include <axp20x.h>
#include "SSD1306Wire.h"  // legacy include: `#include "SSD1306.h"`
#include "OLEDDisplayUi.h"
#include <TTN_esp32.h>
#include "TTN_CayenneLPP.h"
//#include <TinyGPS++.h> // 20230722_2310 disabled/replaced with TinyGPSPlus-ESP32)
#include <TinyGPSPlus.h>
#include <HardwareSerial.h>
#include "images.h"
#include <AceButton.h>

using namespace ace_button;

/***************************************************************************
    Go to your TTN console, register a device then copy fields
    and replace the const char* strings below
 ****************************************************************************/
// Device ID: eui-70B3D57ED01E94A4  =  OTAA
// const char* devEui = "CHOOSE_YOUR_OWN";
// const char* appEui = "CHOOSE_YOUR_OWN";  // Change to TTN Application EUI
// const char* appKey = "CHOOSE_YOUR_OWN";  // Change to TTN Application Key

// Device ID: eui-70B3D57ED01E94A5  =  ABP
const char* devEui  = "CHOOSE_YOUR_OWN";
const char* devAddr = "CHOOSE_YOUR_OWN";  // used in ABP mode
const char* appSKey = "CHOOSE_YOUR_OWN";
const char* nwkSKey = "CHOOSE_YOUR_OWN";


AXP20X_Class axp;
TTN_esp32 ttn;
TTN_CayenneLPP lpp;
uint8_t payload[18];  // Global variable to store the payload

SSD1306Wire display(0x3c, SDA, SCL);  // ADDRESS, SDA, SCL  -  SDA and SCL usually populate automatically based on your board's pins_arduino.h e.g. https://github.com/esp8266/Arduino/blob/master/variants/nodemcu/pins_arduino.h
OLEDDisplayUi ui(&display);
TinyGPSPlus gps;
const int BUTTON_PIN = 38;  // tbeam
AceButton button(BUTTON_PIN);
HardwareSerial hs(1);

#define GPSBaud 9600
#define RXPin 34
#define TXPin 12
#define OLED_RUNEVERY 1000
#define OLED_RST 16

int SEND_TIMER = 20;         // Interval in seconds between transmissions of GPS location
int sendTimer = SEND_TIMER;  // used in menu for selection
int menustate = 1;
int doShow = 1;
int numMessages = 0;
double txTime = 0.0f;       // time in seconds
double totalTxTime = 0.0f;  // time in seconds
unsigned long timeOfFirstTransmission;
double dutycycle = 0.0f;
int tx = 14, txPow = 14;
int sf = 7, spreadingFactor = 7;
int txMode = 1, txM = 1;
int editmode = 0;

uint8_t txBuffer[9];
uint32_t LatitudeBinary, LongitudeBinary;
uint16_t altitudeGps;
uint8_t hdopGps;
float previous_gps_latitude = 0.0f;
float previous_gps_longitude = 0.0f;
boolean xmitEnabled = true;         // Set to false for testing
boolean firstTransmission = false;  // set to false on 30-4-2022 // This will enable to only send a first message containing GPS location, the latter ones will be sent if travelled distance>100m or speed>18kmh.
boolean didTransmit = false;
boolean sendTimerElapsed = false;
unsigned long lastTransmission = 0;

boolean oled_runEvery(unsigned long interval) {
  static unsigned long previousMillis = 0;
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;
    return true;
  }
  return false;
}

String getUptime(unsigned long diff) {
  String str = "";
  unsigned long t = millis() / 1000;
  int s = t % 60;
  int m = (t / 60) % 60;
  int h = (t / 3600);
  str += h;
  str += ":";
  if (m < 10)
    str += "0";
  str += m;
  str += ":";
  if (s < 10)
    str += "0";
  str += s;
  return str;
}

float packetDurationCalculator(int spreadingFactor) {
  /* Source: https://www.rfwireless-world.com/calculators/LoRaWAN-Airtime-calculator.html
    https://github.com/avbentem/airtime-calculator/blob/master/src/lora/Airtime.ts :
    See https://lora-developers.semtech.com/library/product-documents/ for the
    equations in AN1200.13 "LoRa Modem Designer’s Guide"
    for bytesPayload see https://developers.mydevices.com/cayenne/docs/lora/#lora-cayenne-low-power-payload
  */
  int nPreamble = 8;           // Number of symbols in preamble
  int bytesPayload = 18 + 13;  // 18 bytes for lpp cayenne GPS data and 13 bytes overhead
  /*  Payloadsize is full packet size. For LoRaWAN this includes the LoRaWAN MAC
      header (about 9 bytes when no MAC commands are included), the application
      payload, and the MIC (4 bytes).
  */
  int CR = 1;                                                              // coding rate 4/5, a default/fixed value for TTN
  int BW = 125;                                                            // bandwidth
  int DE = (spreadingFactor >= 11) ? 1 : 0;                                // DE = 1 when the low data rate optimization is enabled
  int header = 0;                                                          // 0 = enabled
  float tSymbol = ((pow(2, spreadingFactor)) / (BW * 1000.0f)) * 1000.0f;  // time in milliseconds
  float tPreamble = (nPreamble + 4.25f) * tSymbol;
  int payloadSymbNb = 8 + fmax((ceil(((8 * bytesPayload) - (4 * spreadingFactor) + 28 + 16 - (20 * header))/(4 * (spreadingFactor - 2 * DE)) * (CR + 4))), 0);  // number of payload symbols
  float tPayload = payloadSymbNb * tSymbol;
  float tPacket = tPreamble + tPayload;
  return tPacket;
}

void transmit() {
  if (xmitEnabled) {
    unsigned long justnow = millis();

#ifdef debug
    // Enable for debugging
    // Print the payload data
    Serial.print("Payload size: ");
    Serial.println(lpp.getSize());
    Serial.print("Payload data: ");
    for (int i = 0; i < lpp.getSize(); i++) {
      Serial.print("0x");
      if (payload[i] < 16) Serial.print("0");  // Print leading zero for single-digit hexadecimal values
      Serial.print(payload[i], HEX);
      Serial.print(" ");
    }
    Serial.println();
#endif

    // Send the payload via TTN
    ttn.sendBytes(payload, lpp.getSize());

    txTime = packetDurationCalculator(spreadingFactor);
    totalTxTime += txTime;
    // dutycycle can be calculated just before second transmission
    dutycycle = (double(totalTxTime) / double(justnow + txTime - timeOfFirstTransmission)) * 100.0f;
    numMessages++;

    if (totalTxTime > 30000.0f) {  // 30 seconds, time in milliseconds
      xmitEnabled = false;         // Disable transmissions if maximum allowed time per day is used up.
      Serial.println("transmission disabled");
    }
  }
  // Reset starting point for distance measurement
  // This will show the distance since last transmission
  previous_gps_latitude = gps.location.lat();
  previous_gps_longitude = gps.location.lng();
  didTransmit = true;
  lastTransmission = millis();
}

void PayloadNow() {
  if (gps.location.isValid()) {
    lpp.reset();

    // Add GPS data
    lpp.addGPS(1, gps.location.lat(), gps.location.lng(), gps.altitude.meters());
    lpp.addAnalogInput(2, gps_HDOP());
    lpp.addDigitalInput(3, gps.satellites.value());

    // Encode the CayenneLPP data
    payload[lpp.getSize()];
    lpp.copy(payload);

#ifdef debug
    // Enable for debugging
    // Print the payload data
    Serial.print("Payload size: ");
    Serial.println(lpp.getSize());
    Serial.print("Latitude: ");
    Serial.println(gps.location.lat());
    Serial.print("Longitude: ");
    Serial.println(gps.location.lng());
    Serial.print("Altitude: ");
    Serial.println(gps.altitude.meters());
    Serial.print("HDOP: ");
    Serial.println(gps_HDOP());
    Serial.print("Satellites: ");
    Serial.println(gps.satellites.value());
#endif

    if ((firstTransmission) || ((distance_in_meters() >= 100) && (speed_in_kmh() < 18))) {
      transmit();
      firstTransmission = false;
      //Serial.print("XMIT dist_m ");
      //Serial.println(distance_in_meters());
    }

    unsigned long timeToUse;
    if ((SEND_TIMER * 1000) > (txTime * 99.0f))
      timeToUse = (SEND_TIMER * 1000);
    else
      timeToUse = (txTime * 99.0f);  // Prevents dutycycle to become higher than 1%

    if (millis() - lastTransmission >= timeToUse) {
      sendTimerElapsed = true;
    }

    if (sendTimerElapsed && (speed_in_kmh() >= 18)) {
      transmit();
      sendTimerElapsed = false;
      //Serial.print("XMIT speed_kmh ");
      //Serial.println(speed_in_kmh());
    }
  }
}

void gps_loop() {
  while (hs.available() > 0) {
    gps.encode(hs.read());
  }
}

float gps_HDOP() {
  if (gps.hdop.isValid())
    return gps.hdop.hdop();  // enabled 20230722_2032
  else
    return 40.0;
}

String gps_location() {
  if (gps.location.isValid()) {
    String str = "";
    str += gps.location.lat();
    str += " ";
    str += gps.location.lng();
    return str;
  } else {
    return "#### ####";
  }
}

uint16_t distance_in_meters() {
  if (gps.location.isValid())
    return (uint16_t)gps.distanceBetween(gps.location.lat(), gps.location.lng(), previous_gps_latitude, previous_gps_longitude);
  else
    return 0;
}

float speed_in_kmh() {
  if (gps.speed.isValid())
    return gps.speed.kmph();
  else
    return 0.0f;
}

String gps_date() {
  if (gps.date.isValid()) {
    int d = gps.date.day();
    int m = gps.date.month();
    int y = gps.date.year();
    String str = "";
    if (d < 10) str += "0";
    str += d;
    str += "/";
    if (m < 10) str += "0";
    str += m;
    str += "/";
    str += y;
    return str;
  } else {
    return "xx/xx/xxxx";
  }
}

String gps_time() {
  if (gps.time.isValid()) {
    int s = gps.time.second();
    int m = gps.time.minute();
    int h = gps.time.hour();
    String str = "";
    if (h < 10) str += "0";
    str += h;
    str += ":";
    if (m < 10) str += "0";
    str += m;
    str += ":";
    if (s < 10) str += "0";
    str += s;
    return str;
  } else {
    return "xx:xx:xx";
  }
}

boolean oled_loop() {
  if (oled_runEvery(OLED_RUNEVERY)) {
    if (didTransmit) {  // shows the flash a bit longer
      didTransmit = false;
    }
    if (txMode) {    // txMode==1; automatic, txMode==0; manual
      PayloadNow();  // Lets the flash display next pass
    }
    return true;
  }
  return false;
}

// This is used to display the Transmit symbol; circle with flash inside, see "images.h"
void msOverlay(OLEDDisplay* display, OLEDDisplayUiState* state) {
  if (didTransmit) {
    display->drawXbm(128 - 16, 0, 16, 16, flashImage);  // Display flash during transmission
    //didTransmit = false;
  }
}

void drawFrame1(OLEDDisplay* display, OLEDDisplayUiState* state, int16_t x, int16_t y) {
  display->setFont(ArialMT_Plain_10);
  display->setTextAlignment(TEXT_ALIGN_LEFT);
  String str = "";
  str += "Uptime ";
  str += getUptime(0);
  str += ", Alt ";
  str += gps.altitude.meters();
  str += "m\n";
  str += "GPS ";
  str += gps_location();
  str += " hdop ";
  str += gps_HDOP();
  str += "\n";
  str += "UTC ";
  str += gps_date();
  str += " ";
  str += gps_time();
  str += "\n";
  str += "Dist ";
  str += distance_in_meters();
  str += "m, Speed ";
  str += speed_in_kmh();
  str += "kmh\n";
  display->drawString(0 + x, 0 + y, str);  // left
}

void drawFrame2(OLEDDisplay* display, OLEDDisplayUiState* state, int16_t x, int16_t y) {
  display->setTextAlignment(TEXT_ALIGN_LEFT);
  display->setFont(ArialMT_Plain_10);
  String str = "";
  str += "# Messages sent: ";
  str += String(numMessages);
  str += "\n";
  str += "msg Xmit time: ";
  str += String(double(txTime / 1000.0f), 3);  // ,3 is the number of decimal places
  str += "s\n";
  str += "All TXtime(<30s): ";
  str += String(double(totalTxTime / 1000.0f), 2);  // ,2 is the number of decimal places
  str += "s\n";
  str += "Dutycycle(<1%): ";
  str += String(dutycycle, 2);  // ,2 is the number of decimal places
  str += "%";
  display->drawString(0 + x, 0 + y, str);  // left
}

void drawFrame3(OLEDDisplay* display, OLEDDisplayUiState* state, int16_t x, int16_t y) {
  display->setTextAlignment(TEXT_ALIGN_LEFT);
  display->setFont(ArialMT_Plain_16);
  display->drawString(0 + x, 0 + y, "Set TX Mode");  // left
  display->setFont(ArialMT_Plain_10);
  //  String str = "Manual, Automatic(*)\n";
  //  if (editmode) str += "=> mode = ";
  //  else         str += "   mode = ";
  //  if (txM) str += "automatic";
  //  else    str += "manual   ";
  //  str += "    ";
  String str = "Manual, 7, 20(*), 30, 60s\n";
  if (editmode) str += "=> mode = ";
  else str += "   mode: ";
  if (!txM) str += "manual   ";
  else {
    str += "T=";
    str += String(sendTimer);
    str += "sec  ";
  }
  display->drawString(0 + x, 20 + y, str);  // left
  ui.update();
}

void drawFrame4(OLEDDisplay* display, OLEDDisplayUiState* state, int16_t x, int16_t y) {
  display->setTextAlignment(TEXT_ALIGN_LEFT);
  display->setFont(ArialMT_Plain_16);
  display->drawString(0 + x, 0 + y, "Set Tx Power");  // left
  display->setFont(ArialMT_Plain_10);
  String str = "2, 8, 10, 14dBm(*)\n";
  if (editmode) str += "=> TX power = ";
  else str += "   TX power = ";
  str += String(tx);
  str += "dBm  ";
  display->drawString(0 + x, 20 + y, str);  // left
  ui.update();
}

void drawFrame5(OLEDDisplay* display, OLEDDisplayUiState* state, int16_t x, int16_t y) {
  display->setTextAlignment(TEXT_ALIGN_LEFT);
  display->setFont(ArialMT_Plain_16);
  display->drawString(0 + x, 0 + y, "Set Spread Factor");  // left
  display->setFont(ArialMT_Plain_10);
  String str = "SF7(*) .. SF12\n";
  if (editmode) str += "=> sf = ";
  else str += "   sf = ";
  str += String(sf);
  str += "    ";
  display->drawString(0 + x, 20 + y, str);  // left
  ui.update();
}

void oled_setup() {
  pinMode(OLED_RST, OUTPUT);
  digitalWrite(OLED_RST, HIGH);
  display.init();
  display.resetDisplay();
  display.displayOn();
  display.flipScreenVertically();
  display.setFont(ArialMT_Plain_16);
  display.setContrast(255);
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  display.setFont(ArialMT_Plain_16);
  display.drawString(0, 0, "Waiting for TTN"); // Not necessary for ABP, it will quickly run past this so not visible.
  display.drawString(0, 20, "Connecting...");
  display.drawString(0, 40, "   (OTAA)");
  display.display();
}

class ButtonHandler : public IEventHandler {
public:
  void handleEvent(AceButton* /* button */, uint8_t eventType,
                   uint8_t buttonState) override {
    if (doShow) {
      switch (menustate) {

        case 1:
          ui.transitionToFrame(0);  // First frame has number 0
          break;

        case 2:
          ui.transitionToFrame(1);
          break;

        case 3:
          ui.transitionToFrame(2);
          break;

        case 4:
          ui.transitionToFrame(3);
          break;

        case 5:
          ui.transitionToFrame(4);
          break;

        case 31:
          txM = 0;
          break;

        case 32:
          txM = 1;
          sendTimer = 7;
          break;

        case 33:
          sendTimer = 20;
          txM = 1;
          break;

        case 34:
          txM = 1;
          sendTimer = 30;
          break;

        case 35:
          txM = 1;
          sendTimer = 60;
          break;

        case 41:
          tx = 2;
          break;

        case 42:
          tx = 8;
          break;

        case 43:
          tx = 10;
          break;

        case 44:
          tx = 14;
          break;

        case 51:
          sf = 7;
          break;

        case 52:
          sf = 8;
          break;

        case 53:
          sf = 9;
          break;

        case 54:
          sf = 10;
          break;

        case 55:
          sf = 11;
          break;

        case 56:
          sf = 12;
          break;

        default:
          break;
      }
      doShow = 0;
    }
    switch (eventType) {
      case AceButton::kEventClicked:
        if (menustate >= 56) menustate = 50;                         // submenu 3, Spreading Factor
        else if (menustate >= 44 && menustate < 50) menustate = 40;  // submenu 2, TX Power
        else if (menustate >= 35 && menustate < 40) menustate = 30;  // submenu 1, TX Mode
        else if (menustate >= 5 && menustate < 30) menustate = 0;    // main menu, back to the 'loop'
        menustate++;
        doShow = 1;
        break;

      case AceButton::kEventLongPressed:
        if (menustate == 1) {
          firstTransmission = true;
          PayloadNow();  // Check for transmission mode and transmit
        }
        if (menustate == 2) {
          firstTransmission = true;
          PayloadNow();  // Check for transmission mode and transmit
        }
        if (menustate == 3) {
          menustate = 30;  // submenu 1, TX Mode
          editmode = 1;
        }
        if (menustate == 4) {
          menustate = 40;  // submenu 2, TX Power
          editmode = 1;
        }
        if (menustate == 5) {
          menustate = 50;  // submenu 3, Spreading Factor
          editmode = 1;
        }
        if (menustate > 50 && menustate < 58) {
          menustate = 5;
          editmode = 0;
          spreadingFactor = sf;
          LMIC_setDrTxpow(12 - spreadingFactor, txPow);  // datarate, powerlevel
        } else if (menustate > 40 && menustate < 46) {
          menustate = 4;
          editmode = 0;
          txPow = tx;
          // Default frequency plan for EU 868MHz ISM band
          // data rates
          // this is a little confusing: the integer values of these constants are the
          // DataRates from the LoRaWAN Regional Parameter spec. The names are just
          // convenient indications, so we can use them in the rare case that we need to
          // choose a DataRate by SF and configuration, not by DR code.
          //            DR_SF12 = 0,
          //            DR_SF11, // 1
          //            DR_SF10, // 2
          //            DR_SF9, // 3
          //            DR_SF8, // 4
          //            DR_SF7, // 5
          //            DR_SF7B, // 6
          //            DR_FSK, // 7
          // TxPowerLevels 16, 14, 12, 10, 8, 6, 4, 2
          // working values: 2, 4, 6, 8, 12, 14 (16 can only be seen with LMIC.adrTxPow)
          // LMIC_setDrTxpow(0,10); // datarate, powerlevel
          LMIC_setDrTxpow(12 - spreadingFactor, txPow);  // datarate, powerlevel
        } else if (menustate > 30 && menustate < 36) {
          menustate = 3;
          editmode = 0;
          txMode = txM;
          SEND_TIMER = sendTimer;
        }
        doShow = 1;
        break;

      default:
        break;
    }
  }
};

ButtonHandler handleEvent;

// how many display frames are there?
int frameCount = 5;
int overlaysCount = 1;

// This array keeps function pointers to all display frames
// frames are the single views that slide in
FrameCallback frames[] = { drawFrame1, drawFrame2, drawFrame3, drawFrame4, drawFrame5 };

// Display overlays are statically drawn on top of a frame. This is used for the transmission "flash".
OverlayCallback overlays[] = { msOverlay };

void setup() {
  Serial.begin(115200);

  Wire.begin(21, 22);
  if (!axp.begin(Wire, AXP192_SLAVE_ADDRESS)) {
    Serial.println("AXP192 Begin PASS");
  } else {
    Serial.println("AXP192 Begin FAIL");
  }
  axp.setPowerOutPut(AXP192_LDO2, AXP202_ON);
  axp.setPowerOutPut(AXP192_LDO3, AXP202_ON);
  axp.setPowerOutPut(AXP192_DCDC2, AXP202_ON);
  axp.setPowerOutPut(AXP192_EXTEN, AXP202_ON);
  axp.setPowerOutPut(AXP192_DCDC1, AXP202_ON);

  oled_setup();                                 // Display TTN connection information
  hs.begin(GPSBaud, SERIAL_8N1, RXPin, TXPin);  // GPS setup
  ttn.begin();

  // OTAA:
  // ttn.join(devEui, appEui, appKey);
  // Serial.print("Joining TTN ");
  // while (!ttn.isJoined()) {
  //   Serial.print(".");
  //   delay(500);
  // }
  // Serial.println("\njoined !");

  // ABP:
  ttn.begin();
  ttn.personalize(devAddr, nwkSKey, appSKey);
  ttn.showStatus();

  timeOfFirstTransmission = millis();
  LMIC_setAdrMode(0);  // Disable ADR mode (if mobile turn off), set to 0x80 to enable
  ttn.showStatus();

  ui.setTargetFPS(25);
  ui.setActiveSymbol(activeSymbol);
  ui.setInactiveSymbol(inactiveSymbol);
  ui.setIndicatorPosition(BOTTOM);
  ui.setIndicatorDirection(LEFT_RIGHT);
  ui.setFrameAnimation(SLIDE_LEFT);
  ui.setFrames(frames, frameCount);
  ui.setOverlays(overlays, overlaysCount);
  ui.disableAutoTransition();
  ui.init();
  display.flipScreenVertically();
  ui.switchToFrame(0);
  hs.begin(GPSBaud, SERIAL_8N1, RXPin, TXPin);  // GPS setup
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  // Configure the ButtonConfig with the event handler, and enable all higher level events.
  ButtonConfig* buttonConfig = button.getButtonConfig();
  buttonConfig->setIEventHandler(&handleEvent);
  buttonConfig->setFeature(ButtonConfig::kFeatureClick);
  buttonConfig->setFeature(ButtonConfig::kFeatureLongPress);
  buttonConfig->setFeature(ButtonConfig::kFeatureSuppressAfterLongPress);
}

void loop() {
  // Should be called every 4-5ms or faster, for the default debouncing time ~20ms.
  button.check();
  ui.update();

  gps_loop();  // There's no timing component for the GPS in here.
  oled_loop();
}
