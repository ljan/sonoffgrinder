# SonoffGrinder
This is a Coffee Grinder Controller with a Sonoff Basic as a Timer.

## Setup
Connect the Motor of the Coffee Grinder to the Output of the Sonoff and the Pushbutton to GPIO 14 and Ground.
Connect TX to SCK and RX to SDA of the Display. (0,96" OLED)


## Usage
The Controller tries to connect to specified WiFi. Just set up your local Wifi in Code before flashing the Sonoff.
You can connect to the Sonoff by typing "SonoffGrinder/" or the IP in your Webbrowser.
The Webinterface is presented to programm the grinding Time for single and double Shot.

There are 3 Operation-Modes:
- Single: Grinding for the Time you set for Singleshot
- Double: Grinding for the Time you set for Doubleshot
- Manual: Grinding as long as the Button is held

If you push the button again while the machine is grinding the grinder will stop.

Saving new Times:
- Start Single- or Doubleshot grinding
- Push the Button again and hold as long as you want
- New grinding Time is saved for Single- or Doubleshot

Thanks to timo.helfer @ www.kaffeenetz.de for the inspiration and codebase.


## Video

[![Video: Sonoffgrinder Display](http://img.youtube.com/vi/cjG6sKxTZSo/0.jpg)](http://www.youtube.com/watch?v=cjG6sKxTZSo "Video: Sonoffgrinder Display")
