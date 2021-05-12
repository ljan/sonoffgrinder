#include <Arduino.h>
// Libraries
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <ESP8266WebServer.h>
#include <TTBOUNCE.h>
#include <EEPROM.h>
// OTA
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
// OLED
#include "SSD1306Brzo.h"

#define ON LOW    // LOW is LED ON
#define OFF HIGH  // HIGH is LED OFF

#define SCL 1 // Tx Pin on Sonoff
#define SDA 3 // Rx Pin on Sonoff

#define VERSION "v0.4"
#define OVERLAYTIME 5*60*1000 // show additional information for this period

//======SERVER PART======================================================
const char* ssid     = "kaffee";
const char* password = "kaffeekaffee";
ESP8266WebServer server ( 80 );
char htmlResponse[3000];
//======LOKALER PART======================================================
os_timer_t myTimer;
bool clickOccured = false;
bool dclickOccured = false;
bool pressOccured = false;
bool wifiStatus = false;
bool hideOverlay = false;
int grinderPin = 12;
int ledPin = 13;
TTBOUNCE button = TTBOUNCE(14);  // GPIO14, last PIN on Header
SSD1306Brzo  display(0x3c, SDA, SCL); // ADDRESS, SDA, SCL

int time_ss = 1000;
int time_ds = 2000;
int time_max = 50000;
int holdTime = 0;
int relaisTime = 0;
unsigned long relaisStart = 0;

unsigned int debounceInterval = 50;
unsigned int pressInterval = 500;

void click() {
  if (digitalRead(grinderPin) == LOW) {
    digitalWrite(grinderPin, HIGH);  // turn Relais ON
    os_timer_arm(&myTimer, time_ss, false);
    clickOccured = true;
    relaisTime = time_ss;
    relaisStart = millis();
    //Serial.println("Clicked");
    //Serial.println("Relais " + String(time_ss, DEC) + " ms ON");
    digitalWrite(ledPin, ON);
  } else {
    digitalWrite(grinderPin, LOW);
    os_timer_disarm(&myTimer);
    clickOccured = false;
    dclickOccured = false;
    //Serial.println("Abort!");
    //Serial.println("Relais OFF");
    digitalWrite(ledPin, OFF);
  }
}

void doubleClick() {
  if (digitalRead(grinderPin) == LOW) {
    digitalWrite(grinderPin, HIGH);  // turn Relais ON
    os_timer_arm(&myTimer, time_ds, false);
    dclickOccured = true;
    relaisTime = time_ds;
    relaisStart = millis();
    //Serial.println("DoubleClicked");
    //Serial.println("Relais " + String(time_ds, DEC) + " ms ON");
    digitalWrite(ledPin, ON);
  } else {
    digitalWrite(grinderPin, LOW);
    os_timer_disarm(&myTimer);
    clickOccured = false;
    dclickOccured = false;
    //Serial.println("Abort!");
    //Serial.println("Relais OFF");
    digitalWrite(ledPin, OFF);
  }
}

void press() {
  if(digitalRead(grinderPin) == LOW || (clickOccured == true || dclickOccured == true)) {
    digitalWrite(grinderPin, HIGH);  // turn Relais ON
    os_timer_arm(&myTimer, time_max, false);
    pressOccured = true;
    relaisTime = time_max;
    if (clickOccured == false && dclickOccured == false) {
      relaisStart = millis();
    }
    //Serial.println("Pressed");
    //Serial.println("Relais ON");
    digitalWrite(ledPin, ON);
  } else {
    digitalWrite(grinderPin, LOW);
    os_timer_disarm(&myTimer);
    pressOccured = false;
    //Serial.println("Abort!");
    //Serial.println("Relais OFF");
    digitalWrite(ledPin, OFF);
  }
}

