#include "application.h"

int switch_pin = D5;
int led_pin    = D1;
int button_pin = D3;

int is_armed         = false;
int has_been_pressed = false;

int is_button_pressed()
{
	int button_pressed = true;
	int i;

	for(i = 0; i < 20; ++i)
	{
		button_pressed &= ~digitalRead(button_pin);
		delay(1);
	}
	return button_pressed;
}

void setup()
{
	pinMode(switch_pin, INPUT);
	pinMode(button_pin, INPUT);
	pinMode(led_pin, OUTPUT);
}

void loop()
{
	is_armed = digitalRead(switch_pin);
	if (is_armed)
	{
		has_been_pressed |= is_button_pressed();
		if (has_been_pressed)
		{
			digitalWrite(led_pin, HIGH);
			delay(100);
			digitalWrite(led_pin, LOW);
			delay(100);
		}
		else
		{
			digitalWrite(led_pin, HIGH);
		}
	}
	else
	{
		digitalWrite(led_pin, LOW);
		has_been_pressed = false;
	}
}
