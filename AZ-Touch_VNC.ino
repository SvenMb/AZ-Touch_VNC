/*******************************************************************************
 * Arduino VNC
 * This is a simple VNC sample
 *
 * Dependent libraries:
 * ArduinoVNC: https://github.com/moononournation/arduinoVNC.git
 *
 * Touch libraries:
 * FT6X36: https://github.com/strange-v/FT6X36.git
 * GT911: https://github.com/TAMCTec/gt911-arduino.git
 * XPT2046: https://github.com/PaulStoffregen/XPT2046_Touchscreen.git
 ******************************************************************************/
#define ESP_DRD_USE_SPIFFS true

#include <FS.h>
#include <SPIFFS.h>
#include <ArduinoJson.h>

char VNC_IP[50] = "192.168.4.128";
uint16_t VNC_PORT = 5900;
char VNC_PW[50] = "12345678";

#define JSON_CONFIG_FILE "/vnc.json"
bool shouldSaveConfig = false;
bool forceConfig = false;

/*******************************************************************************
 * Please config the touch panel in touch.h
 * is configured there for XPT2046 on AZ-Touch mod
 ******************************************************************************/
#include "touch.h"

/*******************************************************************************
 * Please config the optional keyboard in keyboard.h
 * currently no keyboard
 ******************************************************************************/
#include "keyboard.h"

/*******************************************************************************
 * Start of Arduino_GFX setting
 *
 * Arduino_GFX try to find the settings depends on selected board in Arduino IDE
 * Or you can define the display dev kit not in the board list
 * Defalult pin list for non display dev kit:
 * Arduino Nano, Micro and more: CS:  9, DC:  8, RST:  7, BL:  6, SCK: 13, MOSI: 11, MISO: 12
 * ESP32 various dev board     : CS:  5, DC: 27, RST: 33, BL: 22, SCK: 18, MOSI: 23, MISO: nil
 * ESP32-C3 various dev board  : CS:  7, DC:  2, RST:  1, BL:  3, SCK:  4, MOSI:  6, MISO: nil
 * ESP32-S2 various dev board  : CS: 34, DC: 38, RST: 33, BL: 21, SCK: 36, MOSI: 35, MISO: nil
 * ESP32-S3 various dev board  : CS: 40, DC: 41, RST: 42, BL: 48, SCK: 36, MOSI: 35, MISO: nil
 * ESP8266 various dev board   : CS: 15, DC:  4, RST:  2, BL:  5, SCK: 14, MOSI: 13, MISO: 12
 ******************************************************************************/
#include <Arduino_GFX_Library.h>

// Backlight pin on AZ-Touch Mod
#define GFX_BL 15

#if defined(ESP32)
// this initialization is for ESP32 Devkit in AZ-Touch Mod
Arduino_DataBus *bus = new Arduino_ESP32SPI(4 /* DC */, 5 /* CS */, 18 /* SCK */, 23 /* MOSI */, 19 /* MISO */);
Arduino_GFX *gfx = new Arduino_ILI9341(bus, 22 /* RST */, 1 /* rotation */);
#elif defined(ESP8266)
// don't know the pins/ can't test with the ESP8266 , please tell me, should work there too
#endif

/*******************************************************************************
 * End of Arduino_GFX setting
 ******************************************************************************/

// WiFi Manager does all the Wifi config stuff and loads config for VNC-Client
#include <WiFiManager.h> // https://github.com/tzapu/WiFiManager
WiFiManager wm; // global WifiManager

#define TRIGGER_PIN 0 // use boot pin to force config

#include "VNC_GFX.h"
#include <VNC.h>

VNC_GFX *vnc_gfx = new VNC_GFX(gfx);
arduinoVNC vnc = arduinoVNC(vnc_gfx);


// save and load config, got it from: https://github.com/witnessmenow/ESP32-WiFi-Manager-Examples
void saveConfigFile()
{
  Serial.println(F("Saving config"));
  StaticJsonDocument<512> json;
  json["VNC_IP"] = VNC_IP;
  json["VNC_PORT"] = VNC_PORT;
  json["VNC_PW"] = VNC_PW;

  File configFile = SPIFFS.open(JSON_CONFIG_FILE, "w");
  if (!configFile)
  {
    Serial.println("failed to open config file for writing");
  }

  serializeJsonPretty(json, Serial);
  if (serializeJson(json, configFile) == 0)
  {
    Serial.println(F("Failed to write to file"));
  }
  configFile.close();
}

