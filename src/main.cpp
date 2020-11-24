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

#define ON LOW    // LOW is LED ON
#define OFF HIGH  // HIGH is LED OFF

//======SERVER PART======================================================
const char* ssid     = "kaffee";
const char* password = "kaffeekaffee";
ESP8266WebServer server ( 80 );
char htmlResponse[3000];
//======LOKALER PART======================================================
os_timer_t myTimer;
bool tickOccured = false;
bool pressOccured = false;
bool wifiStatus = false;
int grinderPin = 12;
int ledPin = 13;
TTBOUNCE button = TTBOUNCE(14);  // GPIO 14, last PIN on Header

int time_ss = 1000;
int time_ds = 2000;
int time_max = 15000;
int holdTime = 0;

unsigned int debounceInterval = 50;
unsigned int pressInterval = 500;

void click() {
  if (digitalRead(grinderPin) == LOW) {
    digitalWrite(grinderPin, HIGH);  // turn Relais ON
    os_timer_arm(&myTimer, time_ss, false);
    Serial.println("Clicked");
    Serial.println("Relais " + String(time_ss, DEC) + " ms ON");
    digitalWrite(ledPin, ON);
  } else {
    tickOccured = false;
    digitalWrite(grinderPin, LOW);
    os_timer_disarm(&myTimer);
    Serial.println("Abort!");
    Serial.println("Relais OFF");
    digitalWrite(ledPin, OFF);
  }
}

void doubleClick() {
  if (digitalRead(grinderPin) == LOW) {
    digitalWrite(grinderPin, HIGH);  // turn Relais ON
    os_timer_arm(&myTimer, time_ds, false);
    Serial.println("DoubleClicked");
    Serial.println("Relais " + String(time_ds, DEC) + " ms ON");
    digitalWrite(ledPin, ON);
  } else {
    tickOccured = false;
    digitalWrite(grinderPin, LOW);
    os_timer_disarm(&myTimer);
    Serial.println("Abort!");
    Serial.println("Relais OFF");
    digitalWrite(ledPin, OFF);
  }
}

void press() {
  if(digitalRead(grinderPin) == LOW) {
    digitalWrite(grinderPin, HIGH);  // turn Relais ON
    os_timer_arm(&myTimer, time_max, false);
    pressOccured = true;
    Serial.println("Pressed");
    Serial.println("Relais ON");
    digitalWrite(ledPin, ON);
  } else {
    tickOccured = false;
    digitalWrite(grinderPin, LOW);
    os_timer_disarm(&myTimer);
    pressOccured = false;
    Serial.println("Abort!");
    Serial.println("Relais OFF");
    digitalWrite(ledPin, OFF);
  }
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
                        <h1>Zeiten für Singleshot und Doubleshot</h1>\
                        <h3>Singleshot: %d% ms </h3>\
                        <input type='text' name='date_ss' id='date_ss' size=2 autofocus> ms \
                        <h3>Doubleshot: %d% ms</h3>\
                        <input type='text' name='date_ds' id='date_ds' size=2 autofocus> ms \
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
                      location.reload();\
                    });\
                  </script>\
                </body>\
              </html>", time_ss, time_ds);
    server.send(200, "text/html", htmlResponse);
}

void handleSave() {
  if (server.arg("ss")!= "") {
    Serial.println("Singleshot: " + server.arg("ss"));
    time_ss = server.arg("ss").toInt();
    eeWriteInt(0, server.arg("ss").toInt());
  }
  if (server.arg("ds")!= "") {
    Serial.println("Doubleshot: " + server.arg("ds"));
    time_ds = server.arg("ds").toInt();
    eeWriteInt(4, server.arg("ds").toInt());
  }

}

// handle WiFi and connect if not connected
void handleWifi() {
  static bool oldWifiStaus = false;
  static unsigned long wifi_start = millis();
  unsigned long wifi_int = 30000;
  
  if (WiFi.status() != WL_CONNECTED && millis() > wifi_start + wifi_int) {
    // Connecting to a WiFi network
    Serial.println("WiFi not connected");
    Serial.print("Trying to connect to: ");
    Serial.println(ssid);
    WiFi.begin(ssid, password);
    oldWifiStaus = false;
    wifi_start = millis();
  }
  
  if (oldWifiStaus == false && WiFi.status() == WL_CONNECTED) {
    // Connected to WiFi
    Serial.println("WiFi connected");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
    oldWifiStaus = true;
    
    if (!MDNS.begin("sonoffgrinder")) {             // Start the mDNS responder for esp8266.local
      Serial.println("Error setting up MDNS responder!");
    }
    Serial.println("mDNS responder started");
  }

}

// start of timerCallback
void timerCallback(void *pArg) {
  tickOccured = true;
}


// *******SETUP*******
void setup() {
  Serial.begin(115200); // Start serial

//======HARDWARE PART======================================================
  pinMode(grinderPin, OUTPUT);      // define grinder output pin
  digitalWrite(grinderPin, LOW);    // turn Relais OFF
  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, ON);         // turn LED ON at start

  pinMode(14, INPUT_PULLUP);        // Bugfix ttbounce button.enablePullup(); not working

//======Wifi PART==========================================================
  Serial.print("Connecting to: ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  yield();

//======SERVER PART=======================================================
  server.on ( "/", handleRoot );
  server.on ("/save", handleSave);

  server.begin();
  Serial.println ( "HTTP server started" );

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
    Serial.println("Start updating " + type);
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) {
      Serial.println("Auth Failed");
    } else if (error == OTA_BEGIN_ERROR) {
      Serial.println("Begin Failed");
    } else if (error == OTA_CONNECT_ERROR) {
      Serial.println("Connect Failed");
    } else if (error == OTA_RECEIVE_ERROR) {
      Serial.println("Receive Failed");
    } else if (error == OTA_END_ERROR) {
      Serial.println("End Failed");
    }
  });
  ArduinoOTA.begin();

  digitalWrite(ledPin, OFF);        // turn LED OFF after Setup
}

// *******LOOP*******
void loop() {
//======SERVER PART======================================================
  handleWifi(); // check Wifi
  ArduinoOTA.handle();
  server.handleClient();
  yield();
//======LOKALER PART======================================================
  button.update();

  if (pressOccured == true) {
    // press was detected
    if (button.getHoldTime() > 0) {
      holdTime = (int)button.getHoldTime(); // hold button time
    }
    if (button.read() == LOW) { // wait for button release
      holdTime = holdTime - pressInterval; // substract press detection time from hold time
      eeWriteInt(4, holdTime); // safe doublshot time
      tickOccured = true;
      pressOccured = false;
      time_ds = holdTime;
      Serial.println("Doubleshot set to " + String(holdTime, DEC) + " ms");
    }
  }

  if (tickOccured == true) {
     tickOccured = false;
     digitalWrite(grinderPin, LOW);
     Serial.println("Timer ausgelaufen");
     Serial.println("Relais OFF");
     digitalWrite(ledPin, OFF);
     os_timer_disarm(&myTimer);
  }

}