void timerCallback(void *pArg) {
  // start of timerCallback
  digitalWrite(grinderPin, LOW);
  //Serial.println("Timer ausgelaufen");
  //Serial.println("Relais OFF");
  digitalWrite(ledPin, OFF);
  os_timer_disarm(&myTimer);
  clickOccured = false;
  dclickOccured = false; // TODO besser?
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
                        <h3>Single Shot (Click): %d% ms </h3>\
                        <h3><input type='text' name='date_ss' id='date_ss' size=2 autofocus>  ms</h3> \
                        <h3>Double Shot (Doubleclick): %d% ms</h3>\
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
              </html>", time_ss, time_ds);
    server.send(200, "text/html", htmlResponse);
}

void handleSave() {
  // saving times via web
  if (server.arg("ss")!= "") {
    //Serial.println("Singleshot: " + server.arg("ss"));
    time_ss = server.arg("ss").toInt();
    eeWriteInt(0, server.arg("ss").toInt());
  }
  if (server.arg("ds")!= "") {
    //Serial.println("Doubleshot: " + server.arg("ds"));
    time_ds = server.arg("ds").toInt();
    eeWriteInt(4, server.arg("ds").toInt());
  }
}

void handleWifi() {
  // handle WiFi and connect if not connected
  static bool lastWifiStatus = false;
  static unsigned long wifi_restart = millis();
  unsigned long wifi_interval = 500;

  if (WiFi.status() == WL_CONNECTED) {
    // Connected to WiFi
    if (lastWifiStatus == false) {
      // Newly connected to WiFi
      //Serial.println("WiFi connected");
      //Serial.println("IP address: ");
      //Serial.println(WiFi.localIP());
      lastWifiStatus = true;
      if (!MDNS.begin("sonoffgrinder")) {             // Start the mDNS responder for esp8266.local
        //Serial.println("Error setting up MDNS responder!");
      }
      //Serial.println("mDNS responder started");
    }
    if(hideOverlay == false) {
      display.setTextAlignment(TEXT_ALIGN_RIGHT);
      display.setFont(ArialMT_Plain_10);
      display.drawString(128, 54, WiFi.localIP().toString());
    }
  }

  if (WiFi.status() != WL_CONNECTED) {
    // Waiting to connect to a WiFi network
    //Serial.println("WiFi not connected");
    //Serial.print("Trying to connect to: ");
    //Serial.println(ssid);
    if (lastWifiStatus == true) {
      // (Re)start WiFi 
      WiFi.begin(ssid, password);
      lastWifiStatus = false;
      wifi_restart = millis();
    }
    if (millis() > wifi_restart) {
      wifi_restart = millis() + wifi_interval; // next try
      delay(10); // allow WiFi and TCP/IP tasks to run
    }
    display.setTextAlignment(TEXT_ALIGN_RIGHT);
    display.setFont(ArialMT_Plain_10);
    display.drawString(128, 54, ssid);
  }

  yield();
}

void handleDisplay() {
  // Handle Display
  
  if(pressOccured == false && (clickOccured == true || dclickOccured == true)){
    hideOverlay = true;
    // display grinding progress
    display.setTextAlignment(TEXT_ALIGN_LEFT);
    display.setFont(ArialMT_Plain_10);
    if(clickOccured == true) {
      display.drawString(0, 0, "Single Shot Grinding");
    } else if (dclickOccured == true) {
      display.drawString(0, 0, "Double Shot Grinding");
    }
    display.setTextAlignment(TEXT_ALIGN_RIGHT);
    display.setFont(ArialMT_Plain_16);
    display.drawString(128, 16, String((millis() - relaisStart)/1000.0,2) + "/" + String(relaisTime/1000.0,2) + " s");
    display.drawProgressBar(0, 38, 127, 10, (millis() - relaisStart) / (relaisTime / 100));
    //display.drawString(64, 45, String((millis() - relaisStart) / (relaisTime / 100)) + " %");
  }

  if(pressOccured == true){
    hideOverlay = true;
    // display grinding setup progress
    display.setTextAlignment(TEXT_ALIGN_LEFT);
    display.setFont(ArialMT_Plain_10);
    if(clickOccured == true) {
      display.drawString(0, 0, "Saving Single Time");
    } else if (dclickOccured == true) {
      display.drawString(0, 0, "Saving Double Time");
    } else {
      display.drawString(0, 0, "Manual Grinding");
    }
    display.setTextAlignment(TEXT_ALIGN_RIGHT);
    display.setFont(ArialMT_Plain_16);
    display.drawString(128, 16, String(millis() - relaisStart) + " ms");
    display.drawProgressBar(0, 38, 127, 10, (millis() - relaisStart) / (relaisTime / 100));
  }

  if(clickOccured == false && dclickOccured == false && pressOccured == false){
    if(millis() < OVERLAYTIME) { 
      hideOverlay = false;
    }
    display.setTextAlignment(TEXT_ALIGN_RIGHT);
    display.setFont(ArialMT_Plain_16);
    display.drawString(128, 16, "Single " + String(time_ss/1000.0,2) + " s");
    display.drawString(128, 34, "Double " + String(time_ds/1000.0,2) + " s");
  }
  
  // Overlay
  if(hideOverlay == false) {
    display.setTextAlignment(TEXT_ALIGN_LEFT);
    display.setFont(ArialMT_Plain_10);
    display.drawString(0, 0, "SonoffGrinder");
    display.setTextAlignment(TEXT_ALIGN_RIGHT);
    display.setFont(ArialMT_Plain_10);
    display.drawString(128, 0, VERSION);
  }

  display.display();
}

