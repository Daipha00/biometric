#include <WiFi.h>
#include <HTTPClient.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Keypad.h>
#include <Adafruit_Fingerprint.h>
#include <ArduinoJson.h>

// WiFi Settings
const char* ssid = "Galaxy Tab A78565";
const char* password = "1234567890";

// Server Address (replace with your computer's IP)
String serverName = "http://192.168.255.2/biometric_api/";

// LCD
LiquidCrystal_I2C lcd(0x27, 16, 2);

// Keypad Setup
const byte ROWS = 4;
const byte COLS = 3;
char keys[ROWS][COLS] = {
  {'1','2','3'},
  {'4','5','6'},
  {'7','8','9'},
  {'*','0','#'}
};
byte rowPins[ROWS] = {13, 25, 27, 19};
byte colPins[COLS] = {23, 32, 5};
Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);
String enteredID = "";

// Fingerprint Sensor
HardwareSerial mySerial(2); // GPIO16 (RX), GPIO17 (TX)
Adafruit_Fingerprint finger = Adafruit_Fingerprint(&mySerial);

// Global Variables
String memberName = "";
String memberNo = "";
int fingerprintID = 0;

void setup() {
  Serial.begin(115200);
  mySerial.begin(57600, SERIAL_8N1, 16, 17);
  finger.begin(57600);

  lcd.init();
  lcd.backlight();
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Connecting...");

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    lcd.print(".");
  }

  lcd.clear();
  lcd.print("WiFi connected");
  delay(1000);
  lcd.clear();

  if (finger.verifyPassword()) {
    lcd.setCursor(0, 0);
    lcd.print("Sensor Ready");
  } else {
    lcd.setCursor(0, 0);
    lcd.print("Sensor Error");
    while (1); // stop everything
  }

  delay(1500);
  lcd.clear();
}

void loop() {
  lcd.setCursor(0, 0);
  lcd.print("MembershipNo:   ");
  lcd.setCursor(0, 1);
  lcd.print(enteredID + "          ");

  char key = keypad.getKey();
  if (key) {
    if (key == '#') {
      lcd.clear();
      lcd.print("Checking...");
      checkMember(enteredID);
      enteredID = "";
    } else if (key == '*') {
      enteredID = "";
      lcd.setCursor(0, 1);
      lcd.print("                ");
    } else if (enteredID.length() < 10) {
      enteredID += key;
      lcd.setCursor(0, 1);
      lcd.print(enteredID);
    }
  }

  // Optional: check if fingerprint is scanned
  if (finger.getImage() == FINGERPRINT_OK &&
      finger.image2Tz() == FINGERPRINT_OK &&
      finger.fingerFastSearch() == FINGERPRINT_OK) {
    int foundID = finger.fingerID;
    lcd.clear();
    lcd.print("Matched ID:");
    lcd.setCursor(0, 1);
    lcd.print(foundID);
    delay(2000);
    getMemberByFingerprint(foundID);
  }
}

void checkMember(String membershipNo) {
  HTTPClient http;
  http.begin(serverName + "check_member.php?membershipNo=" + membershipNo);
  int httpCode = http.GET();
  if (httpCode == 200) {
    String response = http.getString();
    Serial.println(response);

    if (response.indexOf("not_found") != -1) {
      lcd.clear();
      lcd.print("Not Found");
      delay(2000);
      return;
    }

    StaticJsonDocument<512> doc;
    deserializeJson(doc, response);

    memberName = doc["memberName"].as<String>();
    fingerprintID = doc["fingerprintID"] | 0;
    memberNo = doc["membershipNo"].as<String>();

    if (doc["isFingerprintRegistered"] == true) {
      lcd.clear();
      lcd.print("Already Reg.");
      delay(2000);
      return;
    }

    enrollFingerprint(fingerprintID);
  } else {
    lcd.clear();
    lcd.print("Error: " + String(httpCode));
    delay(2000);
  }
  http.end();
}

void enrollFingerprint(int id) {
  lcd.clear();
  lcd.print("Place Finger");

  while (finger.getImage() != FINGERPRINT_OK);
  if (finger.image2Tz(1) != FINGERPRINT_OK) return;

  lcd.clear();
  lcd.print("Again...");
  delay(1500);

  while (finger.getImage() != FINGERPRINT_OK);
  if (finger.image2Tz(2) != FINGERPRINT_OK) return;

  if (finger.createModel() != FINGERPRINT_OK) return;

  if (finger.storeModel(id) == FINGERPRINT_OK) {
    lcd.clear();
    lcd.print("Saved ID: ");
    lcd.print(id);
    updateFingerprint(memberNo, id);
    delay(2000);
  } else {
    lcd.clear();
    lcd.print("Save Failed");
    delay(2000);
  }
}

void updateFingerprint(String membershipNo, int fpID) {
  HTTPClient http;
  http.begin(serverName + "register_fingerprint.php");
  http.addHeader("Content-Type", "application/json");

  String payload = "{\"membershipNo\":\"" + membershipNo + "\",\"fingerprintID\":" + String(fpID) + "}";
  int httpResponseCode = http.POST(payload);

  if (httpResponseCode == 200) {
    lcd.clear();
    lcd.print("Reg Success");
  } else {
    lcd.clear();
    lcd.print("Reg Failed");
  }

  delay(2000);
  http.end();
}

void getMemberByFingerprint(int id) {
  HTTPClient http;
  http.begin(serverName + "get_member_by_fingerprint.php?fingerprintID=" + String(id));
  int httpCode = http.GET();
  if (httpCode == 200) {
    String response = http.getString();
    StaticJsonDocument<512> doc;
    deserializeJson(doc, response);

    if (response.indexOf("not_found") != -1) {
      lcd.clear();
      lcd.print("Unknown Finger");
      delay(2000);
      return;
    }

    lcd.clear();
    lcd.print(doc["memberName"].as<String>());
    lcd.setCursor(0, 1);
    lcd.print(doc["address"].as<String>());
    delay(3000);
  }
  http.end();
}
