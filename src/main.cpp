#include <NeoPixelBus.h>
#include <PubSubClient.h>
#include <WiFiManager.h>

#include "mqtt_settings.h"

const uint16_t PixelCount = 5;
const uint8_t PixelPin = 2;
const unsigned int colorSaturation = 128;
const unsigned long button_timeout = 1000;

typedef struct timed_callback {
  unsigned long time_ms;
  void (*callback)(void);
} timed_callback;

enum callback_indices {
  LED_OFF = 0,
  MAX_CALLBACKS,
};

timed_callback callback_list[MAX_CALLBACKS];

void handle_callbacks() {
  // Serial.println(__func__);
  for (int i = 0; i < MAX_CALLBACKS; i++) {
    // Serial.print("Callback "); Serial.println(i);
    if (callback_list[i].time_ms && callback_list[i].time_ms < millis()) {
      // Serial.print("time_ms "); Serial.println(callback_list[i].time_ms);
      callback_list[i].callback();
      callback_list[i].time_ms = 0;
    } else {
      ;
      // Serial.println("Empty");
    }
  }
}

char hostname[128];
WiFiClient espClient;
PubSubClient client(espClient);
NeoPixelBus<NeoGrbFeature, Neo800KbpsMethod> strip(PixelCount, PixelPin);

RgbColor red(colorSaturation, 0, 0);
RgbColor green(0, colorSaturation, 0);
RgbColor blue(0, 0, colorSaturation);
RgbColor white(colorSaturation);
RgbColor black(0);

bool button_active = true;

void init_hostname() {
  hostname[0] = '\0';
  strcat(hostname, hostname_base);
  strcat(hostname, WiFi.macAddress().c_str());
  Serial.print("hostname set to ");
  Serial.println(hostname);
}

void turn_led_off() {
  Serial.println(__func__);
  strip.SetPixelColor(2, black);
  strip.SetPixelColor(3, black);
  strip.SetPixelColor(4, black);
  strip.Show();
}

void mqtt_handle_message_coffee() {
  Serial.println(__func__);
  callback_list[LED_OFF].time_ms = millis() + 1000;
  callback_list[LED_OFF].callback = turn_led_off;
  strip.SetPixelColor(2, red);
  strip.SetPixelColor(3, red);
  strip.SetPixelColor(4, red);
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

  if (strcmp(message, mqtt_keyword) == 0) {
    mqtt_handle_message_coffee();
  }
}

void setup_wifi() {
  WiFi.setHostname(hostname);
  WiFiManager wifiManager;
  wifiManager.autoConnect(hostname);
  Serial.println("Connected to Wi-Fi");
}

void setup_leds() {
  Serial.println("Setting up LEDs");
  strip.Begin();
  strip.SetPixelColor(0, green);
  strip.Show();
}

void setup() {
  Serial.begin(115200);
  init_hostname();
  setup_leds();
  setup_wifi();
  strip.SetPixelColor(0, blue);
  strip.Show();
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(mqtt_callback);

  pinMode(0, INPUT);
  pinMode(35, INPUT);
}

unsigned long last = 0;

void loop() {
  /* Handle mqtt: Make sure it's still connected and serviced */
  if(!client.connected())
    mqtt_connect(&client, hostname);
  client.loop();

  /* handle programmed callbacks, eg led-off timers */
  handle_callbacks();

  if (!digitalRead(0)) {
    Serial.println("Sending a event!");
    client.publish(mqtt_channel, mqtt_keyword);
  }
}
