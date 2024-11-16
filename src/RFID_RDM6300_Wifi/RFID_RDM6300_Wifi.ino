//Code sử dụng thư viện RDM6300 của arduino12 lấy từ Github
//Code nạp vào Arduino UNO R3 kết nối với moudle RDM6300 và còi chip để thực hiện
//đọc và lấy ID thẻ RFID 125khz

#include <ESP8266WiFi.h>
#include <FS.h>
#include <LittleFS.h>
#include "rdm6300.h"

#define RDM6300_TX_PIN 2  //Chân TX của RDM6300
//#define READ_LED_PIN 13         //Led có sẵn trên Arduino
#define WRITE_BUZZER_PIN 15  //Chân + của còi buzzer
#define GREEN_LED_RGB 5      //Chân xanh lá của led rgb
#define RED_LED_RGB 16       //Chân đỏ của led rgb
#define BLUE_LED_RGB 4       //Chân xanh dương của led rgb

#define FILE_WIFI_SSID "/WiFiSSID.txt"          //File chứa ssid của wifi
#define FILE_WIFI_PASSWORD "/WiFiPassword.txt"  //File chứa password wifi
#define FILE_IS_USING_WIFI "/IsUsingWifi.txt"   //File chứa đang dùng wifi hay không
#define FILE_DIEM_DANH "/diemdanh.txt"

Rdm6300 rdm6300;
bool isCOMConnected = false, isFlashError = false, isUsingWifi = false, isWifiConnected = false;

// char* WIFI_SSID;
// char* WIFI_PASSWORD;

WiFiServer server(1234);  // Tạo server trên cổng 1234
WiFiClient client;

void changeLedColor(int red, int green, int blue) {
  analogWrite(RED_LED_RGB, 255 - red);
  analogWrite(GREEN_LED_RGB, 255 - green);
  analogWrite(BLUE_LED_RGB, 255 - blue);
}

void writeFile(const char* filePath, const char* text) {
  File file = LittleFS.open(filePath, "w");
  if (!file) {
    changeLedColor(255, 0, 0);
    isFlashError = true;
    return;
  }
  file.print(text);
  file.close();
}

char* readFile(const char* filePath) {
  File file = LittleFS.open(filePath, "r");
  if (!file) {
    changeLedColor(255, 0, 0);
    Serial.println("error");
    isFlashError = true;
    return NULL;
  }
  int fileSize = file.size();
  char* text = new char[fileSize + 1];
  file.readBytes(text, fileSize);
  text[fileSize] = '\0';
  file.close();
  return text;
}

void connectWifi(bool isStartup) {
  changeLedColor(255, 0, 0);
  // Đọc dữ liệu từ tệp
  const char* WIFI_SSID = readFile(FILE_WIFI_SSID);
  const char* WIFI_PASSWORD = readFile(FILE_WIFI_PASSWORD);

  WiFi.disconnect();
  WiFi.hostname("ESP-DiemDanh");  //Fix 1 số wifi khó
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  int startAttemptTime = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < 10000) {
    delay(100);
  }
  if (WiFi.status() != WL_CONNECTED) {
    Serial.print("wifiError");
    writeFile(FILE_IS_USING_WIFI, "0");
    isWifiConnected = false;
    return;
  }

  if (!isStartup)
    Serial.print(WiFi.localIP());

  server.begin();  // Bắt đầu server

  isWifiConnected = true;
}

void setup() {
  Serial.begin(9600);

  pinMode(WRITE_BUZZER_PIN, OUTPUT);
  pinMode(GREEN_LED_RGB, OUTPUT);
  pinMode(RED_LED_RGB, OUTPUT);
  pinMode(BLUE_LED_RGB, OUTPUT);

  rdm6300.begin(RDM6300_TX_PIN);

  if (!LittleFS.begin()) {
    changeLedColor(255, 0, 0);
    isFlashError = true;
    return;
  }

  char* c_isUsingWifi = readFile("/IsUsingWifi.txt");

  isUsingWifi = (c_isUsingWifi[0] == '1');

  if (isUsingWifi) {
    connectWifi(true);
  }
}

void loop() {
  while (isFlashError) {
    delay(10000);
  }

  //blue: đã kết nối wifi nhưng đang đợi
  //vàng = red + green: chưa kết nối COM
  //tím = red + blue: Đã kết nối COM
  changeLedColor(!isWifiConnected * 170, !(isWifiConnected || isCOMConnected) * 255, (isWifiConnected || isCOMConnected) * 255);

  if (isWifiConnected) {
    client = server.available();  // Chờ client kết nối

    if (client) {
      changeLedColor(0, 255, 0);
      while (client.connected()) {
        if (rdm6300.get_new_tag_id()) {
          client.print(rdm6300.get_tag_id(), HEX);
          client.print("\n");
          digitalWrite(WRITE_BUZZER_PIN, HIGH);
          delay(50);
          digitalWrite(WRITE_BUZZER_PIN, LOW);
        } else if (client.available()) {
          if (client.readStringUntil('\n') == "disconectWifi") writeFile("/IsUsingWifi.txt", "0");
        } 
        else {
          client.print("stillConnected\n");
          delay(100);
        }
      }
      changeLedColor(!isWifiConnected * 170, !isWifiConnected * 255, isWifiConnected * 255);
    }
  }

  //kiểm tra xem module rfid có nhận được thẻ mới không
  if (rdm6300.get_new_tag_id()) {
    if (isCOMConnected) {
      Serial.print(rdm6300.get_tag_id(), HEX);
    } else {
      String ID = String(rdm6300.get_tag_id(), HEX);
      ID.toUpperCase();
      File fileDiemDanh = LittleFS.open(FILE_DIEM_DANH, "a");
      fileDiemDanh.print(ID + "\n");
      fileDiemDanh.close();
    }
    digitalWrite(WRITE_BUZZER_PIN, HIGH);
    delay(50);
    digitalWrite(WRITE_BUZZER_PIN, LOW);
  } else if (Serial.available() > 0) {  //nếu nhận được lệnh từ serial
    isCOMConnected = true;              //chuyển trạng thái kết nối cổng COM
    String commandReceived = Serial.readString();
    if (commandReceived == "connectWifi") {
      while (Serial.available() <= 0) {
        delay(10);
      }
      writeFile(FILE_WIFI_SSID, Serial.readString().c_str());

      while (Serial.available() <= 0) {
        delay(10);
      }
      String password = Serial.readString();
      if (password == "null") {
        password = "";
      }
      writeFile(FILE_WIFI_PASSWORD, password.c_str());

      writeFile(FILE_IS_USING_WIFI, "1");

      connectWifi(false);
    } else if (commandReceived == "printTXTFile") {
      File fileDiemDanh = LittleFS.open(FILE_DIEM_DANH, "r");
      if (fileDiemDanh.size() == 0) {
        Serial.print("sdcard_empty");
      } else {
        while (fileDiemDanh.available()) {
          Serial.write(fileDiemDanh.read());
        }
      }
      fileDiemDanh.close();
    } else if (commandReceived == "prepareForDisconnect") {
      writeFile(FILE_DIEM_DANH, "");
    }
  }
  delay(10);
}