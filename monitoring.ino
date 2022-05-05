#include <DHT.h>
#include "WiFi.h"
#include <PubSubClient.h>

// --- CONSTANTS ---
#define PIN_DHT 12
#define DEFAULT_SENSE_FREQUENCY 2000

const char* SSID = "GUEST-FASTWEB-B37487";
const char* PASS = "zPxg9ax5nV";

const char* IP_MQTT_SERVER="130.136.2.70";
const char* MQTT_USER = "iot2020";
const char* MQTT_PASSWD = "mqtt2020*";

const char* TOPIC_0="sensor/temp";
const char* TOPIC_1="sensor/hum";

// --- VARIABLES ---

PubSubClient clientMQTT; 
WiFiClient clientWiFi;

DHT dht(PIN_DHT, DHT22);

float tempValue;
float humValue;
boolean resultMQTT;

// --- STRUCTS ---

struct SensorData {
  float temp;
  float hum;
};

// --- FUNCTIONS ---

void setup() {
  Serial.begin(115200);
  delay(100);
  dht.begin();
  delay(2000);
  connect_wifi();
  setup_mqtt();
}

void setup_mqtt() {
  clientMQTT.setClient(clientWiFi);
  clientMQTT.setServer(IP_MQTT_SERVER,1883);
  clientMQTT.setBufferSize(400);
  resultMQTT = false;
}

void connect_wifi() {
  WiFi.begin(SSID, PASS);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.println("Connection attempt");
    delay(500);
  }
  delay(5000);
  Serial.println("WiFi connected");
  Serial.println(WiFi.localIP());
}

boolean publishData(int channel, float value) {
  bool result = false;
  boolean connected = clientMQTT.connected();
  
  if (!connected) 
    connected = clientMQTT.connect("airQualitySensor", MQTT_USER, MQTT_PASSWD);
  if (connected) {
      String message = String(value);
      const char* payload = message.c_str();
      if (channel == 0) 
        result = clientMQTT.publish(TOPIC_0, payload);
      else
       result = clientMQTT.publish(TOPIC_1, payload);
      clientMQTT.loop();
      return result;
  } else return(false);  
}

struct SensorData read_sensor_data() {
  struct SensorData sensor_data;
  sensor_data.temp = dht.readTemperature();
  sensor_data.hum = dht.readHumidity();
  return sensor_data;
}

void log_sensor_data(struct SensorData sensor_data) {
  Serial.print("\n[LOG] Temperature: ");
  Serial.print(sensor_data.temp);
  delay(200);
  Serial.print("\n[LOG] Humidity: ");
  Serial.print(sensor_data.hum);
}

void loop() {
  struct SensorData sensor_data = read_sensor_data();
  log_sensor_data(sensor_data);

  resultMQTT = publishData(0,tempValue);
  resultMQTT = publishData(1, humValue);

  if (resultMQTT) 
    Serial.println("[LOG] Data temp published on the MQTT server");
  else
    Serial.println("[ERROR] MQTT connection failed");
  
  delay(DEFAULT_SENSE_FREQUENCY);
}