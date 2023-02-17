#include <WiFiManager.h>
#include <PubSubClient.h>
#include <NeoPixelBus.h>

#include "mqtt_settings.h"

const uint16_t PixelCount = 5;
const uint8_t PixelPin = 2;

WiFiClient espClient;
PubSubClient client(espClient);
NeoPixelBus<NeoGrbFeature, Neo800KbpsMethod> strip(PixelCount, PixelPin);

#define colorSaturation 128
RgbColor red(colorSaturation, 0, 0);
RgbColor green(0, colorSaturation, 0);
RgbColor blue(0, 0, colorSaturation);
RgbColor white(colorSaturation);
RgbColor black(0);

enum {
  LED_STATUS = 0,
  LED_KAFFEE = 1
};

void setup_wifi() {
  WiFiManager wifiManager;
  wifiManager.autoConnect("CoffeeSignal");
  Serial.println("Connected to Wi-Fi");
}

void setup_leds() {
  Serial.println("Setting up LEDs");
  strip.Begin();
  strip.SetPixelColor(0, blue);
  strip.Show();
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  char message[length + 1];
  for (unsigned int i = 0; i < length; i++) {
    message[i] = (char)payload[i];
  }
  message[length] = '\0';
  Serial.println(message);
  
  if (strcmp(message, "coffee") == 0) { // Check for the keyword "coffee"
    strip.SetPixelColor(3, red);
    strip.Show(); // Send the updated color to the LED
  }
}

void setup() {
  Serial.begin(115200);
  setup_leds();
  setup_wifi();
  strip.SetPixelColor(1, blue);
  strip.Show();
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);
}

void loop() {
  if (!client.connected()) {
    char identifier[128] = "";
    strcat(identifier, mqtt_indentifier_base);
    strcat(identifier, WiFi.macAddress().c_str());
    Serial.print("Connecting to MQTT broker as ");
    Serial.println(identifier);
    if (client.connect(identifier, mqtt_user, mqtt_password)) {
      Serial.println("MQTT client connected");
      client.subscribe(mqtt_channel);
    } else {
      Serial.print("MQTT connection failed, state=");
      Serial.println(client.state());
    }
  }
  client.loop();
  strip.SetPixelColor(2, blue);
  strip.Show();
  delay(1000);
}