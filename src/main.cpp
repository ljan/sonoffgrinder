#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <ESP8266WebServer.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <EEPROM.h>
#include <TTBOUNCE.h>
#include <SSD1306Brzo.h>

// Defines
#define VERSION "v1.0"
#define NAME "SonoffGrinder"
#define SONOFFBASIC // Use Sonoff Basic default Pins

#define ON LOW    // LOW is LED ON
#define OFF HIGH  // HIGH is LED OFF

// Configuration
const char* cSSID     = "coffee";
const char* cPASSWORD = "coffeecoffee";

const bool bFlipDisplay = true;

#ifdef SONOFFBASIC
const int pGRINDER = 12; // Relay Pin of Sonoff
const int pLED = 13; // LED Pin of Sonoff
const int pBUTTON = 14; // GPIO14, last Pin on Header

const int pSCL = 1; // Tx Pin on Sonoff
const int pSDA = 3; // Rx Pin on Sonoff
#else // Custom or Development Board
const int pGRINDER = 12; // Relay Pin of Sonoff
const int pLED = 16; // LED Pin of Sonoff
const int pBUTTON = 0; // GPIO14, last Pin on Header

const int pSCL = 5; // Tx Pin on Sonoff
const int pSDA = 2; // Rx Pin on Sonoff
#endif

const unsigned long tMAX = 60000; // Maxmimum Time for grinding
const unsigned long tOVERLAY = 60000; // show additional information for this period
const unsigned long tDEBOUNCE = 50; // Debounce Time for Button
const unsigned long tPRESS = 300; // Time for Button Press Detection

char htmlResponse[3000];

bool bClick = false;
bool bDoubleClick = false;
bool bPress = false;

bool bWifiConnected = true; // initialize true to start Connection
bool bShowOverlay = true; // initialize true

int tSingleShot = 5000;
int tDualShot = 10000;

int tGrindDuration = 0;
int tGrindPeriod = 0;
unsigned long tGrindStart = 0;

os_timer_t timerGRINDER;

ESP8266WebServer server (80);
TTBOUNCE button = TTBOUNCE(pBUTTON);
SSD1306Brzo  display(0x3c, pSDA, pSCL); // ADDRESS, SDA, SCL

void click() {
  if (digitalRead(pGRINDER) == LOW) {
    digitalWrite(pGRINDER, HIGH);  // turn Relais ON
    os_timer_arm(&timerGRINDER, tSingleShot, false);
    bClick = true;
    tGrindPeriod = tSingleShot;
    tGrindStart = millis();
    //Serial.println("Clicked");
    //Serial.println("Relais " + String(tSingleShot, DEC) + " ms ON");
    digitalWrite(pLED, ON);
  } else {
    digitalWrite(pGRINDER, LOW);
    os_timer_disarm(&timerGRINDER);
    bClick = false;
    bDoubleClick = false;
    //Serial.println("Abort!");
    //Serial.println("Relais OFF");
    digitalWrite(pLED, OFF);
  }
}

void doubleClick() {
  if (digitalRead(pGRINDER) == LOW) {
    digitalWrite(pGRINDER, HIGH);  // turn Relais ON
    os_timer_arm(&timerGRINDER, tDualShot, false);
    bDoubleClick = true;
    tGrindPeriod = tDualShot;
    tGrindStart = millis();
    //Serial.println("DoubleClicked");
    //Serial.println("Relais " + String(tDualShot, DEC) + " ms ON");
    digitalWrite(pLED, ON);
  } else {
    digitalWrite(pGRINDER, LOW);
    os_timer_disarm(&timerGRINDER);
    bClick = false;
    bDoubleClick = false;
    //Serial.println("Abort!");
    //Serial.println("Relais OFF");
    digitalWrite(pLED, OFF);
  }
}

void press() {
  if(digitalRead(pGRINDER) == LOW || (bClick == true || bDoubleClick == true)) {
    digitalWrite(pGRINDER, HIGH);  // turn Relais ON
    bPress = true;
    tGrindPeriod = tMAX;
    if (bClick == false && bDoubleClick == false) {
      os_timer_arm(&timerGRINDER, tMAX, false);
      tGrindStart = millis(); // only initialize if manual Mode
    } else {
      os_timer_arm(&timerGRINDER, tMAX - (millis() - tGrindStart), false);
    }
    //Serial.println("Pressed");
    //Serial.println("Relais ON");
    digitalWrite(pLED, ON);
  } else {
    digitalWrite(pGRINDER, LOW);
    os_timer_disarm(&timerGRINDER);
    bPress = false;
    //Serial.println("Abort!");
    //Serial.println("Relais OFF");
    digitalWrite(pLED, OFF);
  }
}

