#include <TTBOUNCE.h>

TTBOUNCE b = TTBOUNCE(3);          //create button instance and attach digital pin 3

void setup(){  
  b.setDebounceInterval(50);       //set debouncing time in ms (default well be 10ms)
  b.setActiveLow();                //digitalRead == LOW means button is pressed (default will be activeHigh)
  b.enablePullup();                //enable internal pullup resistor

  Serial.begin(9600);
}


void loop(){
  b.update();
  
  Serial.print("state: ");
  Serial.println(b.read());
  
  delay(10);
}
