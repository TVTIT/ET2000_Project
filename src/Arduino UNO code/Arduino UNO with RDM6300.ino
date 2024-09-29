#include "rdm6300.h"

#define RDM6300_RX_PIN 2
#define READ_LED_PIN 13
#define READ_BUZZER_PIN 7

Rdm6300 rdm6300;

void setup()
{
	Serial.begin(9600);

	pinMode(READ_LED_PIN, OUTPUT);
	digitalWrite(READ_LED_PIN, LOW);

  pinMode(READ_BUZZER_PIN, OUTPUT);
  digitalWrite(READ_LED_PIN, LOW);

	rdm6300.begin(RDM6300_RX_PIN);
}

void loop()
{
	if (rdm6300.get_new_tag_id())
  {
    //Serial.print("ID Card: ");
    Serial.print(rdm6300.get_tag_id(), HEX);
    digitalWrite(READ_BUZZER_PIN, HIGH);
    delay(50);
    digitalWrite(READ_BUZZER_PIN, LOW);
  }

	digitalWrite(READ_LED_PIN, rdm6300.get_tag_id());

	delay(10);
}