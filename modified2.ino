#include <TFT_eSPI.h>
#include <SPI.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <Arduino.h>
#if defined(ESP32)
#include <WiFi.h>
#elif defined(ESP8266)
#include <ESP8266WiFi.h>
#endif
#include <Firebase_ESP_Client.h>
//Provide the token generation process info.
#include "addons/TokenHelper.h"
//Provide the RTDB payload printing info and other helper functions.
#include "addons/RTDBHelper.h"


class Menu {
public:
  void drawMenuButton1(TFT_eSPI& tft, uint32_t fillColor) {
    tft.fillRoundRect(5, 185, 100, 50, 10, fillColor);
    tft.drawRoundRect(5, 185, 100, 50, 10, TFT_WHITE);
    tft.setTextColor(TFT_WHITE);
    tft.setTextSize(1);
    tft.setFreeFont(&FreeSerif9pt7b);
    int textWidth = tft.textWidth("Set");
    tft.drawString("Set", 5 + (100 - textWidth) / 2, 205);
  }

  void drawMenuButton2(TFT_eSPI& tft, uint32_t fillColor, String label) {
    tft.fillRoundRect(110, 185, 100, 50, 10, fillColor);
    tft.drawRoundRect(110, 185, 100, 50, 10, TFT_WHITE);
    tft.setTextColor(TFT_WHITE);
    tft.setTextSize(1);
    tft.setFreeFont(&FreeSerif9pt7b);
    int textWidth = tft.textWidth(label);
    tft.drawString(label, 110 + (100 - textWidth) / 2, 205);
  }

  void drawMenuButton3(TFT_eSPI& tft, uint32_t fillColor) {
    tft.fillRoundRect(215, 185, 100, 50, 10, fillColor);
    tft.drawRoundRect(215, 185, 100, 50, 10, TFT_WHITE);
    tft.setTextColor(TFT_WHITE);
    tft.setTextSize(1);
    tft.setFreeFont(&FreeSerif9pt7b);
    int textWidth = tft.textWidth("Warm");
    tft.drawString("Warm", 215 + (100 - textWidth) / 2, 205);
  }

  void setTempValDisplay(TFT_eSPI& tft, String Value) {
    tft.setTextColor(TFT_WHITE);
    tft.setTextSize(1);
    tft.setFreeFont(&FreeSerif9pt7b);
    String text = "Set to: " + Value;
    int textWidth = tft.textWidth(text);
    tft.fillRect(0, 0, 320, 45, TFT_BLACK);
    tft.drawString(text, (320 - textWidth) / 2, 10);
  }

  void currentTempDisplay(TFT_eSPI& tft, float temperature) {
    tft.setTextSize(2);
    tft.setFreeFont(&FreeSerif18pt7b);
    String text = String(int(temperature));
    int textWidth = tft.textWidth(text);
    tft.fillRect(0, 50, 320, 100, TFT_BLACK);
    tft.drawString(text, (320 - textWidth) / 2, 50);
  }
};


TFT_eSPI tft = TFT_eSPI();


#define TEMP_MENU_BUTTON_WIDTH 100
#define TEMP_MENU_BUTTON_HEIGHT 70
#define TEMP_MENU_BUTTON_MARGIN_X 5
#define TEMP_MENU_BUTTON_MARGIN_Y 5
#define KEY_PAD_BUTTON_WIDTH 60
#define KEY_PAD_BUTTON_HEIGHT 50
#define KEY_PAD_BUTTON_MARGIN_X 5
#define KEY_PAD_BUTTON_MARGIN_Y 5
#define DISP_W 100
#define DISP_H 50
#define NUM_LEN 2
#define RELAY D0

#define WIFI_SSID "Galaxy M02s1065"
#define WIFI_PASSWORD "12345678"
// Insert Firebase project API Key
#define API_KEY "AIzaSyD9GUVG9AbgYvNT9dNTXRBPB8_K4h1GXL8"
// Insert RTDB URLefine the RTDB URL */
#define DATABASE_URL "https://ipoursmartkettle-default-rtdb.firebaseio.com/"

int row;
int col;
int tempMenuStartX = 5;
int tempMenuStartY = 50;
int keyPadStartX = 120;
int keyPadStartY = 15;
int tempMenuButtonX;
int tempMenuButtonY;
int keyPadButtonX;
int keyPadButtonY;
int cursor_X;
int cursor_Y;

