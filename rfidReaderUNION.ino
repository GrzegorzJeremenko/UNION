#include <ArduinoJson.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <SPI.h>
#include <MFRC522.h>
#include <EEPROM.h>
#include <WiFiUdp.h>


#define SS_PIN D4
#define RST_PIN D3

#define BEEP_PIN D8
#define KEYBOARD_PIN D0

#define baudRate 115200


// Define special characters

byte backSlash[] = {
	
	B00000,
	B10000,
	B01000,
	B00100,
	B00010,
	B00001,
	B00000,
	B00000
	
};

byte dot[] = {
	
	B00000,
	B00000,
	B01110,
	B10001,
	B10001,
	B10001,
	B01110,
	B00000
	
};

byte dotFill[] = {
	
	B00000,
	B00000,
	B01110,
	B11111,
	B11111,
	B11111,
	B01110,
	B00000
	
};

byte polishZ[] = {
	
	B00100,
	B00000,
	B11111,
	B00010,
	B00100,
	B01000,
	B11111,
	B00000
  
};

byte polishE[] = {
	
	B00000,
	B00000,
	B01110,
	B10001,
	B11111,
	B10000,
	B01110,
	B00010
  
};

byte polishL[] = {
	
	B01100,
	B00100,
	B00100,
	B00110,
	B01100,
	B00100,
	B01110,
	B00000
  
};

byte polishA[] = {
	
	B00000,
	B00000,
	B01110,
	B00001,
	B01111,
	B10001,
	B01111,
	B00010
	
};


// Define variables

String ver = "1.0", cardUID = "", pinCode = "", ipAdress="85.89.189.114";

char ssid[50], pass[50], json[200];

int pin, uid, animationTimer = 30, special = 0, screenFrame = 0;

bool animationDot = false, twoByte = false;


// Define adress to communication with LCD 

LiquidCrystal_I2C lcd(0x27, 16, 2);


// Define PINs to communication with RFID scanner

MFRC522 mfrc522(SS_PIN, RST_PIN);

void setup() {
	
	// LCD include

	lcd.init();
	lcd.backlight();
	lcd.clear();
	lcd.setCursor(3, 0);
	lcd.print("UNION v");
	lcd.print(ver);
	

	// Boot delay
	
	tone(BEEP_PIN, 659, 200);
	delay(200);
	tone(BEEP_PIN, 698, 200);
	delay(200);
	tone(BEEP_PIN, 587, 200);
	delay(200);
	tone(BEEP_PIN, 784, 350);


	// Start serial port

	Serial.begin(baudRate);

	Serial.println("");
	Serial.println("Starting devices...");
	
	lcd.setCursor(0, 1);
	lcd.print(" Serial monitor ");


	// SPI begin

	SPI.begin();
	
	lcd.setCursor(0, 1);
	lcd.print("     SPI...     ");


	// RFID module include

	mfrc522.PCD_Init();
	
	lcd.setCursor(0, 1);
	lcd.print("  RFID  module  ");


	// LCD special characters

	lcd.createChar(0, dot);
	lcd.createChar(1, dotFill);
	lcd.createChar(2, backSlash);
	lcd.createChar(3, polishZ);
	lcd.createChar(4, polishE);
	lcd.createChar(5, polishL);
	lcd.createChar(6, polishA);
	
	
	// Pin Mode
	
	pinMode(BEEP_PIN, OUTPUT);
	pinMode(KEYBOARD_PIN, INPUT_PULLUP);

	Serial.println("Done!");
	
	
	// Basic function
	
	loadEEPROM();
	
	wifiConnect();
	
	loginDeviceAPI();
	
}

void loop() {
	
	if(screenFrame == 0) defaultScreen();
	else if(screenFrame == 1) infoScreen();
	
}

void infoScreen() {
	
	special = 0;
	animationTimer = 30;
	
	lcd.clear();
	
	lcd.setCursor(0,0);
	lcd.print("   Informacje");
	
	lcd.setCursor(0,1);
	
	lcd.print("Zasi");
	lcd.write(4);
	lcd.print("g: ");
	lcd.print(WiFi.RSSI());
	lcd.print("dBm");
	
	while(Serial.available() > 0) char t = Serial.read();
	
	delay(4000);

	lcd.clear();
	screenFrame = 0;

	
}

