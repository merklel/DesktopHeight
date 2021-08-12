#include <ESP8266WiFi.h>
#include "Adafruit_MQTT.h"
#include "Adafruit_MQTT_Client.h"
#include <ArduinoJson.h>
#include <Adafruit_Sensor.h>
#include "secrets.h"

#define MQTT_BROKER     S_MQTT_BROKER
#define MQTT_PORT       S_MQTT_PORT

#ifndef STASSID
#define STASSID S_STASSID
#define STAPSK  S_STAPSK
#endif

#define PIN_TRIGGER 16
#define PIN_ECHO 5

const char* ssid     = STASSID;
const char* password = STAPSK;

const int SENSOR_MAX_RANGE = 300; // in cm
float duration;
float distance;

WiFiClient client;
Adafruit_MQTT_Client mqtt(&client, MQTT_BROKER, MQTT_PORT, "", "");

Adafruit_MQTT_Publish topic = Adafruit_MQTT_Publish(&mqtt, "" "v1/devices/height_1");

float read_dist_sensor(){
  digitalWrite(PIN_TRIGGER, LOW);
  delayMicroseconds(2);

  digitalWrite(PIN_TRIGGER, HIGH);
  delayMicroseconds(10);

  duration = pulseIn(PIN_ECHO, HIGH);
  distance = duration/58;

  if (distance > SENSOR_MAX_RANGE || distance <= 0){
    Serial.print("Out of sensor range! Dist: ");
    Serial.println(distance);
  } else {
    Serial.println("Distance to object: " + String(distance) + " cm");
  }

  return distance;
}

void MQTT_connect() {
  int8_t ret;

  // Stop if already connected.
  if (mqtt.connected()) {
    return;
  }

  Serial.print("Connecting to MQTT... ");

  uint8_t retries = 3;
  while ((ret = mqtt.connect()) != 0) { // connect will return 0 for connected
       Serial.println(mqtt.connectErrorString(ret));
       Serial.println("Retrying MQTT connection in 5 seconds...");
       mqtt.disconnect();
       delay(3000);  // wait 5 seconds
       retries--;
       if (retries == 0) {
         // basically die and wait for WDT to reset me
         while (1);
       }
  }
  Serial.println("MQTT Connected!");
}

void setup() {
  Serial.begin(115200);

  pinMode(PIN_TRIGGER, OUTPUT);
  pinMode(PIN_ECHO, INPUT);

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("WiFi connected");
  Serial.println("IP address: "); Serial.println(WiFi.localIP());
}


void loop() {

  MQTT_connect();

  // N_READINGS readings and average them to counter zeros
  int N_READINGS = 5;
  float distance = 0;
  int counter=0;
  while(counter < N_READINGS){

    int reading = read_dist_sensor();
   delay(10);

    if (reading > 0){
      distance += reading;
      counter +=1;
    }    
  }
  distance = distance / N_READINGS;

  

  DynamicJsonDocument message(1024);
  message["distance"]   = distance;
  char message_serialized[100];
  serializeJson(message, message_serialized);

  if (! topic.publish(message_serialized)) {
    Serial.println(F("Failed"));
  } else {
    Serial.println(F("mqtt publish ok!"));
  }

  delay(10000);

}
