#include <SPI.h>
#include <MFRC522.h>
#include "WiFi.h"
#include "ESPAsyncWebServer.h"
#include "SPIFFS.h"

#define SS_PIN 5
#define RST_PIN 14

MFRC522 rfid(SS_PIN, RST_PIN); // Instance of the class

MFRC522::MIFARE_Key key;

// Init array that will store new NUID
byte nuidPICC[4] = {0xB7, 0x66, 0x66, 0x43};

// 自分の環境によって置き換え
const char* ssid = "TABEBOX";
const char* password = "tabeboxme310";
const IPAddress ip(192,168,0,1);
const IPAddress subnet(255,255,255,0);

// GPIOの２番ピンをkeyPinとして設定
const int keyPin = 2;
// KEYの状態を保持する変数
String keyState;

// ポート80にサーバーを設定
AsyncWebServer server(80);

// 実際のピン出力によってhtmlファイル内のSTATEの文字を変える
String processor(const String& var) {
  Serial.println(var);
  if (var == "STATE") {
    if (digitalRead(keyPin)) {
      keyState = "LOCKED";
    }
    else {
      keyState = "UNLOCKED";
    }
    Serial.print(keyState);
    return keyState;
  }
  return String();
}

void setup() {
  Serial.begin(115200);
  pinMode(keyPin, OUTPUT); //GPIO02をアウトプットに
  SPI.begin(); // Init SPI bus
  rfid.PCD_Init(); // Init MFRC522

  for (byte i = 0; i < 6; i++) {
    key.keyByte[i] = 0xFF;
  }

  Serial.println(F("This code scan the MIFARE Classsic NUID."));
  Serial.print(F("Using the following key:"));
  // SPIFFSのセットアップ
  if (!SPIFFS.begin(true)) {
    Serial.println("An Error has occurred while mounting SPIFFS");
    return;
  }

  WiFi.softAP(ssid,password);
  delay(100);
  WiFi.softAPConfig(ip,ip,subnet);
  IPAddress serverIP = WiFi.softAPIP();

  // サーバーのルートディレクトリにアクセスされた時のレスポンス
  server.on("/", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send(SPIFFS, "/index.html", String(), false, processor);
  });

  // style.cssにアクセスされた時のレスポンス
  server.on("/style.css", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send(SPIFFS, "/style.css", "text/css");
  });

  // Onボタンが押された時のレスポンス
  server.on("/on", HTTP_GET, [](AsyncWebServerRequest * request) {
    digitalWrite(keyPin, HIGH);
    request->send(SPIFFS, "/index.html", String(), false, processor);
  });

  // Offボタンが押された時のレスポンス
  server.on("/off", HTTP_GET, [](AsyncWebServerRequest * request) {
    digitalWrite(keyPin, LOW);
    request->send(SPIFFS, "/index.html", String(), false, processor);
  });

  // サーバースタート
  server.begin();
}

void loop() {
   // Reset the loop if no new card present on the sensor/reader. This saves the entire process when idle.
  if ( ! rfid.PICC_IsNewCardPresent())
    return;

  // Verify if the NUID has been readed
  if ( ! rfid.PICC_ReadCardSerial())
    return;

  Serial.print(F("PICC type: "));
  MFRC522::PICC_Type piccType = rfid.PICC_GetType(rfid.uid.sak);
  Serial.println(rfid.PICC_GetTypeName(piccType));

  // Check is the PICC of Classic MIFARE type
  if (piccType != MFRC522::PICC_TYPE_MIFARE_MINI &&  
    piccType != MFRC522::PICC_TYPE_MIFARE_1K &&
    piccType != MFRC522::PICC_TYPE_MIFARE_4K) {
    Serial.println(F("Your tag is not of type MIFARE Classic."));
    return;
  }

  if (rfid.uid.uidByte[0] == nuidPICC[0] || 
    rfid.uid.uidByte[1] == nuidPICC[1] || 
    rfid.uid.uidByte[2] == nuidPICC[2] || 
    rfid.uid.uidByte[3] == nuidPICC[3] ) {
    Serial.println(F("KEY UNLOCKED"));
    digitalWrite(keyPin, LOW);
  }
  else{
    digitalWrite(keyPin, HIGH);
    Serial.println(F("KEY DENIED"));
  }
  

  // Halt PICC
  rfid.PICC_HaltA();

  // Stop encryption on PCD
  rfid.PCD_StopCrypto1();
}