void defaultScreen() {
	
	// Animation
	
	lcd.setBacklight(1);
	
	lcd.setCursor(0, 0);
	lcd.print("  Zbli");
	lcd.write(3);
	lcd.print("  kart");
	lcd.write(4);
	
	animationTimer++;
	if(animationTimer >= 30) {
	
		lcd.setCursor(4, 1);
		animationTimer = 0;
		animationDot = !animationDot;
		
		if(animationDot == 0) lcd.write(0);
		else lcd.write(1);
		
		lcd.setCursor(6, 1);
		lcd.write(0);
		
		lcd.setCursor(9, 1);
		lcd.write(0);
		
		lcd.setCursor(11, 1);
		lcd.write(0);
	
	}
	
	pinPresent(1);
	
	if(special == 1) addDeviceToUser();
	else if(special == 3) screenFrame = 1;
	
	// Check card
	
	if (mfrc522.PICC_IsNewCardPresent()) { 
	
		cardPresent();
		pinPresent(4);
		
		scanSendAPI();
		lcd.clear();
		
	}
	
}

void resetDevice() {
	
	EEPROM.begin(512);
	
	for(int i = 0 ; i < 512 ; i++) EEPROM.write(i, 0);
	
	EEPROM.commit();
	
	EEPROM.end();
	
	ESP.restart();
	
}

void addDeviceToUser() {
	
	pin = random(0, 100000);
	
	HTTPClient http;

	http.begin("http://"+ipAdress+":3000/api/device/verify");
	http.addHeader("Content-Type", "application/json");

	int httpCode = http.POST("{\"deviceId\":"+String(uid)+",\"device_verify_pin\":"+String(pin)+"}");
	String payload = http.getString();

	http.end();

	if(httpCode == -1) ESP.restart();
	
	
	special = 0;
	lcd.clear();
	lcd.print("UID: "+String(uid));
	lcd.setCursor(0, 1);
	lcd.print("PIN: "+String(pin));
	
	delay(1000);
	
	while(Serial.available() > 0) char t = Serial.read();
	
	delay(30000);
	
	pinPresent(1);
	
	if(special == 2) resetDevice();
	
	while(Serial.available() > 0) char t = Serial.read();
	
	lcd.clear();
	animationTimer = 30;
	
}

void scanSendAPI() {
	
	HTTPClient http;

	http.begin("http://"+ipAdress+":3000/api/device/card/scan");
	http.addHeader("Content-Type", "application/json");

	int httpCode = http.POST("{\"deviceId\":"+String(uid)+",\"cardUid\":"+String(cardUID)+" ,\"cardPin\":"+String(pinCode)+"}");
	String payload = http.getString();

	http.end();

	if(httpCode == -1) ESP.restart();
	
	if(httpCode == 201) {
	
		Serial.println("");
		Serial.println("--- Send! ---");
		
		lcd.setCursor(0, 1);
		lcd.print("    Wys");
		lcd.write(5);
		lcd.print("ano!    ");
	
	}
	else {
		
		lcd.setCursor(0, 1);
		lcd.print("      B");
		lcd.write(5);
		lcd.write(6);
		lcd.print("d      ");
		
	}
	
	delay(300);
	
	lcd.clear();
	
}