void timerCallback(void *pArg) {
  // start of timerCallback
  digitalWrite(pGRINDER, LOW);
  //Serial.println("Timer expired");
  //Serial.println("Relais OFF");
  digitalWrite(pLED, OFF);
  os_timer_disarm(&timerGRINDER);
  bClick = false;
  bDoubleClick = false;
  bPress = false;
}

void eeWriteInt(int pos, int val) {
  byte* p = (byte*) &val;
  EEPROM.write(pos, *p);
  EEPROM.write(pos + 1, *(p + 1));
  EEPROM.write(pos + 2, *(p + 2));
  EEPROM.write(pos + 3, *(p + 3));
  EEPROM.commit();
}

int eeGetInt(int pos) {
  int val;
  byte* p = (byte*) &val;
  *p        = EEPROM.read(pos);
  *(p + 1)  = EEPROM.read(pos + 1);
  *(p + 2)  = EEPROM.read(pos + 2);
  *(p + 3)  = EEPROM.read(pos + 3);
  return val;
}

void handleRoot() {
   snprintf(htmlResponse, 3000,
              "<!DOCTYPE html>\
              <html lang=\"en\">\
                <head>\
                  <meta charset=\"utf-8\">\
                  <meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">\
                </head>\
                <body>\
                        <h1>Set Time for single Shot and double Shot</h1>\
                        <h3>Single Shot (Click): %d ms </h3>\
                        <h3><input type='text' name='date_ss' id='date_ss' size=2 autofocus>  ms</h3> \
                        <h3>Double Shot (Doubleclick): %d ms</h3>\
                        <h3><input type='text' name='date_ds' id='date_ds' size=2 autofocus>  ms</h3> \
                        <h3>You can press and hold the Button for manual Grinding.<br> If you press and hold after a Click or Doubleclick it will save the Time.</h3>\
                        <div>\
                        <br><button id=\"save_button\">Save</button>\
                        </div>\
                  <script src=\"https://ajax.googleapis.com/ajax/libs/jquery/1.11.3/jquery.min.js\"></script>\
                  <script>\
                    var ds;\
                    var ss;\
                    $('#save_button').click(function(e){\
                      e.preventDefault();\
                      ss = $('#date_ss').val();\
                      ds = $('#date_ds').val();\
                      $.get('/save?ss=' + ss + '&ds=' + ds, function(data){\
                        console.log(data);\
                      });\
                    });\
                  </script>\
                </body>\
              </html>", tSingleShot, tDualShot);
    server.send(200, "text/html", htmlResponse);
}

void handleSave() {
  // saving times via web
  if (server.arg("ss")!= "") {
    //Serial.println("Singleshot: " + server.arg("ss"));
    tSingleShot = server.arg("ss").toInt();
    eeWriteInt(0, server.arg("ss").toInt());
  }
  if (server.arg("ds")!= "") {
    //Serial.println("Doubleshot: " + server.arg("ds"));
    tDualShot = server.arg("ds").toInt();
    eeWriteInt(4, server.arg("ds").toInt());
  }
}

void handleWifi() {
  // handle WiFi and connect if not connected

  if (WiFi.status() == WL_CONNECTED) {
    // Connected to WiFi
    if (bWifiConnected == false) {
      // Newly connected to WiFi
      //Serial.println("WiFi connected");
      //Serial.println("IP address: ");
      //Serial.println(WiFi.localIP());
      bWifiConnected = true;
      if (!MDNS.begin(NAME)) { // Start the mDNS responder for esp8266.local
        //Serial.println("Error setting up MDNS responder!");
      }
      //Serial.println("mDNS responder started");
    }
  }

  if (WiFi.status() != WL_CONNECTED) {
    if (bWifiConnected == true) {
      // (Re)start WiFi 
      //Serial.println("WiFi not connected");
      //Serial.print("Trying to connect to: ");
      //Serial.println(cSSID);
      WiFi.mode(WIFI_STA); // explicitly set the ESP8266 to be a WiFi-Client
      WiFi.persistent(false); // do not store Settings in EEPROM
      WiFi.hostname(NAME);
      WiFi.begin(cSSID, cPASSWORD);
      bWifiConnected = false;
    }
  }

  yield(); // allow WiFi and TCP/IP Tasks to run
  delay(10); // yield not sufficient
}

