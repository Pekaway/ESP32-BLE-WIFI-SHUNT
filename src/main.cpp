#include <Logger.h>

void setup()
{
    Serial.begin(9600);
    Logger.begin(&Serial, Level::ALL);
}

void loop()
{
}