void pinPresent(int ba) {
	
	if(ba != 1) lcd.clear();

	byte a = 0;
	pinCode = "";
	bool c = 0;
	
	if(ba != 1) while(Serial.available() > 0) char t = Serial.read();
	
	while(a < ba) {

		if(ba == 4) {
		
			lcd.setCursor(0, 0);
			lcd.print("   Podaj PIN:   ");
			lcd.setCursor(6, 1);
			
			for(int q = 0; q<a; q++) lcd.print("*");
			for(int w = 0; w<ba-a; w++) lcd.print("-");
		
		}
		
		c = 0;
	
		int code;
		
		if(ba != 1) while (Serial.available() == 0);
		
		code = Serial.read();
		
		if(code == 6 && twoByte == false) pinCode += "1";
		else if(code == 24 && twoByte == false) pinCode += "2";
		else if(code == 30 && twoByte == false) pinCode += "3";
		else if(code == 96 && twoByte == false) {
			
			if(ba != 4) {
				
				if(ba == 1) special = 1;
				
				pinCode += "10";
				c = 1;
			
			}
			
			twoByte = true;
			
		}
		else if(code == 102 && twoByte == false) pinCode += "4";
		else if(code == 120 && twoByte == false) pinCode += "5";
		else if(code == 126 && twoByte == false) pinCode += "6";
		else if(code == 128 && twoByte == false) {
			
			if(ba != 4) {
				
				if(ba == 1) special = 2;
				
				pinCode += "11";
				c = 1;
			
			}
			
			twoByte = true;
			
		}
		else if(code == 134 && twoByte == false) {
			
			pinCode += "7";
			twoByte = true;
			c = 1;
			
		}
		else if(code == 152 && twoByte == false) {
			
			pinCode += "8";
			twoByte = true;
			c = 1;
			
		}
		else if(code == 158 && twoByte == false) {
			
			pinCode += "9";
			twoByte = true;
			c = 1;
			
		}
		else if(code == 224 && twoByte == false) {
			
			if(ba != 4) {
			
				if(ba == 1) special = 3;
				
				pinCode += "12";
				c = 1;
				
			}
			
			twoByte = true;
			
		}
		else if(code == 230 && twoByte == false) {
			
			if(ba != 4) {
				
				pinCode += "13";
				c = 1;
			
			}
			
			twoByte = true;
			
		}
		else if(code == 248 && twoByte == false) {
			
			pinCode += "0";
			twoByte = true;
			c = 1;
			
		}
		else if(code == 254 && twoByte == false) {
			
			if(ba != 4) {
		
				pinCode += "14";
				c = 1;
			
			}
			
			twoByte = true;
			
		}
		else if(code == 0 && twoByte == false) {

			if(ba != 4) {
			
				if(ba == 1) special = 4;
				
				pinCode += "15";
				c = 1;
			
			}
			
			twoByte = true;
			
		
		}
		
		if(twoByte == true && c == 0) {
		
			twoByte = false;
			c = 1;
			
		} else {

			if(ba != 1) {
				
				tone(BEEP_PIN, 500, 200);
			
			}
			
			a++;
			
		}
	
	}
	
	if(ba != 1) {
	
		lcd.setCursor(6, 1);
		lcd.print("****");
		animationTimer = 30;
		
		delay(500);
	
	}
	
}

void cardPresent() {
	
	cardUID = "";
	
	tone(BEEP_PIN, 2500, 1000);

	if (mfrc522.PICC_ReadCardSerial())
	{
	
		for (byte i = 0; i < mfrc522.uid.size; i++) {
			
			cardUID += String(mfrc522.uid.uidByte[i], DEC);
			
			lcd.setCursor(4, 1);
			
			if(((i+1)*100)/mfrc522.uid.size >= 25) lcd.write(1);
			else lcd.write(0);
			
			lcd.setCursor(6 , 1);
			
			if(((i+1)*100)/mfrc522.uid.size >= 50) lcd.write(1);
			else lcd.write(0);
			
			lcd.setCursor(9, 1);
			
			if(((i+1)*100)/mfrc522.uid.size >= 75) lcd.write(1);
			else lcd.write(0);
			
			lcd.setCursor(11, 1);
			
			if(((i+1)*100)/mfrc522.uid.size >= 100) lcd.write(1);
			else lcd.write(0);

		}
		
		mfrc522.PICC_HaltA();
	
	}
	
	delay(500);
	
}

void loadEEPROM() {

	struct { 
	
		uint val = 0;
		char str[50] = "";
		
	} data;
  
	Serial.println("Loading information from eeprom...");
	
	lcd.setCursor(0, 1);
	lcd.print(" Loading eeprom ");
	
  
	// Get information about registered device from eeprom
  
	EEPROM.begin(512);
	
	EEPROM.get(0, ssid);
	
	EEPROM.get(0+sizeof(ssid), pass);
	
	EEPROM.get(0+sizeof(ssid)+sizeof(pass), uid);
	
	EEPROM.get(0+sizeof(ssid)+sizeof(pass)+sizeof(uid), pin);
	
	char controlParm[2+1];
	
	EEPROM.get(0+sizeof(ssid)+sizeof(pass)+sizeof(uid)+sizeof(pin), controlParm);
	
	EEPROM.end();
	
	if (String(controlParm) != String("OK")) {
		
		memset(ssid, 0, sizeof(ssid));
		memset(pass, 0, sizeof(pass));
		uid = 0;
		pin = 0;
		
		firstRegisterAPI();
		
	}

	Serial.println("Done!");
  
}