void handleDisplay() {
  display.clear();

  if((bClick == true || bDoubleClick == true) && bPress == false){
    // display timer grinding Progress
    bShowOverlay = false;
    display.setTextAlignment(TEXT_ALIGN_LEFT);
    display.setFont(ArialMT_Plain_10);
    if(bClick == true) {
      display.drawString(0, 0, "Single Shot Grinding");
    } else if (bDoubleClick == true) {
      display.drawString(0, 0, "Double Shot Grinding");
    }
    display.setTextAlignment(TEXT_ALIGN_RIGHT);
    display.setFont(ArialMT_Plain_16);
    display.drawString(128, 16, String((millis() - tGrindStart)/1000.0,2) + "/" + String(tGrindPeriod/1000.0,2) + " s");
    display.drawProgressBar(0, 38, 127, 10, (millis() - tGrindStart) / (tGrindPeriod / 100));
  }

  if(bPress == true){
    // display manual grinding and setup Progress
    bShowOverlay = false;
    display.setTextAlignment(TEXT_ALIGN_LEFT);
    display.setFont(ArialMT_Plain_10);
    if(bClick == true) {
      display.drawString(0, 0, "Saving Single Shot Time");
    } else if (bDoubleClick == true) {
      display.drawString(0, 0, "Saving Double Shot Time");
    } else {
      display.drawString(0, 0, "Manual Grinding");
    }
    display.setTextAlignment(TEXT_ALIGN_RIGHT);
    display.setFont(ArialMT_Plain_16);
    display.drawString(128, 16, String(millis() - tGrindStart) + " ms");
    display.drawProgressBar(0, 38, 127, 10, (millis() - tGrindStart) / (tGrindPeriod / 100));
  }

  if(bClick == false && bDoubleClick == false && bPress == false){
    // default Screen
    if(millis() > tOVERLAY) { 
      bShowOverlay = false; // disable Overlay
    } else {
      bShowOverlay = true; // reenable Overlay
    }
    display.setTextAlignment(TEXT_ALIGN_RIGHT);
    display.setFont(ArialMT_Plain_16);
    display.drawString(128, 16, "Single " + String(tSingleShot/1000.0,2) + " s");
    display.drawString(128, 34, "Double " + String(tDualShot/1000.0,2) + " s");
  }
  
  if(bShowOverlay == true) {
    // display additional Information in Overlay
    display.setTextAlignment(TEXT_ALIGN_LEFT);
    display.setFont(ArialMT_Plain_10);
    display.drawString(0, 0, NAME);
    display.setTextAlignment(TEXT_ALIGN_RIGHT);
    display.setFont(ArialMT_Plain_10);
    display.drawString(128, 0, VERSION);

    if (bWifiConnected == true) {
      // show IP when connected
      display.setTextAlignment(TEXT_ALIGN_RIGHT);
      display.setFont(ArialMT_Plain_10);
      display.drawString(128, 54, WiFi.localIP().toString());
    }
  }

  if (bWifiConnected == false) {
      // show SSID when not connected
      display.setTextAlignment(TEXT_ALIGN_RIGHT);
      display.setFont(ArialMT_Plain_10);
      display.drawString(128, 54, cSSID);
    }

  display.display();
}

