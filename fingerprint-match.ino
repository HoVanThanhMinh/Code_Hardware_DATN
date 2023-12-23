#include "Adafruit_Fingerprint.h"
#ifdef ESP32
#include <WiFi.h>
#define BUILTIN_LED 2
#define Serial_FingerPrint Serial2
#define BUZZER_PIN 23
#else
#include <ESP8266WiFi.h>
#define RX_SW 14
#define TX_SW 12
#define BUILTIN_LED 16
#define BUZZER_PIN 13
constexpr SoftwareSerialConfig swSerialConfig = SWSERIAL_8N1;
SoftwareSerial mySerial(RX_SW, TX_SW);
#define Serial_FingerPrint mySerial
#endif
#include <PubSubClient.h>
#include <ArduinoJson.h>

Adafruit_Fingerprint finger = Adafruit_Fingerprint(&Serial_FingerPrint);

const char* ssid = "MOC II";
const char* password = "99999999";
const char* mqtt_server = "mqtt.coder96.com";
const uint16_t port_mqtt = 1887;
const char* topic_sub = "sync-fingerprint/match/sub";
const char* topic_pub = "sync-fingerprint/match/pub";

WiFiClient espClient;
PubSubClient client(espClient);

volatile uint32_t lastMsg = 0;
volatile uint8_t id = 1;
volatile int p;
volatile uint8_t step = 0;
volatile uint8_t match_step = 0;