bool currentMenuDrawn = false;
bool menuButton2Touched = false;
uint32_t menuButton2Color = TFT_BLUE;
bool menuButton3Touched = false;
uint32_t menuButton3Color = TFT_BLUE;
//String menuButton2Label = "On";
char startMenuLabel[3][10] = { "Set", "Warm", "Settings" };
char tempMenuLabel[6][10] = { "Boil", "Coffee", "Tea", "Green Tea", "Hot choc", "Custom" };
char keyPadLabel[12][4] = { "1", "2", "3", "4", "5", "6", "7", "8", "9", "0", "D", "ok" };
String crrentMenu = "menu1";
String previousMenu = crrentMenu;
char numberBuffer[NUM_LEN + 1] = "";
uint8_t numberIndex = 0;
String tempValue;
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;
bool signupOK = false;
bool relayOffOk = false;
bool stepwiseIcrease = false;
bool justHeat = false;

Menu menu;

int oneWireBus = 10;
int previousTemperature = 0;
float temperatureC;
float setTemperature;
OneWire oneWire(oneWireBus);
DallasTemperature sensors(&oneWire);

unsigned long currentMillis;
unsigned long previousMillis1 = 0;
unsigned long previousMillis2 = 0;
unsigned long interval = 10000;
unsigned long lastTouchTime = 0;
unsigned long debounceDelay = 500;
unsigned long previousMillis = 0;
const unsigned long relayOnDuration = 3000;
const unsigned long relayOffDuration = 2000;
bool relayState = false;


void setup(void) {

  tft.init();
  //tft.setTextColor(TFT_WHITE);
  //tft.setTextSize(1);
  //tft.setFreeFont(&FreeSerif9pt7b);
  tft.setRotation(1);
  tft.fillScreen(TFT_BLACK);
  menu1();
  sensors.begin();
  pinMode(RELAY, OUTPUT);
  digitalWrite(RELAY, HIGH);
  Serial.begin(115200);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to Wi-Fi");
  unsigned long startTime = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - startTime < 10000) {
    Serial.print(".");
    delay(300);
  }


  if (WiFi.status() == WL_CONNECTED) {
    Serial.println();
    Serial.print("Connected with IP: ");
    Serial.println(WiFi.localIP());
    Serial.println();
    config.api_key = API_KEY;


    config.database_url = DATABASE_URL;


    if (Firebase.signUp(&config, &auth, "", "")) {
      Serial.println("ok");
      signupOK = true;
    } else {
      Serial.printf("%s\n", config.signer.signupError.message.c_str());
    }


    config.token_status_callback = tokenStatusCallback;  //see addons/TokenHelper.h

    Firebase.begin(&config, &auth);
    Firebase.reconnectWiFi(true);
  }
}

void loop() {
  sensors.requestTemperatures();
  temperatureC = sensors.getTempCByIndex(0);
  currentMillis = millis();
  Touch();
  drawMenu();
  if (WiFi.status() == WL_CONNECTED) {
    firebaseUpload();
    firebaseFetch();
  }
  relay();
  currentTemp();
}


void drawMenu() {

  if (crrentMenu == "menu1") {

    if (!currentMenuDrawn) {  // Draw menu only if it's not drawn yet
      tft.fillScreen(TFT_BLACK);
      menu1();
      currentMenuDrawn = true;  // Set the flag to true after drawing menu once
    }

    /*
    if (currentMillis - previousMillis >= interval) {
      currentTempDisplay();
      previousMillis = currentMillis;
    }*/


  } else if (crrentMenu == "menu2") {

    if (!currentMenuDrawn) {  // Draw menu only if it's not drawn yet
      tft.fillScreen(TFT_BLACK);
      backButton();
      menu2();
      currentMenuDrawn = true;  // Set the flag to true after drawing menu once
    }

  } else if (crrentMenu == "menu3") {

    if (!currentMenuDrawn) {  // Draw menu only if it's not drawn yet
      tft.fillScreen(TFT_BLACK);
      backButton();
      menu3();
      currentMenuDrawn = true;  // Set the flag to true after drawing menu once
    }
    keyPadDisplay();
  }
}




