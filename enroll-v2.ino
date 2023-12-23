#include <Adafruit_Fingerprint.h>
#ifdef ESP32
#include <WiFi.h>
#define BUILTIN_LED 2
#define Serial_FingerPrint Serial2
#else
#include <ESP8266WiFi.h>
#define RX_SW 14
#define TX_SW 12
#define BUILTIN_LED 16
constexpr SoftwareSerialConfig swSerialConfig = SWSERIAL_8N1;
SoftwareSerial mySerial(RX_SW, TX_SW);
#define Serial_FingerPrint Serial
#endif
#include <PubSubClient.h>

Adafruit_Fingerprint finger = Adafruit_Fingerprint(&Serial_FingerPrint);

const char* ssid = "MOC II";
const char* password = "99999999";
const char* mqtt_server = "mqtt.coder96.com";
const uint16_t port_mqtt = 1887;
const char* topic_pub = "sync-fingerprint/enroll/pub";
const char* topic_sub = "sync-fingerprint/enroll/sub";

WiFiClient espClient;
PubSubClient client(espClient);

volatile uint32_t lastMsg = 0;
volatile uint8_t id = 1;
volatile int p;
volatile uint8_t step = 0;

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
  Serial.begin(115200);
#ifdef ESP8266
  pinMode(RX_SW, INPUT);
  pinMode(TX_SW, OUTPUT);
  // Serial_FingerPrint.begin(57600, swSerialConfig, RX_SW, TX_SW, false, 2048, 11);
  // Serial_FingerPrint.enableIntTx(true);
  // Serial_FingerPrint.enableTxGPIOOpenDrain(true);
