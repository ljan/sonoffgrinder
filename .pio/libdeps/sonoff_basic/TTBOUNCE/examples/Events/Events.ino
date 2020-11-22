#include <TTBOUNCE.h>

TTBOUNCE b = TTBOUNCE(3);           //create button instance and attach digital pin 3

void setup(){
  b.setDebounceInterval(20);        //default is 10ms

  b.setClickInterval(200);          //a click is detected if button is released before this interval (default is 300ms)
  b.attachClick(click);			        //attach the click method to the click event
  
  b.attachDoubleClick(doubleClick); //attach the double click method to the double click event
  
  b.setPressInterval(800);          //a long press is detected if button is held longer than this interval (default is 1000ms)
  b.attachPress(press); 		        //attach the press method to the press event
  
  b.setReTickInterval(100);         //a retick is fired as long the button is pressed (after press interval is lapsed) (default is 200ms)
  b.attachReTick(reTick);           //attach the retick method to the retick event

  b.attachRelease(release);         //attach the release method to the release event (fired when button is released after click/press)

  Serial.begin(9600);
}

void loop(){
  b.update();
  
  Serial.print("state: ");
  Serial.print(b.read());
  
  Serial.print(" time: ");
  Serial.println(b.getHoldTime());
  
  delay(10);
}

void click(){
  Serial.println("Clicked");
}

void doubleClick(){
  Serial.println("Double clicked");
}

void press(){
  Serial.println("Long pressed");
}

void reTick(){
  Serial.println("Reticked");
}

void release(){
  Serial.println("Button released");
}