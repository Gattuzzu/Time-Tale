#include <Arduino.h>
#include "logger/Logger.h"
#include "logger/LogLevel.h"

// put function declarations here:
int myFunction(int, int);

void setup() {
  Logger::setup();
  Logger::log(LogLevel::Info, "Programm gestartet!");
  
}

void loop() {
  // put your main code here, to run repeatedly:
}
