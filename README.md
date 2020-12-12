# sonoffgrinder
This is a Coffee Grinder Controller with a Sonoff Basic as a Timer.

## Setup
Connect the Motor of the Coffee Grinder to the Output of the Sonoff and the Pushbutton to Pin 14 and Ground.
Connect the TX to SCK and RX to SDA of the Display. (0,96" OLED)


## Usage

The Controller tries to connect to  WiFi. Just set up your local Wifi before flashing the Sonoff.
You can connect to the Sonoff by typing "sonoffgrinder/" in your Webbrowser.
The Webinterface is presented to programm the grinding time for one and doubleClick.

There are 3 Operation-Modes:
- Single Push: Grinding for the Time you set in the Webinterface
- Hold: Grinding as long as the Button is helt down and safe the time on the Doubleclick
- Double Push: Grinding the same time like the last grind by push and hold

If you push the button again while the machine is grinding the grinder will stop.

Thanks to timo.helfer @ www.kaffeenetz.de


## Look

[![Video: Sonoffgrinder Display](http://img.youtube.com/vi/cjG6sKxTZSo/0.jpg)](http://www.youtube.com/watch?v=cjG6sKxTZSo "Video: Sonoffgrinder Display")