// *******SETUP*******
void setup() {
  //Serial.begin(115200); // Start serial

  pinMode(pGRINDER, OUTPUT);      // define Grinder output Pin
  digitalWrite(pGRINDER, LOW);    // turn Relais OFF
  pinMode(pLED, OUTPUT);
  digitalWrite(pLED, ON);         // turn LED ON at start

  pinMode(pBUTTON, INPUT_PULLUP);        // Bugfix ttbounce button.enablePullup(); not working

  handleWifi();

  display.init();
  if(bFlipDisplay == true) {display.flipScreenVertically();}
  handleDisplay();

  server.on("/", handleRoot );
  server.on("/save", handleSave);
  server.begin();
  //Serial.println ( "HTTP Server started" );

  os_timer_setfn(&timerGRINDER, timerCallback, NULL);

  button.setActiveLow(); button.enablePullup();   // Button from GPIO directly to GND, ebable internal pullup
  button.setDebounceInterval(tDEBOUNCE);
  button.setPressInterval(tPRESS);
  button.attachClick(click);                      // attach the Click Method to the Click Event
  button.attachDoubleClick(doubleClick);          // attach the double Click Method to the double Click Event
  button.attachPress(press);                      // attach the Press Method to the Press Event

  EEPROM.begin(8);  // Initialize EEPROM
  tSingleShot = eeGetInt(0);
  tDualShot = eeGetInt(4);

//======OTA PART=========================================================
  // Port defaults to 8266
  // ArduinoOTA.setPort(8266);

  // Hostname defaults to esp8266-[ChipID]
  // ArduinoOTA.setHostname("myesp8266");

  // No authentication by default
  // ArduinoOTA.setPassword("admin");

  // Password can be set with it's md5 value as well
  // MD5(admin) = 21232f297a57a5a743894a0e4a801fc3
  // ArduinoOTA.setPasswordHash("21232f297a57a5a743894a0e4a801fc3");

  ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH) {
      type = "sketch";
    } else { // U_FS
      type = "filesystem";
    }
    // NOTE: if updating FS this would be the place to unmount FS using FS.end()
    //Serial.println("Start updating " + type);
    digitalWrite(pLED, ON);
    os_timer_arm(&timerGRINDER, 0, false); // instantly fire Timer
  });
  ArduinoOTA.onEnd([]() {
    //Serial.println("\nEnd");
    display.clear();
    display.setTextAlignment(TEXT_ALIGN_CENTER);
    display.setFont(ArialMT_Plain_16);
    display.drawString(64, 10, "OTA Finsihed");
    display.drawProgressBar(0, 32, 127, 10, 100);
    display.drawString(64, 45, "rebooting..");
    display.display();
    // eeWriteInt(0, 1000); // for testing
    // eeWriteInt(4, 2000); // for testing
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    //Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
    display.clear();
    display.setTextAlignment(TEXT_ALIGN_CENTER);
    display.setFont(ArialMT_Plain_16);
    display.drawString(64, 10, "OTA Update");
    display.drawProgressBar(0, 32, 127, 10, progress / (total / 100));
    display.drawString(64, 45, String(progress / (total / 100)) + " %");
    display.display();
  });
  ArduinoOTA.onError([](ota_error_t error) {
    //Serial.printf("Error[%u]: ", error);
    display.clear();
    display.setTextAlignment(TEXT_ALIGN_CENTER);
    display.setFont(ArialMT_Plain_16);
    display.drawString(64, 10, "OTA Error");
    display.drawProgressBar(0, 32, 127, 10, 100);
    if (error == OTA_AUTH_ERROR) {
      //Serial.println("Auth Failed");
      display.drawString(64, 45, "Auth Failed");
    } else if (error == OTA_BEGIN_ERROR) {
      //Serial.println("Begin Failed");
      display.drawString(64, 45, "Begin Failed");
    } else if (error == OTA_CONNECT_ERROR) {
      //Serial.println("Connect Failed");
      display.drawString(64, 45, "Connect Failed");
    } else if (error == OTA_RECEIVE_ERROR) {
      //Serial.println("Receive Failed");
      display.drawString(64, 45, "Receive Failed");
    } else if (error == OTA_END_ERROR) {
      //Serial.println("End Failed");
      display.drawString(64, 45, "End Failed");
    }
    display.display();
    delay(1000);
  });
  ArduinoOTA.begin();

  digitalWrite(pLED, OFF);        // turn LED OFF after Setup
}

// *******LOOP*******
void loop() {
  ArduinoOTA.handle();

  server.handleClient();
  
  button.update();

  if(bPress == true) {
    // Press was detected
    if(button.read() == LOW) { // wait for Button release
      tGrindDuration = millis() - tGrindStart;
      os_timer_arm(&timerGRINDER, 0, false); // instantly fire Timer
      bPress = false;
      if (bClick == true) {
        eeWriteInt(0, tGrindDuration); // safe single Shot Time
        bClick = false;
        tSingleShot = tGrindDuration;
        //Serial.println("Single Shot set to " + String(tGrindDuration, DEC) + " ms");
      }
      else if (bDoubleClick == true) {
        eeWriteInt(4, tGrindDuration); // safe double Shot Time
        bDoubleClick = false;
        tDualShot = tGrindDuration;
        //Serial.println("Double Shot set to " + String(tGrindDuration, DEC) + " ms");
      }
    }
  }

  handleWifi();
  handleDisplay();
}
