// Include necessary libraries
#include <WiFi.h>
#include <HTTPClient.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <ArduinoJson.h>
#include <Keypad.h>
#include <Adafruit_Fingerprint.h>

// WiFi credentials
const char* ssid = ".";
const char* password = "11223344555";
String serverName = "http://192.168.75.2/biometric_api/"; // Replace with your actual server IP

// LCD settings
LiquidCrystal_I2C lcd(0x27, 16, 2);

// Fingerprint sensor setup
HardwareSerial mySerial(2);
Adafruit_Fingerprint finger = Adafruit_Fingerprint(&mySerial);

// Keypad setup
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

// Global variables
String enteredID = "";
String enteredFingerprintID = "";
String memberNo = "";
bool waitingForFingerprintID = false;
bool isChecking = false;
bool systemIdle = true;

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
  delay(1500);
  lcd.clear();

  if (!finger.verifyPassword()) {
    lcd.print("FP Error!");
    while (1);
  }

  lcd.print("Karibu!");
  delay(2000);
  lcd.clear();
  lcd.print("Weka kidole...");
}

void loop() {
  // Check for fingerprint match
  if (finger.getImage() == FINGERPRINT_OK &&
      finger.image2Tz() == FINGERPRINT_OK &&
      finger.fingerFastSearch() == FINGERPRINT_OK) {
    lcd.clear();
    lcd.print("Inachakata...");
    int foundID = finger.fingerID;
    delay(1000);
    lcd.clear();
    lcd.print("Ondoa kidole");
    showMemberInfo(foundID);
    delay(2000);
    lcd.clear();
    lcd.print("Karibu!");
    return;
  }

  // Handle keypad input
  char key = keypad.getKey();
  if (key) {
    if (waitingForFingerprintID) {
      if (key == '#') {
        if (enteredFingerprintID.length() > 0) {
          int fpID = enteredFingerprintID.toInt();

          if (isFingerprintIDUsed(fpID) || hasFPModel(fpID)) {
            lcd.clear();
            lcd.print("FP ID Tayari");
            delay(2000);
            enteredFingerprintID = "";
            waitingForFingerprintID = false;
            lcd.clear();
            lcd.print("Karibu!");
            return;
          }

          lcd.clear();
          lcd.print("Weka Kidole 1");
          enrollWithVerification(fpID);
          enteredFingerprintID = "";
          waitingForFingerprintID = false;
          lcd.clear();
          lcd.print("Karibu!");
        }
      } else if (key == '*') {
        enteredFingerprintID = "";
        lcd.setCursor(0, 1);
        lcd.print("                ");
      } else if (enteredFingerprintID.length() < 3) {
        enteredFingerprintID += key;
        lcd.setCursor(0, 1);
        lcd.print(enteredFingerprintID);
      }
      return;
    }

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Member No:");
    lcd.setCursor(0, 1);
    lcd.print(enteredID);

    if (key == '#') {
      if (enteredID.length() > 0) {
        isChecking = true;
        lcd.clear();
        lcd.print("Checking...");
        checkMember(enteredID);
        enteredID = "";
        isChecking = false;
      }
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
}

// === Functions from the second part of the code ===
// Function to check if a given membership number exists and is eligible for registration
void checkMember(String membershipNo) {
  HTTPClient http;
  String url = serverName + "check_member.php?membershipNo=" + membershipNo;
  http.begin(url);  // Initiate HTTP request
  int code = http.GET();  // Send GET request

  if (code == 200) {
    String response = http.getString();
    if (response.indexOf("not_found") != -1) {
      lcd.clear();
      lcd.print("Member Not Found");
      delay(2000);
      return;
    }

    // Parse JSON response
    StaticJsonDocument<512> doc;
    deserializeJson(doc, response);

    memberNo = doc["membershipNo"].as<String>();
    bool alreadyRegistered = doc["isFingerprintRegistered"];

    if (alreadyRegistered) {
      lcd.clear();
      lcd.print("Already Reg.");
      delay(2000);
      return;
    }

    lcd.clear();
    lcd.print("Enter FP ID:");
    enteredFingerprintID = "";
    waitingForFingerprintID = true;

  } else {
    lcd.clear();
    lcd.print("HTTP Error:");
    lcd.setCursor(0, 1);
    lcd.print(code);
    delay(2000);
  }

  http.end();
}

// Check if fingerprint ID is already used in the database
bool isFingerprintIDUsed(int fpID) {
  HTTPClient http;
  String url = serverName + "check_fingerprint.php?fingerprintID=" + String(fpID);
  Serial.print("Checking fingerprintID: ");
  Serial.println(fpID);
  Serial.print("Request URL: ");
  Serial.println(url);

  http.begin(url);
  int code = http.GET();

  Serial.print("HTTP response code: ");
  Serial.println(code);

  if (code == 200) {
    String response = http.getString();
    response.trim();
    response.toLowerCase();
    Serial.print("Response: '");
    Serial.print(response);
    Serial.println("'");

    http.end();
    return response == "used";
  } else {
    Serial.println("HTTP error");
    http.end();
    return false;
  }
}

// Check if fingerprint ID has a stored model in the sensor
bool hasFPModel(int id) {
  if (finger.loadModel(id) == FINGERPRINT_OK) {
    Serial.println("Model found in sensor.");
    return true;
  } else {
    Serial.println("Model not found.");
    return false;
  }
}

// Enroll fingerprint with verification (2 scans)
void enrollWithVerification(int id) {
  while (finger.getImage() != FINGERPRINT_OK);
  if (finger.image2Tz(1) != FINGERPRINT_OK) {
    lcd.clear(); lcd.print("Scan Fail 1");
    delay(2000); return;
  }

  lcd.clear(); lcd.print("Weka tena...");
  delay(1500);

  while (finger.getImage() != FINGERPRINT_OK);
  if (finger.image2Tz(2) != FINGERPRINT_OK) {
    lcd.clear(); lcd.print("Scan Fail 2");
    delay(2000); return;
  }

  if (finger.createModel() != FINGERPRINT_OK) {
    lcd.clear(); lcd.print("Match Error");
    delay(2000); return;
  }

  if (finger.storeModel(id) == FINGERPRINT_OK) {
    lcd.clear(); lcd.print("Saved ID: "); lcd.print(id);
    updateFingerprint(memberNo, id);
    delay(2000);
  } else {
    lcd.clear(); lcd.print("Store Failed");
    delay(2000);
  }
}

// Update fingerprint ID on server for given member
void updateFingerprint(String membershipNo, int fpID) {
  HTTPClient http;
  http.begin(serverName + "register_fingerprint.php");
  http.addHeader("Content-Type", "application/json");

  String payload = "{\"membershipNo\":\"" + membershipNo + "\",\"fingerprintID\":" + String(fpID) + "}";
  int httpCode = http.POST(payload);

  if (httpCode == 200) {
    lcd.clear(); lcd.print("Reg Success");
    lcd.setCursor(0, 1); lcd.print("FP ID updated!");
  } else {
    lcd.clear(); lcd.print("Reg Failed");
  }

  delay(2000);
  http.end();
  lcd.clear();
}

// Show member info when a fingerprint is recognized
void showMemberInfo(int fingerprintID) {
  HTTPClient http;
  String url = serverName + "get_member_by_fingerprint.php?fingerprintID=" + String(fingerprintID);
  http.begin(url);
  int httpCode = http.GET();

  if (httpCode == 200) {
    String response = http.getString();
    if (response.indexOf("not_found") != -1) {
      lcd.clear(); lcd.print("Not Found");
      delay(2000); return;
    }

    StaticJsonDocument<512> doc;
    DeserializationError error = deserializeJson(doc, response);
    if (error) {
      lcd.clear(); lcd.print("JSON Error");
      delay(2000); return;
    }

    String name = doc["memberName"].as<String>();
    String address = doc["address"].as<String>();

    lcd.clear(); lcd.setCursor(0, 0); lcd.print(name.substring(0, 16));
    lcd.setCursor(0, 1); lcd.print(address.substring(0, 16));
    delay(3000);
  }

  http.end();
}
