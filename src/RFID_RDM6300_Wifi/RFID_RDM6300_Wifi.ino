//Code sử dụng thư viện RDM6300 của arduino12 lấy từ Github
//và thư viện LCDI2C_Vietnamese thuộc thư viện LiquidCrystal_I2C_Multilingual của locple (cần cài vào Arduino IDE)
//Code nạp vào Wemos D1 Wifi ESP8266 kết nối với moudle RDM6300 và còi chip để thực hiện
//đọc và lấy ID thẻ RFID 125khz

#include <ESP8266WiFi.h>
#include <FS.h>
#include <LittleFS.h>
#include <LCDI2C_Vietnamese.h>
#include "rdm6300.h"

#define RDM6300_TX_PIN D9  //Chân TX của RDM6300
#define WRITE_BUZZER_PIN D10  //Chân + của còi buzzer
#define GREEN_LED_RGB D6      //Chân xanh lá của led rgb
#define RED_LED_RGB D5        //Chân đỏ của led rgb
#define BLUE_LED_RGB D7       //Chân xanh dương của led rgb

#define FILE_WIFI_SSID "/WiFiSSID.txt"          //File chứa ssid của wifi
#define FILE_WIFI_PASSWORD "/WiFiPassword.txt"  //File chứa password wifi
#define FILE_IS_USING_WIFI "/IsUsingWifi.txt"   //File chứa đang dùng wifi hay không
#define FILE_DIEM_DANH "/diemdanh.txt"

Rdm6300 rdm6300;
LCDI2C_Vietnamese lcd(0x27, 16, 2);
bool isCOMConnected = false, isFlashError = false, isUsingWifi = false, isWifiConnected = false;
String previousTextLCD = "";
int printLCDTime = 0;
bool showFirstInfo = true;

char* WIFI_SSID;
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

void printAndScrollLCD(String text) {
  if (previousTextLCD == text) return;
  previousTextLCD = text;
  lcd.setCursor(0, 0);
  lcd.clear();
  lcd.print(text);
}

void connectWifi(bool isStartup) {
  changeLedColor(255, 0, 0);
  // Đọc dữ liệu từ tệp
  WIFI_SSID = readFile(FILE_WIFI_SSID);
  const char* WIFI_PASSWORD = readFile(FILE_WIFI_PASSWORD);

  WiFi.disconnect();
  WiFi.hostname("ESP-DiemDanh");  //Fix 1 số wifi khó
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  printAndScrollLCD("Đang kết nối Wifi...");
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

  lcd.init();
  lcd.backlight();

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
    String wifiSSID = WIFI_SSID;
    if (millis() - printLCDTime >= 2000) {
      printLCDTime = millis();
      if (showFirstInfo) {
        String str_wifiSSID = WIFI_SSID;
        printAndScrollLCD("Đã kết nối Wifi:" + str_wifiSSID);
      } else {
        printAndScrollLCD("Địa chỉ IP: " + WiFi.localIP().toString());
      }
      showFirstInfo = !showFirstInfo;
    }
  } else if (isCOMConnected) {
    printAndScrollLCD("Đã kết nối phần mềm");
  } else {
    printAndScrollLCD("Chưa kết nối phần mềm/Wifi");
  }

  if (isWifiConnected) {
    client = server.available();  // Chờ client kết nối

    if (client) {
      while (client.connected()) {
        changeLedColor(0, 255, 0);
        if (millis() - printLCDTime >= 2000) printAndScrollLCD("Đã kết nối phần mềm qua Wifi");
        if (rdm6300.get_new_tag_id()) {
          client.print(rdm6300.get_tag_id(), HEX);
          client.print("\n");
          digitalWrite(WRITE_BUZZER_PIN, HIGH);
          delay(50);
          digitalWrite(WRITE_BUZZER_PIN, LOW);
          continue;
        } else if (client.available()) {
          char buffer[32];
          int i = 0;
          while (client.available()) {
            buffer[i] = client.read();
            i++;
          }
          buffer[i] = '\0';
          if (strncmp(buffer, "disconectWifi", 13) == 0) writeFile("/IsUsingWifi.txt", "0");
          else if (strncmp(buffer, "noStudentAvail", 14) == 0) {
            printAndScrollLCD("Ko nhận dạng đc thẻ s.viên");
            printLCDTime = millis();
          } else {
            printAndScrollLCD(String(buffer) + " đã điểm danh");
            printLCDTime = millis();
          }
        }
        client.print("stillConnected\n");
        delay(100);
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