#else
  Serial_FingerPrint.setTxBufferSize(2048);
  Serial_FingerPrint.setRxBufferSize(2048);
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

  switch (step) {
    case 0:
      blinkLed(200);
      break;
    case 2:
      // blinkLed(300);
      break;
    case 3:
      blinkLed(700);
      break;
    case 4:
      // blinkLed(500);
      break;
    case 5:
      // blinkLed(700);
      break;
    case 6:
      // blinkLed(1000);
      break;
  }

  if (!client.connected()) {
    reconnect();
  } else {
    getFingerprintEnroll();
  }

}
//////////////////////////////////////////////////////////////////////////////////
void getFingerprintEnroll() {
  //------------------------------------------------
  if (step == 0) {
    p = -1;
    // Serial.print("Waiting for valid finger to enroll as #");
    // Serial.println(id);
    if (p != FINGERPRINT_OK) {
      p = finger.getImage();
      switch (p) {
        case FINGERPRINT_OK:
          step = 1;
          // Serial.println("\nImage taken");
          break;
        case FINGERPRINT_NOFINGER:
          // Serial.print(",");
          break;
        case FINGERPRINT_PACKETRECIEVEERR:
          // Serial.println("Communication error");
          break;
        case FINGERPRINT_IMAGEFAIL:
          // Serial.println("Imaging error");
          break;
        default:
          // Serial.println("Unknown error");
          break;
      }
    }
  }
  //------------------------------------------------
  if (step == 1) {
    p = finger.image2Tz(1);
    switch (p) {
      case FINGERPRINT_OK:
        step = 2;
        // Serial.println("Image converted");
        break;
      case FINGERPRINT_IMAGEMESS:
        step = 0;
        // Serial.println("Image too messy");
        break;
      case FINGERPRINT_PACKETRECIEVEERR:
        step = 0;
        // Serial.println("Communication error");
        break;
      case FINGERPRINT_FEATUREFAIL:
        step = 0;
        // Serial.println("Could not find fingerprint features");
        break;
      case FINGERPRINT_INVALIDIMAGE:
        step = 0;
        // Serial.println("Could not find fingerprint features");
        break;
      default:
        step = 0;
        // Serial.println("Unknown error");
        break;
    }
  }
  //------------------------------------------------
  if (step == 2) {
    // Serial.print("Remove finger ");
    delay(2000);
    p = 0;
    while (p != FINGERPRINT_NOFINGER) {
      p = finger.getImage();
    }
    step = 3;
  }
  //------------------------------------------------
  if (step == 3) {
    p = -1;
    // Serial.print("ID ");
    // Serial.println(id);
    // Serial.println("Place same finger again");
    if (p != FINGERPRINT_OK) {
      p = finger.getImage();
      switch (p) {
        case FINGERPRINT_OK:
          step = 4;
          // Serial.println("\nImage taken");
          break;
        case FINGERPRINT_NOFINGER:
          Serial.print(".");
          break;
        case FINGERPRINT_PACKETRECIEVEERR:
          // Serial.println("Communication error");
          break;
        case FINGERPRINT_IMAGEFAIL:
          // Serial.println("Imaging error");
          break;
        default:
          // Serial.println("Unknown error");
          break;
      }
    }
  }
  //------------------------------------------------
  if (step == 4) {
    p = finger.image2Tz(2);
    switch (p) {
      case FINGERPRINT_OK:
        step = 5;
        // Serial.println("Image converted");
        break;
      case FINGERPRINT_IMAGEMESS:
        step = 0;
        // Serial.println("Image too messy");
        break;
      case FINGERPRINT_PACKETRECIEVEERR:
        step = 0;
        // Serial.println("Communication error");
        break;
      case FINGERPRINT_FEATUREFAIL:
        step = 0;
        // Serial.println("Could not find fingerprint features");
        break;
      case FINGERPRINT_INVALIDIMAGE:
        step = 0;
        // Serial.println("Could not find fingerprint features");
        break;
      default:
        step = 0;
        // Serial.println("Unknown error");
        break;
    }
  }
  //------------------------------------------------
  if (step == 5) {
    // OK converted!
    // Serial.print("Creating model for #");
    // Serial.println(id);

    p = finger.createModel();
    if (p == FINGERPRINT_OK) {
      step = 6;
      p = 1;
      // Serial.println("Prints matched!");
    } else if (p == FINGERPRINT_PACKETRECIEVEERR) {
      step = 0;
      // Serial.println("Communication error");
    } else if (p == FINGERPRINT_ENROLLMISMATCH) {
      step = 0;
      // Serial.println("Fingerprints did not match");
    } else {
      step = 0;
      // Serial.println("Unknown error");
    }
  }

  if (step == 6) {
    uploadFingerprintTemplate();
    step = 0;
  }
}
//////////////////////////////////////////////////////////////////////////////////
void uploadFingerprintTemplate() {
  // Serial.print("==> Attempting to get Templete #");
  // Serial.println(id);
  p;
  p = finger.getModel();
  switch (p) {
    case FINGERPRINT_OK:
      // Serial.print("Template ");
      // Serial.print(id);
      // Serial.println(" transferring:");
      break;
    default:
      // Serial.print("Unknown error ");
      // Serial.println(p);
      return;
  }

  uint8_t bytesReceived[900];

  int i = 0;
  while (i <= 554) {
    if (Serial_FingerPrint.available()) {
      bytesReceived[i++] = Serial_FingerPrint.read();
    }
  }

  // Serial.println("Decoding packet...");

  // Filtering The Packet
  int a = 0, x = 3;

  // Serial.print("uint8_t packet2[] = {");
  String payload = "";

  for (int i = 10; i <= 554; ++i) {
    a++;
    if (a >= 129) {
      i += 10;
      a = 0;
      // payload += "\n";
      // Serial.println("};");
      // Serial.print("uint8_t packet");
      // Serial.print(x);
      // Serial.print("[] = {");
      x++;
    } else {
      // Serial.print("0x");
      // payload += "0x";
      //printHex(bytesReceived[i - 1], 2);
      payload += printHex(bytesReceived[i - 1], 2);
      // Serial.print(", ");
      // payload += ", ";
    }
  }

  // Serial.println("};");
  // Serial.println(payload);
  // Serial.println("COMPLETED\n");

  client.publish_P(topic_pub, payload.c_str(), false);
  // delay(1000);
}
//////////////////////////////////////////////////////////////////////////////////
String printHex(int num, int precision) {
  char tmp[16];
  char format[128];

  sprintf(format, "%%.%dX", precision);

  sprintf(tmp, format, num);
  // Serial.print(tmp);
  return String(tmp);
}