//////////////////////////////////////////////////////////////////////////////////
void buzzerError() {
  tone(BUZZER_PIN, 1000, 700);
  noTone(BUZZER_PIN);
}
//////////////////////////////////////////////////////////////////////////////////
void buzzerSuccess() {
  tone(BUZZER_PIN, 30000, 100);
  noTone(BUZZER_PIN);
  delay(200);
  tone(BUZZER_PIN, 30000, 100);
  noTone(BUZZER_PIN);
}
//////////////////////////////////////////////////////////////////////////////////
void buzzerSyncSuccess() {
  tone(BUZZER_PIN, 30000, 100);
  noTone(BUZZER_PIN);
  delay(200);
  tone(BUZZER_PIN, 30000, 100);
  noTone(BUZZER_PIN);
  delay(200);
  tone(BUZZER_PIN, 30000, 100);
  noTone(BUZZER_PIN);
}
//////////////////////////////////////////////////////////////////////////////////
void setup_wifi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }
}
//////////////////////////////////////////////////////////////////////////////////
void callback(char* topic, byte* payload, unsigned int length) {
  String json = "";
  for (int i = 0; i < length; i++) {
    json += (char)payload[i];
  }
  StaticJsonDocument<2048> doc;
  DeserializationError error = deserializeJson(doc, json);
  if (error) {
    Serial.print(F("deserializeJson() failed: "));
    Serial.println(error.f_str());
    return;
  }
  Serial.println(json);
  String command = doc["command"];
  id = doc["id"];
  if (command == "sync") {
    String temp = doc["template"];
    downloadFingerprintTemplate(temp);
  }
  if (command == "delete") {
    deleteFingerprint(id);
  }

  if (command == "empty") {
    finger.emptyDatabase();
  }
}
//////////////////////////////////////////////////////////////////////////////////
uint8_t deleteFingerprint(uint8_t id) {
  p = -1;
  p = finger.deleteModel(id);
  if (p == FINGERPRINT_OK) {
    Serial.println("==> Deleted!\n");
  } else if (p == FINGERPRINT_PACKETRECIEVEERR) {
    Serial.println("Communication error");
    return p;
  } else if (p == FINGERPRINT_BADLOCATION) {
    Serial.println("Could not delete in that location");
    return p;
  } else if (p == FINGERPRINT_FLASHERR) {
    Serial.println("Error writing to flash");
    return p;
  } else {
    Serial.print("Unknown error: 0x");
    Serial.println(p, HEX);
    return p;
  }
  return 0;
}
//////////////////////////////////////////////////////////////////////////////////
void downloadFingerprintTemplate(String temp) {
  // Chuyển đổi dữ liệu vân tay đã lưu trữ sang dạng byte array
  uint16_t templateSize = temp.length() / 2;
  uint8_t templateData[templateSize];
  for (uint16_t i = 0; i < templateSize; i++) {
    String hexByte = temp.substring(i * 2, i * 2 + 2);
    templateData[i] = strtoul(hexByte.c_str(), NULL, 16);
  }

  uint8_t packet2[128];
  uint8_t packet3[128];
  uint8_t packet4[128];
  uint8_t packet5[128];

  for (int i = 0; i < templateSize; i++) {
    if (i < 128) {
      packet2[i] = templateData[i];
    }
    if (i >= 128 && i < 128 * 2) {
      packet3[i - 128] = templateData[i];
    }
    if (i >= 128 * 2 && i < 128 * 3) {
      packet4[i - 128 * 2] = templateData[i];
    }
    if (i >= 128 * 3) {
      packet5[i - 128 * 3] = templateData[i];
    }
  }


  p = finger.uploadModelPercobaan(packet2, packet3, packet4, packet5);  // Simpan di Char Buffer 01
  switch (p) {
    case FINGERPRINT_OK:
      Serial.print("Upload Templete loaded");
      break;
    case FINGERPRINT_PACKETRECIEVEERR:
      Serial.println("Communication error");
      break;
    case FINGERPRINT_BADPACKET:
      Serial.println("Bad packet");
      break;
    default:
      Serial.println("Unknown error uploadModel");
      Serial.println(p);
      break;
  }

  Serial.print("\n==>[SUKSES] StoreModel + ID = ");
  Serial.print(id);

  p = finger.storeModel(id);  // taruh di ID = 0 pada flash memory FP
  if (p == FINGERPRINT_OK) {
    buzzerSyncSuccess();
    Serial.println("Stored!");
  } else if (p == FINGERPRINT_PACKETRECIEVEERR) {
    Serial.println("Communication error");
  } else if (p == FINGERPRINT_BADLOCATION) {
    Serial.println("Could not store in that location");
  } else if (p == FINGERPRINT_FLASHERR) {
    Serial.println("Error writing to flash");
  } else {
    Serial.println("Unknown error");
  }
}
//////////////////////////////////////////////////////////////////////////////////
void printHex(int num, int precision) {
  char tmp[16];
  char format[128];

  sprintf(format, "%%.%dX", precision);

  sprintf(tmp, format, num);
  Serial.print(tmp);
}
//////////////////////////////////////////////////////////////////////////////////
void fingerMatch() {
  static uint32_t lastime_finger_match = 0;
  if (millis() - lastime_finger_match > 200) {
    lastime_finger_match = millis();
    p = finger.getImage();
    switch (p) {
      case FINGERPRINT_OK:
        match_step = 1;
        Serial.println("\nImage taken");
        break;
      case FINGERPRINT_NOFINGER:
        Serial.print(".");
        break;
      case FINGERPRINT_PACKETRECIEVEERR:
        Serial.println("Communication error");
        break;
      case FINGERPRINT_IMAGEFAIL:
        Serial.println("Imaging error");
        break;
      default:
        Serial.println("Unknown error");
        break;
    }
    if (match_step == 1) {
      getFingerprintID();
      match_step = 0;
    }
  }
}
//////////////////////////////////////////////////////////////////////////////////
uint8_t getFingerprintID() {
  p = finger.image2Tz();
  switch (p) {
    case FINGERPRINT_OK:
      Serial.println("Image converted");
      break;
    case FINGERPRINT_IMAGEMESS:
      Serial.println("Image too messy");
      return p;
    case FINGERPRINT_PACKETRECIEVEERR:
      Serial.println("Communication error");
      return p;
    case FINGERPRINT_FEATUREFAIL:
      Serial.println("Could not find fingerprint features");
      return p;
    case FINGERPRINT_INVALIDIMAGE:
      Serial.println("Could not find fingerprint features");
      return p;
    default:
      Serial.println("Unknown error");
      return p;
  }

  // OK converted!
  Serial.print("==> ");
  p = finger.fingerFastSearch();
  if (p == FINGERPRINT_OK) {
    buzzerSuccess();
    Serial.println("Found a print match!");
  } else if (p == FINGERPRINT_PACKETRECIEVEERR) {
    buzzerError();
    Serial.println("Communication error");
    return p;
  } else if (p == FINGERPRINT_NOTFOUND) {
    buzzerError();
    Serial.println("Did not find a match");
    return p;
  } else {
    buzzerError();
    Serial.println("Unknown error");
    return p;
  }

  // found a match!
  Serial.print("Found ID #");
  Serial.print(finger.fingerID);
  Serial.print(" with confidence of ");
  Serial.println(finger.confidence);

  if (client.connected()) {
    client.publish(topic_pub, String(finger.fingerID).c_str());
  }

  delay(1000);
  return finger.fingerID;
}
//////////////////////////////////////////////////////////////////////////////////
void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    String clientId = "ESP8266Client-";
    clientId += String(random(0, 1000));
    if (client.connect(clientId.c_str())) {
      client.subscribe(topic_sub);
    } else {
      delay(5000);
    }
  }
}
//////////////////////////////////////////////////////////////////////////////////
void blinkLed(uint16_t interval) {
  static uint32_t last_blink = 0;
  static bool state_led = false;
  if (millis() - last_blink > interval) {
    last_blink = millis();
    state_led = !state_led;
    digitalWrite(BUILTIN_LED, state_led);
  }
}
//////////////////////////////////////////////////////////////////////////////////
void setup() {
  pinMode(BUILTIN_LED, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);

  Serial.begin(9600);
#ifdef ESP8266
  pinMode(RX_SW, INPUT);
  pinMode(TX_SW, OUTPUT);
  Serial_FingerPrint.begin(57600, swSerialConfig, RX_SW, TX_SW, false, 1024);
#endif
  finger.begin(57600);

  if (finger.verifyPassword()) {
    // Serial.println("Found fingerprint sensor!");
  } else {
    // Serial.println("Did not find fingerprint sensor :(");
    while (1) { delay(1); }
  }

  setup_wifi();
  client.setServer(mqtt_server, port_mqtt);
  client.setCallback(callback);
  client.setBufferSize(2048);
}
//////////////////////////////////////////////////////////////////////////////////
void loop() {
  client.loop();

  if (!client.connected()) {
    reconnect();
  } else {
    fingerMatch();
  }
}
