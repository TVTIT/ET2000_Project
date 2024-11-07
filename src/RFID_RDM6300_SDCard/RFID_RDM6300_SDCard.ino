//Code sử dụng thư viện RDM6300 của arduino12 lấy từ Github
//Code nạp vào Arduino UNO R3 kết nối với moudle RDM6300, module Micro SD Card SPI và còi chip để thực hiện
//đọc và lấy ID thẻ RFID 125khz, đọc và ghi ID thẻ RFID vào thẻ nhớ, đọc mã thẻ RFID từ thẻ nhớ gửi lên máy tính

#include <SD.h>
#include <SPI.h>
#include <string.h>
#include "rdm6300.h"

#define RDM6300_TX_PIN 2  //Chân TX của RDM6300
//#define READ_LED_PIN 13         //Led có sẵn trên Arduino
#define WRITE_BUZZER_PIN 7      //Chân + của còi buzzer
#define GREEN_LED_RGB 6    //Chân xanh lá của led rgb
#define RED_LED_RGB 5     //Chân đỏ của led rgb
#define SD_CARD_CHIP_SELECT 10  //Chân CS của module micro sd card spi

Rdm6300 rdm6300;
File dataFile;
bool isCOMConnected;

void changeLedColor(int red, int green)
{
  analogWrite(RED_LED_RGB, 255 - red);
  analogWrite(GREEN_LED_RGB, 255 - green);
}

void setup() {
  Serial.begin(9600);

  pinMode(WRITE_BUZZER_PIN, OUTPUT);
  pinMode(GREEN_LED_RGB, OUTPUT);
  pinMode(RED_LED_RGB, OUTPUT);

  //Để led rgb sáng màu vàng
  //changeLedColor(185, 255);

  rdm6300.begin(RDM6300_TX_PIN);

  isCOMConnected = false;
}

void loop() {
  changeLedColor(!isCOMConnected * 170, 255);
  //Kiếm tra xem thẻ nhớ có đang lắp vào module không
  //nếu không thì cho còi kêu
  while (!SD.begin(SD_CARD_CHIP_SELECT)) {
    changeLedColor(255, 0);
    digitalWrite(WRITE_BUZZER_PIN, HIGH);
    delay(500);
    digitalWrite(WRITE_BUZZER_PIN, LOW);
    //delay(50);
  }
  //kiểm tra xem module rfid có nhận được thẻ mới không
  if (rdm6300.get_new_tag_id()) {
    //nếu đã kết nối cồng COM thì gửi ID thẻ qua Serial
    if (isCOMConnected) {
      Serial.print(rdm6300.get_tag_id(), HEX);
      digitalWrite(WRITE_BUZZER_PIN, HIGH);
      delay(50);
      digitalWrite(WRITE_BUZZER_PIN, LOW);
    } else {  //nếu chưa kết nối thì ghi id thẻ vào thẻ nhớ
      dataFile = SD.open("diemdanh.txt", FILE_WRITE);
      dataFile.print(rdm6300.get_tag_id(), HEX);
      dataFile.print("\n");
      dataFile.close();

      digitalWrite(WRITE_BUZZER_PIN, HIGH);
      delay(50);
      digitalWrite(WRITE_BUZZER_PIN, LOW);
    }
  } else if (Serial.available() > 0) {  //nếu nhận được lệnh từ serial
    isCOMConnected = true;              //chuyển trạng thái kết nối cổng COM
    String commandReceived = Serial.readString();
    if (commandReceived == "printTXTFile") {
      dataFile = SD.open("diemdanh.txt", FILE_READ);
      if (dataFile.size() == 0) {
        Serial.print("sdcard_empty");
      } else {
        while (dataFile.available()) {
          Serial.write(dataFile.read());
        }
      }

    } else if (commandReceived == "prepareForDisconnect") {
      SD.remove("diemdanh.txt");
      dataFile = SD.open("diemdanh.txt", FILE_WRITE);
      dataFile.close();
    } else if (commandReceived == "startup") {

    }
  }
  //Bỏ chân 13 vì module thẻ nhớ đã dùng chân 13 rồi
  //digitalWrite(READ_LED_PIN, rdm6300.get_tag_id());
  delay(10);
}