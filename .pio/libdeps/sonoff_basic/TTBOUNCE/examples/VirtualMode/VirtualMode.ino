#include <TTBOUNCE.h>

TTBOUNCE b = TTBOUNCE(TTBOUNCE_WITHOUT_PIN); //create virtual button without hardware pin
uint8_t sensorState = LOW;

void setup(){  
  Serial.begin(9600);
}


void loop(){
  sensorState = HIGH;                        //read HIGH/LOW state from sensor here
  b.update(sensorState);
  
  Serial.print("state: ");
  Serial.println(b.read());
  
  delay(10);
}
