#include <Arduino.h>

void setup() 
{
  Serial.begin(9600);
}
 
void loop() 
{
  float sensorVoltage; 
  float sensorValue;
 
  sensorValue = analogRead(36);
  sensorVoltage = sensorValue/4095*3.3;
  Serial.print("sensor reading = ");
  Serial.print(sensorValue);
  Serial.println("");
  Serial.print("sensor voltage = ");
  Serial.print(sensorVoltage);
  Serial.println(" V");
  delay(1000);
}