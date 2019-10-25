#include "FastLED.h"
#include <ESP8266mDNS.h>
#include <ArduinoOTA.h>
#include <WiFiClient.h>

#include <SPI.h>
#include <MFRC522.h>

#define SS_PIN 15
#define RST_PIN 0
 
MFRC522 rfid(SS_PIN, RST_PIN); 
MFRC522::MIFARE_Key key; 


#define PIXELPIN 5
#define NUM_PIXELS 24
#define NUM_LEDS 24
CRGB leds[NUM_PIXELS];


#ifndef STASSID
#define STASSID "hal9k"
#define STAPSK  "some-password"
#endif

const char* ssid = STASSID;
const char* password = STAPSK;

void setup() {
    Serial.begin(115200);
    Serial.println("Booting");

    // tell FastLED about the LED strip configuration
    FastLED.addLeds<WS2811,PIXELPIN,GRB>(leds, NUM_PIXELS).setCorrection(TypicalLEDStrip);
    FastLED.setBrightness(50);
    
    // Do black when booting
    fill_solid(leds, NUM_PIXELS, CRGB::Black); 
    FastLED.show();

    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid);
    while (WiFi.waitForConnectResult() != WL_CONNECTED) {
        Serial.println("Connection Failed! Rebooting...");
        delay(5000);
        ESP.restart();
    }

    // Hostname defaults to esp8266-[ChipID]
    ArduinoOTA.setHostname("kokokollama");

    ArduinoOTA.onStart([]() {
        String type;
        if (ArduinoOTA.getCommand() == U_FLASH) {
            type = "sketch";
        } else { // U_SPIFFS
            type = "filesystem";
        }

        // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
        Serial.println("Start updating " + type);
    });
    ArduinoOTA.onEnd([]() {
        Serial.println("\nEnd");
    });
    ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
        Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
    });
    ArduinoOTA.onError([](ota_error_t error) {
        Serial.printf("Error[%u]: ", error);
        if (error == OTA_AUTH_ERROR) {
            Serial.println("Auth Failed");
        } else if (error == OTA_BEGIN_ERROR) {
            Serial.println("Begin Failed");
        } else if (error == OTA_CONNECT_ERROR) {
            Serial.println("Connect Failed");
        } else if (error == OTA_RECEIVE_ERROR) {
            Serial.println("Receive Failed");
        } else if (error == OTA_END_ERROR) {
            Serial.println("End Failed");
        }
    });
    ArduinoOTA.begin();
    Serial.println("Ready");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());

    // NFC reader init

  SPI.begin(); // Init SPI bus
  rfid.PCD_Init(); // Init MFRC522 

  for (byte i = 0; i < 6; i++) {
    key.keyByte[i] = 0xFF;
  }
  
}

uint8_t gHue = 0; // rotating "base color" used by many of the patterns
  
#define ARRAY_SIZE(A) (sizeof(A) / sizeof((A)[0]))

// Use WiFiClient class to create TCP connections
WiFiClient client;

void loop() {
  ArduinoOTA.handle();

  // Call the current pattern function once, updating the 'leds' array
  juggle();

  // send the 'leds' array out to the actual LED strip
  FastLED.show();  

  // insert a delay to keep the framerate modest
  FastLED.delay(1000/120); 

  // do some periodic updates
  EVERY_N_MILLISECONDS( 20 ) { gHue++; } // slowly cycle the "base color" through the rainbow
  
  handleConnection();

  handleNfc();
}

void handleConnection() {
  
  if (client.connected()) {
    return;
  }
  
  if (!client.connect("10.42.3.66", 1337)) {
    Serial.println("connection failed");
    Serial.println("wait 5 sec...");
    delay(5000);
    return;
  }

  // This will send the request to the server

  //read back one line from server
  /* Serial.println("receiving from remote server");
  String line = client.readStringUntil('\r');
  Serial.println(line);

  Serial.println("closing connection");
  client.stop(); */
}

void handleNfc() {

    // Reset the loop if no new card present on the sensor/reader. This saves the entire process when idle.
  if ( ! rfid.PICC_IsNewCardPresent())
    return;

  // Verify if the NUID has been readed
  if ( ! rfid.PICC_ReadCardSerial())
    return;

  printHex(rfid.uid.uidByte, rfid.uid.size);
  fill_solid(leds, NUM_PIXELS, CRGB::Red); 
  // Halt PICC
  rfid.PICC_HaltA();

  // Stop encryption on PCD
  rfid.PCD_StopCrypto1();
}

/**
 * Helper routine to dump a byte array as hex values to Serial. 
 */
void printHex(byte *buffer, byte bufferSize) {
  Serial.print(F("In hex: "));
  for (byte i = 0; i < bufferSize; i++) {
    Serial.print(buffer[i] < 0x10 ? " 0" : " ");
    Serial.print(buffer[i], HEX);
  }
  Serial.println();

  for (byte i = 0; i < bufferSize; i++) {
    client.print(buffer[i] < 0x10 ? " 0" : " ");
    client.print(buffer[i], HEX);
  }
  client.println();
}

void confetti() 
{
  // random colored speckles that blink in and fade smoothly
  fadeToBlackBy( leds, NUM_LEDS, 10);
  int pos = random16(NUM_LEDS);
  leds[pos] += CHSV( gHue + random8(64), 200, 255);
}

void sinelon()
{
  // a colored dot sweeping back and forth, with fading trails
  fadeToBlackBy( leds, NUM_LEDS, 20);
  int pos = beatsin16( 13, 0, NUM_LEDS-1 );
  leds[pos] += CHSV( gHue, 255, 192);
}

void juggle() {
  // eight colored dots, weaving in and out of sync with each other
  fadeToBlackBy( leds, NUM_LEDS, 20);
  byte dothue = 0;
  for( int i = 0; i < 2; i++) {
    leds[beatsin16( i+7, 0, NUM_LEDS-1 )] |= CHSV(dothue, 200, 255);
    dothue += 32;
  }
}
