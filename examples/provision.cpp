#include <Arduino.h>
#include <SmarterConfig.h>

long lastPrintMain = millis();

void setup() {
  Serial.begin(115200);
  Serial.println("Starting");
  SmarterConfig::start();
  
  while(SmarterConfig::isAwaitingConfig()){
    if(millis()-lastPrintMain > 5000){
        lastPrintMain = millis();
        Serial.print(" Configuring ");
        Serial.println(millis());
    }
  }
  SmarterConfig::stop();
  
}

void loop() {
    if(millis()-lastPrintMain > 5000){
        lastPrintMain = millis();
        Serial.print(" looping ");
        Serial.println(millis());
    }
}