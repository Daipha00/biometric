#include <WiFi.h>
#include <HTTPClient.h>
#include <Wire.h>
#include <LCD_I2C.h>
#include <ArduinoJson.h>
#include <Keypad.h>
#include <String>
#include <Adafruit_Fingerprint.h>

// WiFi credentials
const char* ssid = ".";
const char* password = "11223344555";
String serverName = "http://192.168.12.2/biometric_api/";

// LCD setup
LCD_I2C lcd(0x27, 20, 4);

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

int FID[] = {};

byte rowPins[ROWS] = {13, 25, 27, 19};
byte colPins[COLS] = {23, 32, 5};
Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

// State variables
enum SystemState {
  SHOW_MENU,
  WAIT_FOR_MENU_SELECTION,
  SHOW_INFO_WAIT_FINGERPRINT,
  REGISTRATION_WAIT_FP_ID,
  REGISTRATION_ENROLL,
  PROCESSING
};

SystemState currentState = SHOW_MENU;

String enteredFPID = "";
String enteredMemberID = "";
String currentMemberNo = "";

void setup() {
  Serial.begin(115200);
  mySerial.begin(57600, SERIAL_8N1, 16, 17);
  finger.begin(57600);

  lcd.begin();
  lcd.backlight();

  lcd.clear();
  lcd.setCursor(0, 0); 
  lcd.print("Connecting...");
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    lcd.print(".");
  }
  delay(500);
  lcd.clear();

  if (!finger.verifyPassword()) {
    lcd.print("FP Sensor Err!");
    while (true);
  }
  finger.emptyDatabase();

}

void loop() {
  switch (currentState) {
    case SHOW_MENU:
      showMenu();
      currentState = WAIT_FOR_MENU_SELECTION;
      break;
    case WAIT_FOR_MENU_SELECTION:
      handleMenuInput();
      break;
    case SHOW_INFO_WAIT_FINGERPRINT:
      handleFingerprintForInfo();
      break;
    case REGISTRATION_WAIT_FP_ID:
      handleFingerprintIDEntry();
      break;
    case REGISTRATION_ENROLL:
      enrollFingerprintProcess();
      break;
    case PROCESSING:
      break;
  }
}

void showMenu() {
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Karibu!");
  delay(1500);

  lcd.clear();
  lcd.print("6: Taarifa");
  lcd.setCursor(0,1);
  lcd.print("4: Usajili");
}

void handleMenuInput() {
  char key = keypad.getKey();
  if (!key) return;

  if (key == '6') {
    lcd.clear();
    lcd.print("Weka kidole");
    enteredFPID = "";
    currentState = SHOW_INFO_WAIT_FINGERPRINT;
  } else if (key == '4') {
    lcd.clear();
    lcd.print("Ingiza MemberNo:");
    enteredMemberID = "";
    currentState = REGISTRATION_WAIT_FP_ID;
  }
}


void handleFingerprintForInfo() {
  int p = finger.getImage();
  if (p == FINGERPRINT_OK) {
      p = finger.image2Tz();
      p = finger.fingerSearch();
      int fid = finger.fingerID;

    if (fid > 0) {
      lcd.clear();
      lcd.print("Kuchakata... ");
      showMemberInfo(fid);
      delay(3000);
      currentState = SHOW_MENU;
      
    } else {
      lcd.clear();
      lcd.print("Model Haipatikani ");
      delay(2000);
      lcd.clear();
      lcd.print("Jaribu tena");
      delay(1500);
      lcd.clear();
      lcd.print("Weka kidole");
    }
  }
}

void handleFingerprintIDEntry() {
  char key = keypad.getKey();
  if (!key) return;

  if (key == '#' || key == '0') {
    if (enteredMemberID.length() == 0) return;
    lcd.clear();
    lcd.print("Kukagua...");
    currentMemberNo = enteredMemberID;
    if (checkMemberExists(currentMemberNo)) {
      if (isMemberRegisteredWithFP(currentMemberNo)) {
        lcd.clear();
        lcd.print("Imeandikishwa");
        delay(2000);
        currentState = SHOW_MENU;
      } else {
        lcd.clear();
        lcd.print("Ingiza FP ID");
        enteredFPID = "";
        currentState = REGISTRATION_ENROLL;
      }
    } else {
      lcd.clear();
      lcd.print("Member Hapana");
      delay(2000);
      currentState = SHOW_MENU;
    }
    enteredMemberID = "";
  } else if (key == '*') {
    enteredMemberID = "";
    lcd.clear();
    lcd.print("Ingiza MemberNo:");
  } else {
    if (enteredMemberID.length() < 10) {
      enteredMemberID += key;
      lcd.setCursor(0,1);
      lcd.print(enteredMemberID);
    }
  }
}