void Touch() {

  if (currentMillis - lastTouchTime < debounceDelay) {
    return;
  }
  uint16_t x = 0, y = 0;
  if (tft.getTouch(&x, &y)) {

    x = map(x, 0, 320, 0, 320);
    y = map(y, 0, 240, 240, 0);


    if (crrentMenu == "menu1") {
      if (x >= 110 && x <= 210 && y >= 185 && y <= 235) {
        menuButton2Touched = !menuButton2Touched;
        Serial.println("Button is touched");

        if (menuButton2Touched) {
          menu.drawMenuButton2(tft, TFT_RED, "Off");  // Change to red if current color is blue
          //justHeat = false;
          //digitalWrite(RELAY, LOW);
        }
        if (!menuButton2Touched) {
          menu.drawMenuButton2(tft, TFT_BLUE, "On");  // Change to blue if current color is red
          //digitalWrite(RELAY, HIGH);
        }
      }



      if (x >= 215 && x <= 315 && y >= 185 && y <= 235) {
        menuButton3Touched = !menuButton3Touched;
        //Serial.println("Button is touched");
        Serial.println(menuButton3Touched, DEC);
        if (menuButton3Touched) {
          menu.drawMenuButton3(tft, TFT_RED);  // Change to red if current color is blue
        }
        if (!menuButton3Touched) {
          menu.drawMenuButton3(tft, TFT_BLUE);  // Change to blue if current color is red
        }
      }


      if (x >= 5 && x <= 105 && y >= 185 && y <= 235) {
        tempValue = "";
        menuButton2Touched = false;
        menuButton3Touched = false;
        crrentMenu = "menu2";
        currentMenuDrawn = false;
      }
    }



    if (crrentMenu == "menu2") {

      if (x >= 10 && x <= 80 && y >= 10 && y <= 40) {
        crrentMenu = "menu1";
        currentMenuDrawn = false;
      }


      for (int i = 0; i < 6; i++) {
        row = i / 3;
        col = i % 3;

        tempMenuButtonX = tempMenuStartX + col * (TEMP_MENU_BUTTON_WIDTH + TEMP_MENU_BUTTON_MARGIN_X);
        tempMenuButtonY = tempMenuStartY + row * (TEMP_MENU_BUTTON_HEIGHT + TEMP_MENU_BUTTON_MARGIN_Y);

        if (x >= tempMenuButtonX && x <= tempMenuButtonX + TEMP_MENU_BUTTON_WIDTH && y >= tempMenuButtonY && y <= tempMenuButtonY + TEMP_MENU_BUTTON_HEIGHT) {

          if (i == 0) {
            setTemperature = 97;
            tempValue = "Boil";
            crrentMenu = "menu1";
            currentMenuDrawn = false;
          } else if (i == 1) {
            setTemperature = 95;
            tempValue = "Coffee";
            crrentMenu = "menu1";
            currentMenuDrawn = false;
          } else if (i == 2) {
            setTemperature = 95;
            tempValue = "Tea";
            crrentMenu = "menu1";
            currentMenuDrawn = false;
          } else if (i == 3) {
            setTemperature = 70;
            tempValue = "Green tea";
            crrentMenu = "menu1";
            currentMenuDrawn = false;
          } else if (i == 4) {
            setTemperature = 65;
            tempValue = "Hot chocolate";
            crrentMenu = "menu1";
            currentMenuDrawn = false;
          } else if (i == 5) {
            crrentMenu = "menu3";
            currentMenuDrawn = false;
          }

          Serial.print("Button ");
          Serial.print(tempMenuLabel[i]);
          Serial.println(" is touched");
        }
      }
    }




    if (crrentMenu == "menu3") {

      if (x >= 10 && x <= 80 && y >= 10 && y <= 40) {
        numberIndex = 0;
        numberBuffer[numberIndex] = 0;
        tempValue = "";
        crrentMenu = "menu2";
        currentMenuDrawn = false;
      }

      for (int i = 0; i < 12; i++) {
        row = i / 3;
        col = i % 3;

        keyPadButtonX = keyPadStartX + col * (KEY_PAD_BUTTON_WIDTH + KEY_PAD_BUTTON_MARGIN_X);
        keyPadButtonY = keyPadStartY + row * (KEY_PAD_BUTTON_HEIGHT + KEY_PAD_BUTTON_MARGIN_Y);

        if (x >= keyPadButtonX && x <= keyPadButtonX + KEY_PAD_BUTTON_WIDTH && y >= keyPadButtonY && y <= keyPadButtonY + KEY_PAD_BUTTON_HEIGHT) {

          if (i < 10) {

            if (numberIndex < NUM_LEN) {
              numberBuffer[numberIndex] = keyPadLabel[i][0];
              numberIndex++;
              numberBuffer[numberIndex] = 0;  // zero terminate
            }


          } else if (i == 10) {

            numberBuffer[numberIndex] = 0;
            if (numberIndex > 0) {
              numberIndex--;
              numberBuffer[numberIndex] = 0;
            }


          } else if (i == 11 && numberBuffer[0] != '\0') {
            setTemperature = tempValue.toFloat();
            crrentMenu = "menu1";
            currentMenuDrawn = false;
          }

          Serial.println(numberBuffer);
        }
      }
    }
  }
  lastTouchTime = currentMillis;
}