void loginDeviceAPI() {
	
	Serial.println("");
	Serial.println("--- Logging to server ---");
	
	lcd.setCursor(0, 1);
	lcd.print("   Logging in   ");

	HTTPClient http;

	http.begin("http://"+ipAdress+":3000/api/device/login");
	http.addHeader("Content-Type", "application/json");

	int httpCode = http.POST("{\"id\":"+String(uid)+", \"pin\":"+String(pin)+"}");
	String payload = http.getString();

	http.end();

	if(httpCode == -1) ESP.restart();
	
	if(httpCode == 200) {
	
		Serial.println("");
		Serial.println("--- Done! ---");
		
		lcd.setCursor(0, 1);
		lcd.print("     Signed     ");
	
	}
	else ESP.restart();
	
	delay(100);
	
	lcd.clear();
	
}

void firstRegisterAPI() {

	
	// First registration device on database

	Serial.println("");
	Serial.println("--- First registration ---");

	Serial.println("");
	Serial.println("WIFI SSID:");
	
	lcd.setCursor(0, 1);
	lcd.print("  Registration  ");
	
	while (Serial.available() == 0);
	
	Serial.readBytesUntil(10, ssid, 50);
	Serial.println(ssid);
	

	Serial.println("WIFI PASSWORD:");
	
	while (Serial.available() == 0);

	Serial.readBytesUntil(10, pass, 50);
	Serial.println(pass);
	
	
	// Connecting to wifi
	
	wifiConnect();
	
	
	// Generating PIN
	
	pin = random(0, 10000);
	
	
	// API connect

	HTTPClient http;

	http.begin("http://"+ipAdress+":3000/api/device/register");
	http.addHeader("Content-Type", "application/json");

	int httpCode = http.POST("{\"pin\":\""+String(pin)+"\"}");
	String payload = http.getString();

	http.end();

	if(httpCode == -1) ESP.restart();
	

	// Save information
	
	const size_t capacity = JSON_OBJECT_SIZE(2) + 60;
	
	DynamicJsonDocument doc(capacity);
	
	deserializeJson(doc, payload);
	
	uid = doc["id"];
	
	EEPROM.begin(512);
	
	EEPROM.put(0, ssid);
	
	EEPROM.put(0+sizeof(ssid), pass);
	
	EEPROM.put(0+sizeof(ssid)+sizeof(pass), uid);
	
	EEPROM.put(0+sizeof(ssid)+sizeof(pass)+sizeof(uid), pin);
	
	char controlParm[2+1] = "OK";
	
	EEPROM.put(0+sizeof(ssid)+sizeof(pass)+sizeof(uid)+sizeof(pin), controlParm);
	
	EEPROM.commit();
	
	EEPROM.end();
	
	Serial.println("--- Registered! ---");
	
	lcd.setCursor(0, 1);
	lcd.print("    Success!    ");
	
	delay(100);
	
	ESP.restart();
  
}

void wifiConnect() {


	// Connecting to wifi
	
	WiFi.disconnect();

	Serial.print("--- Connecting to ");
	Serial.print(ssid);
	Serial.println(" ---");
	
	lcd.setCursor(0, 1);
	lcd.print("  Connecting    ");
	
	byte timeout = 0;

	WiFi.begin(ssid, pass);
	
	
	//Waiting for connection
	
	while (WiFi.status() != WL_CONNECTED) {

		lcd.setCursor(13, 1);
		lcd.print("| ");
		
		delay(250);
		
		lcd.setCursor(13, 1);
		lcd.print("/ ");
		
		delay(250);
		
		lcd.setCursor(13, 1);
		lcd.print("- ");
		
		delay(250);
		
		lcd.setCursor(13, 1);
		lcd.write(2);
		lcd.print(" ");
		
		delay(250);
		

		Serial.print(".");
		timeout++;
		
		if(timeout > 20) ESP.restart();

	}

	Serial.println("");
	Serial.println("--- WiFi connected! ---");
	
	lcd.setCursor(0, 1);
	lcd.print("   Connected!   ");
	delay(100);
  
}
