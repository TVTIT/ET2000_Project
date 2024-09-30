//Code sử dụng thư viện RDM6300 của arduino12 lấy từ Github
//Code nạp vào Arduino UNO R3 kết nối với moudle RDM6300 và còi chip để thực hiện
//đọc và lấy ID thẻ RFID 125khz

#include "rdm6300.h"

#define RDM6300_RX_PIN 2 //Chân RX của RDM6300
#define READ_LED_PIN 13 //Led có sẵn trên Arduino
#define WRITE_BUZZER_PIN 7 //Chân + của còi buzzer

Rdm6300 rdm6300;

void setup()
{
	Serial.begin(9600);

	pinMode(READ_LED_PIN, OUTPUT);
	digitalWrite(READ_LED_PIN, LOW);

  pinMode(WRITE_BUZZER_PIN, OUTPUT);
  digitalWrite(READ_LED_PIN, LOW);

	rdm6300.begin(RDM6300_RX_PIN);
}

void loop()
{
	if (rdm6300.get_new_tag_id())
  {
    Serial.print(rdm6300.get_tag_id(), HEX);
    digitalWrite(WRITE_BUZZER_PIN, HIGH);
    delay(50);
    digitalWrite(WRITE_BUZZER_PIN, LOW);
  }

	digitalWrite(READ_LED_PIN, rdm6300.get_tag_id());

	delay(10);
}