void backButton() {
  tft.setFreeFont(&FreeSerif9pt7b);
  tft.setTextColor(TFT_WHITE);
  tft.setTextSize(1);
  tft.fillRoundRect(10, 10, 70, 30, 10, TFT_BLUE);
  tft.drawRoundRect(10, 10, 70, 30, 10, TFT_WHITE);
  tft.setCursor(15, 25);
  tft.print("Back");
}


void menu1() {
  /*
  tft.setTextColor(TFT_WHITE);
  tft.setTextSize(1);
  tft.setFreeFont(&FreeSerif9pt7b);
  String text = "Set to: " + tempValue;
  int textWidth = tft.textWidth(text);
  tft.drawString(text, (320 - textWidth) / 2, 10);
  */

  menu.drawMenuButton1(tft, TFT_BLUE);
  menu.drawMenuButton2(tft, TFT_BLUE, "On");
  menu.drawMenuButton3(tft, TFT_BLUE);
  menu.setTempValDisplay(tft, tempValue);
  menu.currentTempDisplay(tft, temperatureC);
}




void menu2() {

  tft.setFreeFont(&FreeSerif9pt7b);
  tft.setTextColor(TFT_WHITE);
  tft.setTextSize(1);

  for (int i = 0; i < 6; i++) {
    row = i / 3;
    col = i % 3;

    tempMenuButtonX = tempMenuStartX + col * (TEMP_MENU_BUTTON_WIDTH + TEMP_MENU_BUTTON_MARGIN_X);
    tempMenuButtonY = tempMenuStartY + row * (TEMP_MENU_BUTTON_HEIGHT + TEMP_MENU_BUTTON_MARGIN_Y);

    tft.fillRoundRect(tempMenuButtonX, tempMenuButtonY, TEMP_MENU_BUTTON_WIDTH, TEMP_MENU_BUTTON_HEIGHT, 10, TFT_RED);
    tft.drawRoundRect(tempMenuButtonX, tempMenuButtonY, TEMP_MENU_BUTTON_WIDTH, TEMP_MENU_BUTTON_HEIGHT, 10, TFT_WHITE);

    //int labelX = tempMenuButtonX + TEMP_MENU_BUTTON_WIDTH / 2 - 5;
    int labelY = tempMenuButtonY + TEMP_MENU_BUTTON_HEIGHT / 2 - 5;
    int textWidth = tft.textWidth(tempMenuLabel[i]);
    tft.drawString(tempMenuLabel[i], tempMenuButtonX + (TEMP_MENU_BUTTON_WIDTH - textWidth) / 2, labelY);
    //tft.setCursor(labelX, labelY);
    //tft.print(tempMenuLabel[i]);
  }
}


void menu3() {
  tft.setTextColor(TFT_WHITE);
  tft.setTextSize(1);

  for (int i = 0; i < 12; i++) {
    row = i / 3;
    col = i % 3;

    keyPadButtonX = keyPadStartX + col * (KEY_PAD_BUTTON_WIDTH + KEY_PAD_BUTTON_MARGIN_X);
    keyPadButtonY = keyPadStartY + row * (KEY_PAD_BUTTON_HEIGHT + KEY_PAD_BUTTON_MARGIN_Y);

    tft.fillRoundRect(keyPadButtonX, keyPadButtonY, KEY_PAD_BUTTON_WIDTH, KEY_PAD_BUTTON_HEIGHT, 10, TFT_BLUE);
    tft.drawRoundRect(keyPadButtonX, keyPadButtonY, KEY_PAD_BUTTON_WIDTH, KEY_PAD_BUTTON_HEIGHT, 10, TFT_WHITE);

    int labelX = keyPadButtonX + KEY_PAD_BUTTON_WIDTH / 2 - 5;
    int labelY = keyPadButtonY + KEY_PAD_BUTTON_HEIGHT / 2 - 5;


    tft.setCursor(labelX, labelY);
    tft.print(keyPadLabel[i]);
  }
}