// *******SETUP*******
void setup() {
  //Serial.begin(115200); // Start serial

//======HARDWARE PART======================================================
  pinMode(grinderPin, OUTPUT);      // define grinder output pin
  digitalWrite(grinderPin, LOW);    // turn Relais OFF
  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, ON);         // turn LED ON at start

  pinMode(14, INPUT_PULLUP);        // Bugfix ttbounce button.enablePullup(); not working

  display.init();
  display.flipScreenVertically();
  handleDisplay();

//======Wifi PART==========================================================
  //Serial.print("Connecting to: ");
  //Serial.println(ssid);
  WiFi.mode(WIFI_STA); // explicitly set the ESP8266 to be a WiFi-client
  WiFi.persistent(false); // do not store settings in EEPROM
  WiFi.begin(ssid, password);
  yield();

//======SERVER PART=======================================================
  server.on("/", handleRoot );
  server.on("/save", handleSave);

  server.begin();
  //Serial.println ( "HTTP server started" );

//======LOKALER PART======================================================
  os_timer_setfn(&myTimer, timerCallback, NULL);
  button.setActiveLow(); button.enablePullup();   // button from GPIO directly to GND, ebable internal pullup
  button.setDebounceInterval(debounceInterval);
  button.setPressInterval(pressInterval);
  button.attachClick(click);                      //attach the click method to the click event
  button.attachDoubleClick(doubleClick);          //attach the double click method to the double click event
  button.attachPress(press);                      //attach the press method to the press event

  EEPROM.begin(8);  //Initialize EEPROM
  time_ss = eeGetInt(0);
  time_ds = eeGetInt(4);

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
    digitalWrite(ledPin, ON);
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

  digitalWrite(ledPin, OFF);        // turn LED OFF after Setup
}

// *******LOOP*******
void loop() {
  display.clear();
//======SERVER PART======================================================
  handleWifi(); // check Wifi
  ArduinoOTA.handle();
  server.handleClient();
//======LOKALER PART======================================================
  button.update();

  if(pressOccured == true) {
    // press was detected
    if (button.getHoldTime() > 0) {
      // while button is pressed
      holdTime = (int)button.getHoldTime(); // hold button time
    }
    if(button.read() == LOW) { // wait for button release
      holdTime = millis() - relaisStart; // substract press detection time from hold time
      if (clickOccured == true) {
        eeWriteInt(0, holdTime); // safe singleshot time
        clickOccured = false;
        time_ss = holdTime;
      }
      else if (dclickOccured == true) {
        eeWriteInt(4, holdTime); // safe doubleshot time
        dclickOccured = false;
        time_ds = holdTime;
      }
      pressOccured = false;
      os_timer_arm(&myTimer, 0, false); // instantly fire timer
      //Serial.println("Doubleshot set to " + String(holdTime, DEC) + " ms");
    }
  }

  handleDisplay();
  delay(100);
}