void enrollFingerprintProcess() {
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("START ENROLL");
  char key = keypad.getKey();
  if (!key) return;
  delay(2000);

  if (key == '#') {
    if (enteredFPID.length() == 0) return;

    int fpID = enteredFPID.toInt();

    if (isFingerprintIDUsed(fpID) || hasFPModel(fpID)) {
      lcd.setCursor(0,1);
      lcd.print("FP ID Tayari");
      delay(2000);
      currentState = SHOW_MENU;
      enteredFPID = "";
      return;
    }

    lcd.clear();
    lcd.print("Weka Kidole 1");
    if (!enrollWithVerification(fpID)) {
      lcd.clear();
      lcd.print("Usajili Fail");
      delay(2000);
      currentState = SHOW_MENU;
      return;
    }

    enteredFPID = "";
    currentState = SHOW_MENU;

  } else if (key == '*') {
    enteredFPID = "";
    lcd.setCursor(0,1);
    lcd.print("           ");
  } else {
    if (enteredFPID.length() < 3) {
      enteredFPID += key;
      lcd.setCursor(0,1);
      lcd.print(enteredFPID);
    }
  }
}

void showMemberInfo(int fingerprintID) {

  HTTPClient http;
  String url = serverName + "get_member_by_fingerprint.php?fingerprintID=" + String(fingerprintID);
  http.begin(url);
  int code = http.GET();

  if (code == 200) {
    String response = http.getString();
    StaticJsonDocument<512> doc;
    DeserializationError error = deserializeJson(doc, response);

    if (!error) {
      String name = doc["member_name"].as<String>();
      String address = doc["address"].as<String>();

      lcd.clear();
      lcd.setCursor(0,0);
      lcd.print(name.substring(0, 16));
      lcd.setCursor(0, 1);
      lcd.print(address.substring(0, 16));
      delay(3000);
    } else {
      lcd.clear();
      lcd.print("JSON Error");
      delay(2000);
    }
  } else {
    lcd.clear();
    lcd.print("HTTP Error");
    delay(2000);
  }

  http.end();
}

bool checkMemberExists(String membershipNo) {
  HTTPClient http;
  String url = serverName + "check_member.php?membershipNo=" + membershipNo;
  http.begin(url);
  int code = http.GET();

  if (code == 200) {
    String response = http.getString();
    http.end();
    return response.indexOf("not_found") == -1;
  }
  http.end();
  return false;
}

bool isMemberRegisteredWithFP(String membershipNo) {
  HTTPClient http;
  String url = serverName + "check_member_fingerprint.php?membershipNo=" + membershipNo;
  http.begin(url);
  int code = http.GET();

  if (code == 200) {
    String response = http.getString();
    http.end();
    return response.indexOf("yes") != -1;
  }
  http.end();
  return false;
}

bool isFingerprintIDUsed(int fpID) {
  HTTPClient http;
  String url = serverName + "check_fingerprint.php?fingerprintID=" + String(fpID);
  http.begin(url);
  int code = http.GET();

  if (code == 200) {
    String response = http.getString();
    response.trim();
    response.toLowerCase();
    http.end();
    return response == "used";
  }
  http.end();
  return false;
}

bool hasFPModel(int id) {
  return (finger.loadModel(id) == FINGERPRINT_OK);
}

bool enrollWithVerification(int id) {
  while (finger.getImage() != FINGERPRINT_OK);
    if (finger.image2Tz(1) != FINGERPRINT_OK) {
      lcd.clear(); 
      lcd.setCursor(0,0);
      lcd.print("Scan Fail 1");
      lcd.setCursor(0,1);
      lcd.print("Rudia Tena Kidole Cha Kwanza");
      delay(2000);
      return false;
    }
  
  

  lcd.clear(); lcd.print("Weka tena...");
  delay(1500);

  while (finger.getImage() != FINGERPRINT_OK);
    if (finger.image2Tz() != FINGERPRINT_OK) {
      lcd.clear(); lcd.print("Scan Fail 2");
      delay(2000);
      return false;
    }
  
  
  /* delay(2000);
  if (!CheckFId(id)) {
    lcd.clear(); lcd.print("Match Error");
    delay(2000);
    return false;
  }  */

  if (finger.storeModel(id) == FINGERPRINT_OK) {
    lcd.clear();
    lcd.print("Saved ID: ");
    lcd.setCursor(0, 1);
    lcd.print(id);
    updateFingerprint(currentMemberNo, id);
    
    if (sizeof(FID) > 0)
    {
      FID[sizeof(FID)+1] = id;
    }else{
      FID[0] = id;
    }
    
    delay(2000);

    if (finger.storeModel(id) == FINGERPRINT_OK) {
  if (finger.loadModel(id) == FINGERPRINT_OK) {
    lcd.clear(); lcd.print("Saved OK & Exists");
    Serial.println("Model saved and confirmed.");
  } else {
    lcd.clear(); lcd.print("Save OK but Not Found");
    Serial.println("Model NOT actually saved!");
  }
}
    return true;
  } else {
    lcd.clear();
    lcd.print("Store Fail");
    delay(2000);
    return false;
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