void keyPadDisplay() {
  cursor_X = 10;
  cursor_Y = 100;
  tft.setTextSize(2);
  tft.setFreeFont(&FreeSansBold12pt7b);
  tempValue = numberBuffer;
  int xwidth = tft.drawString(numberBuffer, cursor_X + 4, cursor_Y + 12);
  tft.fillRect(cursor_X + 4 + xwidth, cursor_Y + 1, DISP_W - xwidth - 5, DISP_H - 2, TFT_BLACK);
}

/*
void currentTempDisplay() {
  tft.setTextSize(2);
  tft.setFreeFont(&FreeSerif18pt7b);
  String text = String(int(temperatureC));
  int textWidth = tft.textWidth(text);
  tft.fillRect(0, 50, 320, 100, TFT_BLACK);
  tft.drawString(text, (320 - textWidth) / 2, 50);
}
*/

void currentTemp() {
  int temperature = int(temperatureC);
  if (crrentMenu == "menu1") {
    if (temperature > previousTemperature || temperature < previousTemperature) {
      menu.currentTempDisplay(tft, temperatureC);
      previousTemperature = temperature;
    }
  }
}

void firebaseUpload() {

  if (Firebase.ready() && signupOK) {

    if (Firebase.RTDB.setFloat(&fbdo, "/users/kettle/currentTemperature", temperatureC)) {
      Serial.print("current Temperature: ");
      Serial.println(temperatureC);
    }
    if (Firebase.RTDB.setFloat(&fbdo, "/users/kettle/inputTemperature", tempValue.toFloat())) {
      Serial.print("in put Temperature: ");
      Serial.println(tempValue);
    }
    if (Firebase.RTDB.setBool(&fbdo, "/users/kettle/kettleOn", menuButton2Touched)) {
      Serial.print("kettle on: ");
      Serial.println(menuButton2Touched);
    }
    if (Firebase.RTDB.setBool(&fbdo, "/users/kettle/keepWarm", menuButton3Touched)) {
      Serial.print("keep warm: ");
      Serial.println(menuButton3Touched);

    } else {
      Serial.println("FAILED");
      Serial.println("REASON: " + fbdo.errorReason());
    }
  }
}

void firebaseFetch() {
  if (Firebase.RTDB.getInt(&fbdo, "/users/kettle/kettleAppInputTemp")) {
    int value = fbdo.intData();
    tempValue = String(value);
    setTemperature = float(value);
    //int myInt = int(value);
    if (crrentMenu == "menu1") {
      menu.setTempValDisplay(tft, tempValue);
    }
  }
  if (Firebase.RTDB.getBool(&fbdo, "/users/kettle/kettleAppOn")) {
    menuButton2Touched = fbdo.boolData();
    if (crrentMenu == "menu1") {
      if (menuButton2Touched) {
        menu.drawMenuButton2(tft, TFT_RED, "Off");  // Change to red if current color is blue
        justHeat = false;
        //digitalWrite(RELAY, LOW);
      }
      if (!menuButton2Touched) {
        menu.drawMenuButton2(tft, TFT_BLUE, "On");  // Change to blue if current color is red
        //digitalWrite(RELAY, HIGH);
      }
    }
  }
  if (Firebase.RTDB.getBool(&fbdo, "/users/kettle/kettleAppWarm")) {
    menuButton3Touched = fbdo.boolData();
    if (crrentMenu == "menu1") {
      if (menuButton3Touched) {
        menu.drawMenuButton3(tft, TFT_RED);  // Change to red if current color is blue
      }
      if (!menuButton3Touched) {
        menu.drawMenuButton3(tft, TFT_BLUE);  // Change to blue if current color is red
      }
    }
  } else {
    Serial.println(fbdo.errorReason());
  }
}


void relay() {
  if (menuButton2Touched && !menuButton3Touched) {
    digitalWrite(RELAY, LOW) ;
    if (temperatureC >= setTemperature-5) {
      digitalWrite(RELAY, HIGH);
      menuButton2Touched = false;
      if (crrentMenu == "menu1") {
        if (menuButton2Touched) {
          menu.drawMenuButton2(tft, TFT_RED, "Off");  // Change to red if current color is blue
        }
        if (!menuButton2Touched) {
          menu.drawMenuButton2(tft, TFT_BLUE, "On");  // Change to blue if current color is red
        }
      }
    }
  }
  if (menuButton2Touched && menuButton3Touched){
    if (temperatureC >= setTemperature-5){
      digitalWrite(RELAY, HIGH) ;
    }else{
      digitalWrite(RELAY, LOW) ;
    }
  }
}