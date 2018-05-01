#pragma once

void blink(uint8_t count)
{
	for(int i = 0; i < count; i++)
	{
		digitalWrite(DEBUG_LED_PIN, DEBUG_LED_ON);
		delay(100);
		digitalWrite(DEBUG_LED_PIN, !DEBUG_LED_ON);
		delay(100);
	}
}