#include <NeoPixelBus.h>
#include <PubSubClient.h>
#include <WiFiManager.h>

#include "mqtt_settings.h"

const uint16_t PixelCount = 5;
const uint8_t PixelPin = 2;
const unsigned int colorSaturation = 128;

char hostname[128];
WiFiClient espClient;
PubSubClient client(espClient);
NeoPixelBus<NeoGrbFeature, Neo800KbpsMethod> strip(PixelCount, PixelPin);

RgbColor red(colorSaturation, 0, 0);
RgbColor green(0, colorSaturation, 0);
RgbColor blue(0, 0, colorSaturation);
RgbColor white(colorSaturation);
RgbColor black(0);

void init_hostname() {
  hostname[0] = '\0';
  strcat(hostname, hostname_base);
  strcat(hostname, WiFi.macAddress().c_str());
  Serial.print("hostname set to ");
  Serial.println(hostname);
}

void mqtt_handle_message_coffee() {
  strip.SetPixelColor(3, red);
  strip.Show();
}

void mqtt_connect(PubSubClient *client, char* hostname) {
  Serial.print("Connecting to MQTT broker as ");
  Serial.println(hostname);
  if (client->connect(hostname, mqtt_user, mqtt_password)) {
    Serial.println("MQTT client connected");
    client->subscribe(mqtt_channel);
  } else {
    Serial.print("MQTT connection failed, state=");
    Serial.println(client->state());
  }
}

void mqtt_callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  char message[length + 1];
  for (unsigned int i = 0; i < length; i++) {
    message[i] = (char)payload[i];
  }
  message[length] = '\0';
  Serial.println(message);

  if (strcmp(message, "coffee") == 0) {
    mqtt_handle_message_coffee();
  }
}

void setup_wifi() {
  WiFi.setHostname(hostname);
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

void setup() {
  Serial.begin(115200);
  init_hostname();
  setup_leds();
  setup_wifi();
  strip.SetPixelColor(1, blue);
  strip.Show();
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(mqtt_callback);
}

void loop() {
  /* Handle mqtt: Make sure it's still connected and serviced */
  if(!client.connected())
    mqtt_connect(&client, hostname);
  client.loop();
}
