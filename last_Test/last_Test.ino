#include <WiFi.h>
#include <HTTPClient.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Keypad.h>
#include <Adafruit_Fingerprint.h>
#include <ArduinoJson.h>

// WiFi Settings
const char* ssid = ".";  // Badilisha na SSID yako
const char* password = "11223344555"; // Badilisha na password yako

// Server Address
String serverName = "http://192.168.75.2/biometric_api/";

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
String enteredFingerprintID = "";
bool waitingForFingerprintID = false;
bool waitingToEnrollFingerprint = false;

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
    while (1);
  }
  delay(1500);
  lcd.clear();
}

void loop() {
  if (!waitingForFingerprintID && !waitingToEnrollFingerprint) {
    lcd.setCursor(0, 0);
    lcd.print("MembershipNo:   ");
    lcd.setCursor(0, 1);
    lcd.print(enteredID + "          ");
  } else if (waitingForFingerprintID) {
    lcd.setCursor(0, 0);
    lcd.print("Enter FingerID: ");
    lcd.setCursor(0, 1);
    lcd.print(enteredFingerprintID + "          ");
  }

  char key = keypad.getKey();
  if (key) {
    if (key == '#') {
      if (!waitingForFingerprintID && !waitingToEnrollFingerprint) {
        lcd.clear();
        lcd.print("Checking...");
        checkMember(enteredID);
        enteredID = "";
      } else if (waitingForFingerprintID) {
        fingerprintID = enteredFingerprintID.toInt();
        Serial.print("User entered Fingerprint ID: ");
        Serial.println(fingerprintID);
        waitingForFingerprintID = false;
        waitingToEnrollFingerprint = true;
        lcd.clear();
        lcd.print("Wait...");
      }
    } else if (key == '*') {
      if (!waitingForFingerprintID) {
        enteredID = "";
      } else {
        enteredFingerprintID = "";
      }
      lcd.setCursor(0, 1);
      lcd.print("                ");
    } else {
      if (!waitingForFingerprintID && enteredID.length() < 10) {
        enteredID += key;
        lcd.setCursor(0, 1);
        lcd.print(enteredID);
      } else if (waitingForFingerprintID && enteredFingerprintID.length() < 3) {
        enteredFingerprintID += key;
        lcd.setCursor(0, 1);
        lcd.print(enteredFingerprintID);
      }
    }
  }

  if (waitingToEnrollFingerprint) {
    enrollFingerprint(fingerprintID);
    waitingToEnrollFingerprint = false;
  }

  // Check for fingerprint match only if not in enrollment process
  if (!waitingToEnrollFingerprint) {
    int getImageResult = finger.getImage();
    if (getImageResult == FINGERPRINT_OK) {
      int image2TzResult = finger.image2Tz();
      if (image2TzResult == FINGERPRINT_OK) {
        int searchResult = finger.fingerFastSearch();
        if (searchResult == FINGERPRINT_OK) {
          int foundID = finger.fingerID;
          lcd.clear();
          lcd.print("Matched ID:");
          lcd.setCursor(0, 1);
          lcd.print(foundID);
          delay(2000);
          getMemberByFingerprint(foundID);
        }
      }
    }
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
    DeserializationError error = deserializeJson(doc, response);
    if (error) {
      Serial.print("JSON Error: ");
      Serial.println(error.c_str());
      lcd.clear();
      lcd.print("JSON Error");
      delay(2000);
      return;
    }

    memberName = doc["memberName"].as<String>();
    memberNo = doc["membershipNo"].as<String>();

    waitingForFingerprintID = true;
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
  Serial.println("Sending payload: " + payload);
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

void showMemberInfoUntilNext(String line1, String line2) {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(line1.substring(0, 16)); // 16 chars max for LCD line
  lcd.setCursor(0, 1);
  lcd.print(line2.substring(0, 16)); // 16 chars max for LCD line

  while (true) {
    char key = keypad.getKey();
    if (key == '#') {
      lcd.clear();
      break;
    }
    delay(100);
  }
}

void getMemberByFingerprint(int id) {
  HTTPClient http;
  http.begin(serverName + "get_member_by_fingerprint.php?fingerprintID=" + String(id));
  int httpCode = http.GET();

  if (httpCode == 200) {
    String response = http.getString();
    Serial.println("Response: " + response);

    if (response.indexOf("not_found") != -1) {
      lcd.clear();
      lcd.print("Unknown Finger");
      delay(2000);
      return;
    }

    StaticJsonDocument<512> doc;
    DeserializationError error = deserializeJson(doc, response);
    if (error) {
      Serial.print("JSON Error: ");
      Serial.println(error.c_str());
      lcd.clear();
      lcd.print("JSON Error");
      delay(2000);
      return;
    }

    String memberName = doc["memberName"].as<String>();
    String address = doc["address"].as<String>();
    String membershipNo = doc["membershipNo"].as<String>();

    Serial.println("Member Name: " + memberName);
    Serial.println("Membership No: " + membershipNo);
    Serial.println("Address: " + address);

    // Show info until '#' pressed
    showMemberInfoUntilNext(memberName, address);

  } else {
    lcd.clear();
    lcd.print("HTTP Err:");
    lcd.print(httpCode);
    delay(2000);
  }
  http.end();
}
