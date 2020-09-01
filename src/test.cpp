
#include <Arduino.h>
#include "display.hpp"

Display display = Display();

void setup(void)
{
  display.init();

  display.print(1, "las doce tribus de mahadaonda");
  display.print(2, "123456789012345");
  display.print(3, "pepa");
  display.print(3, "pip");
  display.print(4, "pepa  4");
  display.print(5, "pepa   5");
  display.print(6, "pepa    6");
  display.print(7, "pepa     7");
  display.print(8, "pepa      8");
}

void loop(void)
{
  delay(2000);
}