#include <WiFi.h>
#include <FirebaseESP32.h>
#include <Keypad.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Adafruit_Fingerprint.h>

// WiFi Settings
#define WIFI_SSID "suza-nG"
#define WIFI_PASSWORD "12345678"

// Firebase Settings
#define DATABASE_URL "https://biometric-10ca8-default-rtdb.firebaseio.com"
#define DATABASE_SECRET "HxsvckyAWfMtRTA0mXTXJmvEhrXKpLhjAqRPXnMO"

// Fingerprint UART Pins
#define FINGERPRINT_RX 16
#define FINGERPRINT_TX 17

// Firebase objects
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

// Fingerprint sensor
HardwareSerial fingerSerial(2);
Adafruit_Fingerprint finger(&fingerSerial);

// LCD
LiquidCrystal_I2C lcd(0x27, 16, 2);

// Keypad (4x3)
const byte ROWS = 4, COLS = 3;
char keys[ROWS][COLS] = {
  {'1','2','3'},
  {'4','5','6'},
  {'7','8','9'},
  {'*','0','#'}
};
byte rowPins[ROWS] = {13, 25, 27, 19};
byte colPins[COLS] = {23, 32, 5};
Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

// Variables
String membershipID = "";
bool captureReady = false;

void setup() {
  Serial.begin(115200);
  lcd.init();
  lcd.backlight();

  lcd.print("Connecting WiFi");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(300);
    lcd.print(".");
  }

  lcd.clear(); lcd.print("WiFi Connected");

  config.database_url = DATABASE_URL;
  config.signer.tokens.legacy_token = DATABASE_SECRET;
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);

  delay(1000);
  lcd.clear(); lcd.print("Firebase Ready");

  fingerSerial.begin(57600, SERIAL_8N1, FINGERPRINT_RX, FINGERPRINT_TX);
  finger.begin(57600);
  delay(1000);

  if (finger.verifyPassword()) {
    lcd.clear(); lcd.print("Sensor Ready");
  } else {
    lcd.clear(); lcd.print("Sensor Error");
    while (1) delay(1);
  }

  delay(2000);
  lcd.clear(); lcd.print("Enter Member ID:");
}

void loop() {
  char key = keypad.getKey();
  if (key) {
    if (key == '#') {
      checkMembershipID();
    } else if (key == '*') {
      membershipID = "";
      lcd.setCursor(0, 1);
      lcd.print("                ");
    } else if (key == '0') {
      verifyFingerprint(); // optional: press 0 to verify
    } else {
      membershipID += key;
      lcd.setCursor(0, 1);
      lcd.print("                ");
      lcd.setCursor(0, 1);
      lcd.print(membershipID);
    }
  }

  if (captureReady) {
    captureReady = false;
    enrollAndSaveFingerprint();
  }
}

void checkMembershipID() {
  lcd.clear(); lcd.print("Checking...");
  String path = "/Members/" + membershipID + "/membershipNo";
  if (Firebase.getString(fbdo, path)) {
    if (fbdo.stringData() == membershipID) {
      lcd.clear(); lcd.print("ID Found");
      delay(1000);
      lcd.clear(); lcd.print("Scan Finger...");
      captureReady = true;
    } else {
      lcdError("Mismatch ID");
    }
  } else {
    lcdError("ID Not Found");
  }
}

void enrollAndSaveFingerprint() {
  lcd.clear(); lcd.print("Scan 1...");
  if (finger.getImage() != FINGERPRINT_OK) { lcdError("1st Scan Fail"); return; }
  if (finger.image2Tz(1) != FINGERPRINT_OK) { lcdError("Tz1 Fail"); return; }

  lcd.clear(); lcd.print("Remove Finger");
  delay(2000); while (finger.getImage() != FINGERPRINT_NOFINGER);

  lcd.clear(); lcd.print("Scan Again...");
  while (finger.getImage() != FINGERPRINT_OK);
  if (finger.image2Tz(2) != FINGERPRINT_OK) { lcdError("Tz2 Fail"); return; }

  if (finger.createModel() != FINGERPRINT_OK) { lcdError("Model Fail"); return; }

  int id = membershipID.toInt();
  if (finger.storeModel(id) == FINGERPRINT_OK) {
    lcd.clear(); lcd.print("Stored Local");

    String path = "/Members/" + membershipID + "/fingerprintID";
    if (Firebase.setInt(fbdo, path, id)) {
      lcd.setCursor(0, 1);
      lcd.print("Saved to Cloud");
    } else {
      lcdError("Cloud Save Err");
    }
  } else {
    lcdError("Store Fail");
  }

  delay(3000);
  resetInput();
}


void verifyFingerprint() {
  lcd.clear(); lcd.print("Place Finger");
  if (finger.getImage() != FINGERPRINT_OK) { lcdError("No Image"); return; }
  if (finger.image2Tz() != FINGERPRINT_OK) { lcdError("Tz Fail"); return; }
  if (finger.createModel() != FINGERPRINT_OK) { lcdError("Model Fail"); return; }

  if (finger.fingerFastSearch() == FINGERPRINT_OK) {
    int foundID = finger.fingerID;
    Serial.print("Found ID: "); Serial.println(foundID);

    if (Firebase.getJSON(fbdo, "/Members")) {
      FirebaseJson &json = fbdo.jsonObject();
      FirebaseJsonData data;

      size_t count = json.iteratorBegin();
      int type; String key, value;

      for (size_t i = 0; i < count; i++) {
        if (json.iteratorGet(i, type, key, value) == 0) {
          if (json.get(data, key + "/fingerprintID") && data.intValue == foundID) {
            String name = "", memberNo = "";
            json.get(data, key + "/memberName"); name = data.stringValue;
            json.get(data, key + "/membershipNo"); memberNo = data.stringValue;

            lcd.clear(); lcd.print(name.substring(0, 16));
            lcd.setCursor(0, 1); lcd.print("ID: " + memberNo);
            json.iteratorEnd();
            delay(4000);
            resetInput();
            return;
          }
        }
      }
      json.iteratorEnd();
      lcdError("Match Not Found");
    } else {
      lcdError("DB Read Err");
    }
  } else {
    lcdError("Not Matched");
  }
}

void lcdError(String msg) {
  lcd.clear(); lcd.print(msg); delay(2000); resetInput();
}

void resetInput() {
  membershipID = "";
  lcd.clear(); lcd.print("Enter Member ID:");
}
