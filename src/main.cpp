#include <Arduino.h>

// Libraries
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <TTBOUNCE.h>
#include <EEPROM.h>
extern "C" {
#include "user_interface.h"
}

//======SERVER PART======================================================
const char* ssid     = "kaffee";
const char* password = "kaffeekaffee";
ESP8266WebServer server ( 80 );
char htmlResponse[3000];
//======LOKALER PART======================================================
os_timer_t myTimer;
bool tickOccured = false;
bool timerrunning = false;
bool pressOccured = false;
int RelaisPin = 12;
// GPIO13 Gruene LED auf dem Sonoff
TTBOUNCE button = TTBOUNCE(14);

int time_ss = 1000;
int time_ds = 2000;
int time_max = 15000;
int holdTime = 0;

int attemptMax = 30000;

void press(){
   if (!timerrunning)
   {
     digitalWrite(RelaisPin, HIGH);  // turn Relais ON
     os_timer_arm(&myTimer, time_max, false);
     timerrunning = true;
     pressOccured = true;
     Serial.println("Pressed");
     Serial.println("Relais ON");
   }
  else
     {
     tickOccured = false;
     digitalWrite(RelaisPin, LOW);
     os_timer_disarm(&myTimer);
     timerrunning = false;
     pressOccured = false;
     Serial.println("Abort!");
     Serial.println("Relais OFF");
     }
}

void click(){
   if (!timerrunning)
   {
     digitalWrite(RelaisPin, HIGH);  // turn Relais ON
     os_timer_arm(&myTimer, time_ss, false);
     timerrunning = true;
     Serial.println("Clicked");
     Serial.println("Relais ON");
   }
   else
     {
     tickOccured = false;
     digitalWrite(RelaisPin, LOW);
     os_timer_disarm(&myTimer);
     timerrunning = false;
     Serial.println("Abort!");
     Serial.println("Relais OFF");
     }
}

void doubleClick(){
     if (!timerrunning)
   {
     digitalWrite(RelaisPin, HIGH);  // turn Relais ON
     os_timer_arm(&myTimer, time_ds, false);
     timerrunning = true;
     Serial.println("DoubleClicked");
     Serial.println("Relais ON");
   }
     else
     {
     tickOccured = false;
     digitalWrite(RelaisPin, LOW);
     os_timer_disarm(&myTimer);
     timerrunning = false;
     Serial.println("Abort!");
     Serial.println("Relais OFF");
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

   snprintf ( htmlResponse, 3000,
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
              </html>",

              time_ss,
              time_ds
            );

    server.send ( 200, "text/html", htmlResponse );

}

void handleSave() {
   if (server.arg("ss")!= ""){
     Serial.println("Singleshot: " + server.arg("ss"));
     time_ss = server.arg("ss").toInt();
     eeWriteInt(0, server.arg("ss").toInt());
   }
     if (server.arg("ds")!= ""){
     Serial.println("Doubleshot: " + server.arg("ds"));
     time_ds = server.arg("ds").toInt();
     eeWriteInt(4, server.arg("ds").toInt());
   }

}

// start of timerCallback
void timerCallback(void *pArg) {
  tickOccured = true;
}

void setup() {
  int startwifi = millis();
// Start serial
  Serial.begin(115200);
  delay(10);

//======HARDWARE PART======================================================
  pinMode(RelaisPin, OUTPUT);       // GPIO12 als Ausgang definieren
  digitalWrite(RelaisPin, LOW);  // turn Relais OFF

//======SERVER PART======================================================
// Connecting to a WiFi network
  Serial.println();
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED || millis() >= startwifi + attemptMax) {
    delay(500);
    Serial.print(".");
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("");
    Serial.println("WiFi connected");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
  }

  server.on ( "/", handleRoot );
  server.on ("/save", handleSave);

  server.begin();
  Serial.println ( "HTTP server started" );
//======LOKALER PART======================================================
  os_timer_setfn(&myTimer, timerCallback, NULL);
//  button.enablePullup(); button.setActiveLow();             //ebable internal pullup
  button.disablePullup(); button.setActiveHigh();             // no not enable internale pullup - needs externel pulldown to GND
  button.setDebounceInterval(50);
  button.attachClick(click);         //attach the click method to the click event
  button.attachDoubleClick(doubleClick);//attach the double click method to the double click event
  button.attachPress(press);        //attach the press method to the press event
  tickOccured = false;
  timerrunning = false;
  EEPROM.begin(8);  //Initialize EEPROM
  time_ss = eeGetInt(0);
  time_ds = eeGetInt(4);
}

void loop() {
//======SERVER PART======================================================
   server.handleClient();
//======LOKALER PART======================================================
   button.update();

  if (pressOccured == true) // ein Gedrückthalten wurde erkannt
  {
    if (button.getHoldTime() > 0)
    {
      holdTime = (int)button.getHoldTime();
    }
    if (button.read() == LOW) // warten auf loslassen
    {
      // nach loslassen doublshot Zeit speichern
      eeWriteInt(4, holdTime);
      tickOccured = true;
      pressOccured = false;
      time_ds = holdTime;
      Serial.println("Doubleshot set to " + String(holdTime, DEC) + " ms");
    }
  }

  if (tickOccured == true)
  {
     tickOccured = false;
     digitalWrite(RelaisPin, LOW);
     Serial.println("Timer ausgelaufen");
     Serial.println("Relais OFF");
     os_timer_disarm(&myTimer);
     timerrunning = false;
  }

  yield();  // or delay(0);
}