bool loadConfigFile()
{
  // clean FS, for testing
  // SPIFFS.format();

  //read configuration from FS json
  Serial.println("mounting FS...");

  // May need to make it begin(true) first time you are using SPIFFS
  // NOTE: This might not be a good way to do this! begin(true) reformats the spiffs
  // it will only get called if it fails to mount, which probably means it needs to be
  // formatted, but maybe dont use this if you have something important saved on spiffs
  // that can't be replaced.
  if (SPIFFS.begin(false) || SPIFFS.begin(true))
  {
    Serial.println("mounted file system");
    if (SPIFFS.exists(JSON_CONFIG_FILE))
    {
      //file exists, reading and loading
      Serial.println("reading config file");
      File configFile = SPIFFS.open(JSON_CONFIG_FILE, "r");
      if (configFile)
      {
        Serial.println("opened config file");
        StaticJsonDocument<512> json;
        DeserializationError error = deserializeJson(json, configFile);
        serializeJsonPretty(json, Serial);
        if (!error)
        {
          Serial.println("\nparsed json");

          strcpy(VNC_IP, json["VNC_IP"]);
          VNC_PORT = json["VNC_PORT"].as<int>();
          strcpy(VNC_PW, json["VNC_PW"]);

          return true;
        }
        else
        {
          Serial.println("failed to load json config");
        }
      }
    }
  }
  else
  {
    Serial.println("failed to mount FS");
  }
  //end read
  return false;
}

void saveConfigCallback()
{
  Serial.println("Should save config");
  shouldSaveConfig = true;
}

// This gets called when the config mode is launched, might
// be useful to update a display with this info.
void configModeCallback(WiFiManager *myWiFiManager)
{
  Serial.println("Entered Conf Mode");

  Serial.print("Config SSID: ");
  Serial.println(myWiFiManager->getConfigPortalSSID());

  // Serial.print("Config Parameter: ");
  // Serial.println(myWiFiManager->getParameters);

  Serial.print("Config IP Address: ");
  Serial.println(WiFi.softAPIP());

  
}



void TFTnoWifi(void)
{
  gfx->fillScreen(BLACK);
  gfx->setCursor(0, ((gfx->height() / 2) - (5 * 8)));
  gfx->setTextColor(RED);
  gfx->setTextSize(5);
  gfx->println("NO WIFI!");
  gfx->setTextSize(2);
  gfx->println();
}

void TFT_WM(void)
{
  gfx->fillScreen(BLACK);
  gfx->setCursor(0, ((gfx->height() / 2) - (5 * 8)));
  gfx->setTextColor(RED);
  gfx->setTextSize(4);
  gfx->println("WiFiManager");
  gfx->setTextSize(2);
  gfx->println();
}


void TFTnoVNC(void)
{
  gfx->fillScreen(BLACK);
  gfx->setCursor(0, ((gfx->height() / 2) - (4 * 8)));
  gfx->setTextColor(GREEN);
  gfx->setTextSize(4);
  gfx->println("connect VNC");
  gfx->setTextSize(2);
  gfx->println();
  gfx->print(VNC_IP);
  gfx->print(":");
  gfx->println(VNC_PORT);
}

void handle_touch()
{
  if (touch_has_signal())
  {
    if (touch_touched())
    {
      vnc.mouseEvent(touch_last_x, touch_last_y, 0b001);
    }
    else if (touch_released())
    {
      vnc.mouseEvent(touch_last_x, touch_last_y, 0b000);
    }
  }
}

void handle_keyboard()
{
  int key = keyboard_get_key();
  if (key > 0)
  {
    // Serial.println(key);
    switch (key)
    {
    case 8:
      key = 0xff08; // BackSpace
      break;
    case 9:
      key = 0xff09; // Tab
      break;
    case 13:
      key = 0xff0d; // Return or Enter
      break;
    case 27:
      key = 0xff1b; // Escape
      break;
    case 180:
      key = 0xff51; // Left
      break;
    case 181:
      key = 0xff52; // Up
      break;
    case 182:
      key = 0xff54; // Down
      break;
    case 183:
      key = 0xff53; // Right
      break;
    }
    vnc.keyEvent(key, 0b001);
    vnc.keyEvent(key, 0b000);
  }
}

