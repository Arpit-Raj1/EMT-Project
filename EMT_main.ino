#include <WiFi.h>
#include <HTTPClient.h>
#include <SPI.h>
#include <MFRC522.h>

// ======= WiFi Credentials =======
const char* ssid = "Dhirendra";
const char* password = "dpsingh488";  // leave empty if open network

// ======= Google Script URLs =======
const char* entryUrl = "ENTRY_URL"; // Reader 1 Sheet
const char* exitUrl  = "EXIT_URL";  // Reader 2 Sheet

// ======= SPI and RC522 Pins =======
#define PIN_SCK  18
#define PIN_MOSI 23
#define PIN_MISO 19

#define SS_PIN_1 5
#define RST_PIN_1 21

#define SS_PIN_2 4
#define RST_PIN_2 22

// ======= Buzzer Pin =======
#define ENTRY_BUZZER_PIN 2
#define EXIT_BUZZER_PIN 15
#define BUZZER_DURATION 250

MFRC522 mfrc522_1(SS_PIN_1, RST_PIN_1);
MFRC522 mfrc522_2(SS_PIN_2, RST_PIN_2);

SPISettings rfidSPISettings(10000000, MSBFIRST, SPI_MODE0);

// ======= Setup =======
void setup() {
  Serial.begin(115200);
  delay(200);

  pinMode(ENTRY_BUZZER_PIN, OUTPUT);
  pinMode(EXIT_BUZZER_PIN, OUTPUT);
  digitalWrite(ENTRY_BUZZER_PIN, LOW);
  digitalWrite(EXIT_BUZZER_PIN, LOW);

  // --- Connect to WiFi ---
  Serial.print("Connecting to WiFi ...");
  WiFi.begin(ssid, password);
  int retries = 0;
  while (WiFi.status() != WL_CONNECTED && retries < 30) {
    delay(500);
    Serial.print(".");
    retries++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi connected!");
    Serial.print("IP: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("\nWiFi connection failed, continuing offline...");
  }

  // --- Initialize SPI and RC522 readers ---
  SPI.begin(PIN_SCK, PIN_MISO, PIN_MOSI);
  delay(100);

  mfrc522_1.PCD_Init();
  delay(100);
  mfrc522_2.PCD_Init();
  delay(100);

  // --- Verify readers ---
  Serial.println("Checking RFID readers...");
  byte v1 = mfrc522_1.PCD_ReadRegister(mfrc522_1.VersionReg);
  byte v2 = mfrc522_2.PCD_ReadRegister(mfrc522_2.VersionReg);

  if (v1 == 0x00 || v1 == 0xFF) {
    Serial.println("Reader 1 not detected!");
  } else {
    Serial.print("Reader 1 ready (Version: 0x");
    Serial.print(v1, HEX);
    Serial.println(")");
  }

  if (v2 == 0x00 || v2 == 0xFF) {
    Serial.println("Reader 2 not detected!");
  } else {
    Serial.print("Reader 2 ready (Version: 0x");
    Serial.print(v2, HEX);
    Serial.println(")");
  }

  Serial.println("Place card near Reader 1 or Reader 2...");
}

// ======= Reader Check Function =======
void readRFID(MFRC522 &rfid, const char *readerName, const char *url) {
  if (!rfid.PICC_IsNewCardPresent() || !rfid.PICC_ReadCardSerial()) return;

  if (url == entryUrl)
  {
    beepBuzzer(BUZZER_DURATION, ENTRY_BUZZER_PIN);
    beepBuzzer(BUZZER_DURATION, ENTRY_BUZZER_PIN);
  }
  else if (url == exitUrl)
  {
    beepBuzzer(BUZZER_DURATION, EXIT_BUZZER_PIN);
    beepBuzzer(BUZZER_DURATION, EXIT_BUZZER_PIN);
  }

  Serial.print(readerName);
  String res = "";
  Serial.print(" UID:");
  for (byte i = 0; i < rfid.uid.size; i++) {
    Serial.print(rfid.uid.uidByte[i] < 0x10 ? " 0" : " ");
    res+=rfid.uid.uidByte[i] < 0x10 ? " 0" : " ";
    Serial.print(rfid.uid.uidByte[i], HEX);
    res+=String(rfid.uid.uidByte[i], HEX);
  }
  Serial.println();
  MFRC522::PICC_Type piccType = rfid.PICC_GetType(rfid.uid.sak);
  Serial.print(readerName);
  Serial.print(" Type: ");
  Serial.println(rfid.PICC_GetTypeName(piccType));
  rfid.PICC_HaltA();
  rfid.PCD_StopCrypto1();

  res.toUpperCase();
  sendToGoogle(url, res);

}

// ======= Main Loop =======
void loop() {
  readRFID(mfrc522_1, "Reader 1", entryUrl);
  readRFID(mfrc522_2, "Reader 2", exitUrl);
  delay(100);
}

// ======= Convert UID to String =======
String getUIDString(MFRC522::Uid &uid) {
  String result = "";
  for (byte i = 0; i < uid.size; i++) {
    if (uid.uidByte[i] < 0x10) result += "0";
    result += String(uid.uidByte[i], HEX);
  }
  result.toUpperCase();
  return result;
}

// ======= Send UID to Google Script =======
void sendToGoogle(const char* url, String uid) {
  uid.replace(" ", "");
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    String fullURL = String(url) + "?uid=" + uid;
    Serial.print("Sending request to: ");
    Serial.println(fullURL);

    http.begin(fullURL);
    int httpCode = http.GET();

    if (httpCode > 0) {
      Serial.printf("HTTP Response code: %d\n", httpCode);
      String payload = http.getString();
      Serial.println("Response: " + payload);
    } else {
      Serial.printf("HTTP request failed. Error: %s\n", http.errorToString(httpCode).c_str());
    }

    http.end();
  } else {
    Serial.println("WiFi not connected.");
  }
}


// ======= Buzzer Function =======
void beepBuzzer(int duration, int BUZZER_PIN) {
  digitalWrite(BUZZER_PIN, HIGH);
  delay(duration);
  digitalWrite(BUZZER_PIN, LOW);
  delay(duration);
}
