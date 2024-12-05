//Code sử dụng thư viện RDM6300 của arduino12 lấy từ Github
//Code nạp vào Arduino UNO R3 kết nối với moudle RDM6300, module Micro SD Card SPI và còi chip để thực hiện
//đọc và lấy ID thẻ RFID 125khz, đọc và ghi ID thẻ RFID vào thẻ nhớ, đọc mã thẻ RFID từ thẻ nhớ gửi lên máy tính

#include <SdFat.h>
#include <LiquidCrystal_I2C.h>
#include <string.h>
#include "rdm6300.h"

#define RDM6300_TX_PIN 2  //Chân TX của RDM6300
//#define READ_LED_PIN 13         //Led có sẵn trên Arduino
#define WRITE_BUZZER_PIN 7      //Chân + của còi buzzer
#define GREEN_LED_RGB 6         //Chân xanh lá của led rgb
#define RED_LED_RGB 5           //Chân đỏ của led rgb
#define SD_CARD_CHIP_SELECT 10  //Chân CS của module micro sd card spi

// Try max SPI clock for an SD. Reduce SPI_CLOCK if errors occur.
#define SPI_CLOCK SD_SCK_MHZ(50)

// Try to select the best SD card configuration.
#if HAS_SDIO_CLASS
#define SD_CONFIG SdioConfig(FIFO_SDIO)
#elif ENABLE_DEDICATED_SPI
#define SD_CONFIG SdSpiConfig(SD_CARD_CHIP_SELECT, DEDICATED_SPI, SPI_CLOCK)
#else  // HAS_SDIO_CLASS
#define SD_CONFIG SdSpiConfig(SD_CARD_CHIP_SELECT, SHARED_SPI, SPI_CLOCK)
#endif  // HAS_SDIO_CLASS

Rdm6300 rdm6300;
SdFat32 SD;
File dataFile;
bool isCOMConnected, isCSVFileExist;
LiquidCrystal_I2C lcd(0x27, 16, 2);
String previousTextLCD = "";
uint8_t CSVRowCount = 0;
uint32_t printLCDTime = 0;

void changeLedColor(int red, int green) {
  analogWrite(RED_LED_RGB, 255 - red);
  analogWrite(GREEN_LED_RGB, 255 - green);
}

void readTextSerial(char* result, int maxLength) {
  int i = 0;
  while (Serial.available() && i < maxLength - 1) {
    result[i] = Serial.read();
    i++;
    delay(2);
  }
  result[i] = '\0';
}

//Kiếm tra xem thẻ nhớ có đang lắp vào module không
//nếu không thì cho còi kêu
void checkSDAvail() {
  while (!SD.begin(SD_CONFIG)) {
    changeLedColor(255, 0);
    digitalWrite(WRITE_BUZZER_PIN, HIGH);
    delay(500);
    digitalWrite(WRITE_BUZZER_PIN, LOW);
  }
}

void printAndScrollLCD(String text) {
  if (previousTextLCD == text) return;
  previousTextLCD = text;
  lcd.setCursor(0, 0);
  lcd.clear();
  if (text.length() > 16) {
    int index = 15;
    for (; index >= 0; index--) {
      if (text[index] == ' ') break;
    }
    lcd.print(text.substring(0, index));
    lcd.setCursor(0, 1);
    lcd.print(text.substring(index + 1));
  } else {
    lcd.print(text);
  }
}

void checkIDCard(const char* idCard) {
  if (SD.exists("stulist.csv")) {
    dataFile.open("stulist.csv", FILE_READ);
  }

  bool isCardAvail = false;
  while (dataFile.available()) {
    char line[32];
    dataFile.fgets(line, 32);

    char* token = strtok(line, ",");

    if (strncmp(token, idCard, strlen(idCard)) == 0) {
      token = strtok(nullptr, "\n");
      printAndScrollLCD(String(token) + " da diem danh");
      printLCDTime = millis();
      isCardAvail = true;
      break;
    }
  }
  if (!isCardAvail) {
    printAndScrollLCD("Ko nhan dang dc the s.vien");
    printLCDTime = millis();
  }
}

void setup() {
  Serial.begin(9600);

  pinMode(WRITE_BUZZER_PIN, OUTPUT);
  pinMode(GREEN_LED_RGB, OUTPUT);
  pinMode(RED_LED_RGB, OUTPUT);

  pinMode(A0, OUTPUT);

  rdm6300.begin(RDM6300_TX_PIN);

  lcd.init();
  lcd.backlight();

  isCOMConnected = false;

  checkSDAvail();

  isCSVFileExist = SD.exists("stulist.csv");
}

void loop() {
  changeLedColor(!isCOMConnected * 170, 255);

  checkSDAvail();

  if (isCOMConnected) {
    printAndScrollLCD("Da ket noi phan mem");
  } else if (millis() - printLCDTime >= 2000) {
    printAndScrollLCD("Chua ket noi phan mem");
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
      String ID = String(rdm6300.get_tag_id(), HEX);
      ID.toUpperCase();
      dataFile = SD.open("diemdanh.txt", FILE_WRITE);
      dataFile.print(ID + "\n");
      dataFile.close();

      digitalWrite(WRITE_BUZZER_PIN, HIGH);
      delay(50);
      digitalWrite(WRITE_BUZZER_PIN, LOW);

      if (isCSVFileExist) {
        printAndScrollLCD("Dang doc the...");
        checkIDCard(ID.c_str());
      }
    }
  } else if (Serial.available() > 0) {  //nếu nhận được lệnh từ serial
    isCOMConnected = true;              //chuyển trạng thái kết nối cổng COM
    char commandReceived[32];
    readTextSerial(commandReceived, 32);
    if (strncmp(commandReceived, "printTXTFile", 12) == 0) {
      dataFile = SD.open("diemdanh.txt", FILE_READ);
      if (dataFile.size() == 0) {
        Serial.print("sdcard_empty");
      } else {
        while (dataFile.available()) {
          Serial.write(dataFile.read());
        }
      }

    } else if (strncmp(commandReceived, "prepareForDisconnect", 20) == 0) {
      SD.remove("diemdanh.txt");
      dataFile = SD.open("diemdanh.txt", FILE_WRITE);
      dataFile.close();
      if (isCSVFileExist) SD.remove("stulist.csv");
      dataFile = SD.open("stulist.csv", FILE_WRITE);
      while (!Serial.available()) {
        delay(1);
      }
      while (Serial.available()) {
        dataFile.write(Serial.read());
        delay(1);
      }
      dataFile.close();
      Serial.print("doneRead");
    }
  }
  //Bỏ chân 13 vì module thẻ nhớ đã dùng chân 13 rồi
  //digitalWrite(READ_LED_PIN, rdm6300.get_tag_id());
  delay(10);
}