void checkButton(){
  // check for button press
  if ( digitalRead(TRIGGER_PIN) == LOW ) {
    forceConfig=true;
    Serial.println("Button Pressed");
    gfx->println("Button pressed...");
  }
}


void setup() { 
  Serial.begin(115200);
  Serial.setDebugOutput(true);
  while(!Serial);
  Serial.println("AZ-Touch VNC");
  // Init keyboard device
  keyboard_init();

  Serial.println("Init display");
  gfx->begin();
  touch_init(gfx->width(), gfx->height());
  gfx->fillScreen(BLACK);
#ifdef GFX_BL
  pinMode(GFX_BL, OUTPUT);
  // AZ-Touch Mod has inverted backlight, so LOW for  backlight on
  digitalWrite(GFX_BL, LOW);
#endif
  TFTnoWifi();
 

  bool spiffsSetup = loadConfigFile();
  if (!spiffsSetup)
  {
    TFT_WM();
    Serial.println(F("There is no saved config"));
    gfx->println("There is no saved config");
    SPIFFS.format();
    forceConfig = true;
  } else {
    Serial.println("Press Boot Button for WiFiManager");
    gfx->println("Press Boot for WiFiManager");
    delay(1000);
    checkButton();
  }

  // Config Fields for VNC
  WiFiManagerParameter vnc_ip("VNC_IP","IP-Adress of VNC-Server",VNC_IP,50);
  wm.addParameter(&vnc_ip);
  char convertedValue[6];
  sprintf(convertedValue, "%d", VNC_PORT);
  WiFiManagerParameter vnc_port("VNC_Port","port of VNC-Server",convertedValue,6);
  wm.addParameter(&vnc_port);
  WiFiManagerParameter vnc_pw("VNC_PW","password VNC-Server",VNC_PW,50);
  wm.addParameter(&vnc_pw);

  wm.setSaveConfigCallback(saveConfigCallback);
  //set callback that gets called when connecting to previous WiFi fails, and enters Access Point mode
  wm.setAPCallback(configModeCallback);

  bool wm_con;
  if (forceConfig) {
    TFT_WM();
    gfx->println("WLAN: AZ-Touch-VNC");
    if (!wm.startConfigPortal("AZ-Touch-VNC"))
    {
      Serial.println("failed to connect and hit timeout");
      delay(3000);
      //reset and try again, or maybe put it to deep sleep
      ESP.restart();
      delay(5000);
    }
  } else {
    TFTnoWifi();
    gfx->println("Waiting for WLAN ");
    if (!wm.autoConnect("AZ-Touch-VNC"))
    {
      Serial.println("failed to connect and hit timeout");
      delay(3000);
      // if we still have not connected restart and try all over again
      ESP.restart();
      delay(5000);
    }
  }
  Serial.println(" CONNECTED");
  gfx->println(" CONNECTED");
  Serial.println("IP address: ");
  gfx->println("IP address: ");
  Serial.println(WiFi.localIP());
  gfx->println(WiFi.localIP());
  TFTnoVNC();

  //save the custom parameters to FS
  if (shouldSaveConfig)
  {
    saveConfigFile();
  }

  Serial.println(F("[SETUP] VNC..."));
  VNC_PORT = atoi(vnc_port.getValue());
  vnc.begin(VNC_IP, VNC_PORT);
  vnc.setPassword(VNC_PW); // optional
}

void loop()
{
  if (WiFi.status() != WL_CONNECTED)
  {
    vnc.reconnect();
    TFTnoWifi();
    Serial.println("Press Boot Button for WiFiManager");
    gfx->println("Press Boot Button for WiFiManager");
    delay(100);
    checkButton();
  }
  else
  {
    if (vnc.connected())
    {
      handle_touch();
      handle_keyboard();
    }
    vnc.loop();
    if (!vnc.connected())
    {
      TFTnoVNC();
      // some delay to not flood the server
      delay(5000);
    }
  }
}
