
#include <Arduino.h>
#include "display.hpp"
#ifdef ESP32
  #include <WiFi.h>
#else
  #include <ESP8266WiFi.h>
#endif

Display display = Display();

void setup(void)
{
  display.init();

  display.print(1, "las doce tribus de ", false);
  display.print(2, "123456789012345", false);
  display.print(3, "pepa", true);
  display.print(3, "pip");
  display.print(4, "pepa  4");
  display.print(5, "pepa   5");
  display.print(6, "pepa    6");
  display.print(7, WiFi.macAddress().c_str());
  display.print(8, "pepa      8");
}

int i =0;
void loop(void)
{
  delay(200);
}