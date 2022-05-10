#include "WiFi.h"
#include <Arduino_JSON.h>
#include <DHT.h>
#include <HTTPClient.h>
#include <PubSubClient.h>
#include <string>

// --- CONSTANTS ---
#define PIN_DHT 12
#define DEFAULT_SENSE_FREQUENCY 2000
#define DEFAULT_MIN_GAS 1
#define DEFAULT_MAX_GAS 5

const char *SSID = "GUEST-FASTWEB-B37487";
const char *PASS = "zPxg9ax5nV";
const char *BACKEND_SERVER = "http://192.168.78.108:3000";

const char *IP_MQTT_SERVER = "35.177.125.244";
const char *MQTT_USER = "mosquitto";
const char *MQTT_PASSWD = "mosquitto";
// const int MQTT_PORT = 1883;

const char *TOPIC_TEMP = "temp";
const char *TOPIC_HUM = "hum";
const char *TOPIC_RSS = "rss";
const char *TOPIC_AQI = "aqi";
const char *TOPIC_CHIP_ID = "chip_id";
const char *TOPIC_GPS = "gps";

const char *TOPIC_EXTERNAL = "external/#";
const char *TOPIC_FREQUENCY = "external/frequency";
const char *TOPIC_MIN_GAS = "external/min_gas";
const char *TOPIC_MAX_GAS = "external/max_gas";

enum DATA_TYPE { TEMP = 1, HUM, RSS, AQI, CHIP_ID, GPS };

// --- STRUCTS ---

struct SensorData {
  float temp;
  float hum;
  float aqi;
};

struct BoardData {
  int chipId;
  int cores;
  int chipRevision;
  std::string chipModel;
};

// --- VARIABLES ---

HTTPClient http_client;
WiFiClient wifi_client;
PubSubClient clientMQTT(wifi_client);
DHT dht(PIN_DHT, DHT22);
struct BoardData board_data;

int sample_frequency = DEFAULT_SENSE_FREQUENCY;
int min_gas = DEFAULT_MIN_GAS;
int max_gas = DEFAULT_MAX_GAS;
unsigned long time_now = 0;

// --- FUNCTIONS ---

// BUG: There're a lot of bugs with this implementation.
void publish_http(SensorData sensor_data, int rss, int chip_id, int gps) {
  std::string serverPath;
  serverPath += std::string(BACKEND_SERVER) + "/" + std::to_string(chip_id) +
                "/" + std::to_string(gps);

  http_client.begin(wifi_client, serverPath.c_str());
  http_client.addHeader("Content-Type", "application/json");

  std::string json = "{";
  json += "\"temp\":" + std::to_string(sensor_data.temp) + ",";
  json += "\"hum\":" + std::to_string(sensor_data.hum) + ",";
  json += "\"rss\":" + std::to_string(rss) + ",";
  // json += "\"aqi\":" + std::to_string(sensor_data.aqi) + ",";
  json += "\"chip_id\":" + std::to_string(board_data.chipId) + ",";
  // json += "\"cores\":" + std::to_string(board_data.cores) + ",";
  // json += "\"chip_revision\":" + std::to_string(board_data.chipRevision) + ",";
  // json += "\"chip_model\":" + board_data.chipModel;
  json += "}";

  int httpResponseCode = http_client.POST("{\"temp\":\"12.34\",\"hum\":\"54.4\",\"rss\":\"-61\",\"chip_id\":\"1294782\"}");
  Serial.print("HTTP Response code: ");
  Serial.println(httpResponseCode);

  http_client.end();
}

void receive_callback(char *topic, byte *payload, unsigned int length) {
  String messageTemp;

  Serial.print("[RECEIVED][");
  Serial.print(topic);
  Serial.print("]: ");

  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
    messageTemp += (char)payload[i];
  }

  if (strcmp(topic, TOPIC_FREQUENCY) == 0) {
    sample_frequency = messageTemp.toInt();
    Serial.print("\nFrequency: ");
    Serial.println(sample_frequency);
  } else if (strcmp(topic, TOPIC_MIN_GAS) == 0) {
    min_gas = messageTemp.toInt();
    Serial.print("\nMin gas: ");
    Serial.println(min_gas);
  } else if (strcmp(topic, TOPIC_MAX_GAS) == 0) {
    max_gas = messageTemp.toInt();
    Serial.print("\nMax gas: ");
    Serial.println(max_gas);
  }
}

// TODO: Guarantee this subscription
// TODO: Add gener
void subscribe_to_topics() {
  boolean subscripe_res = clientMQTT.subscribe(TOPIC_EXTERNAL, 1);
  if (!subscripe_res) {
    Serial.print("Failed to subscribe to topic: ");
    Serial.println(TOPIC_EXTERNAL);
  }
}

void setup_mqtt() {
  clientMQTT.setClient(wifi_client);
  clientMQTT.setServer(IP_MQTT_SERVER, 1883);
  clientMQTT.setBufferSize(400);
  clientMQTT.setCallback(receive_callback);
  clientMQTT.connect("airQualitySensor", MQTT_USER, MQTT_PASSWD);

  subscribe_to_topics();
}

void reconnect_mqtt() {
  Serial.println("[MQTT] Reconnecting...");
  while (!clientMQTT.connected()) {
    if (clientMQTT.connect("airQualitySensor", MQTT_USER, MQTT_PASSWD)) {
      Serial.println("[MQTT] Connected!");
      subscribe_to_topics();
    } else {
      Serial.println("[MQTT] Failed to connect!");
      delay(2000);
    }
  }
}

const char *create_topic(std::string sensor, int chip_id, int gps) {
  std::string topic;
  topic += std::string("sensor/") + std::to_string(chip_id) + "/" +
           std::to_string(gps) + "/" + sensor;
  Serial.println(topic.c_str());
  return topic.c_str();
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

boolean publishData(DATA_TYPE data_type, float value, int chip_id, int gps) {
  bool result = false;

  String message = String(value);
  const char *payload = message.c_str();

  switch (data_type) {
  // TODO: Pass the const TOPIC and not the string
  case TEMP:
    result = clientMQTT.publish(create_topic("temp", chip_id, gps), payload);
    break;
  case HUM:
    result = clientMQTT.publish(create_topic("hum", chip_id, gps), payload);
    break;
  case RSS:
    result = clientMQTT.publish(create_topic("rss", chip_id, gps), payload);
    break;
  case AQI:
    result = clientMQTT.publish(create_topic("aqi", chip_id, gps), payload);
    break;
  case CHIP_ID:
    result = clientMQTT.publish(create_topic("chip_id", chip_id, gps), payload);
    break;
  case GPS:
    result = clientMQTT.publish(create_topic("gps", chip_id, gps), payload);
    break;
  default:
    break;
  }

  log_publish_result(result, payload);
  clientMQTT.loop();
  return result;
}

struct SensorData read_sensor_data() {
  struct SensorData sensor_data;
  sensor_data.temp = dht.readTemperature();
  sensor_data.hum = dht.readHumidity();
  return sensor_data;
}

long read_rssi() {
  // The current Received Signal Strength in dBm (RSSI)
  return WiFi.RSSI();
}

struct BoardData chip_info() {
  struct BoardData board_data;

  int chipId = 0;
  for (int i = 0; i < 17; i = i + 8) {
    chipId |= ((ESP.getEfuseMac() >> (40 - i)) & 0xff) << i;
  }

  board_data.chipId = chipId;
  board_data.cores = ESP.getChipCores();
  board_data.chipModel = ESP.getChipModel();
  board_data.chipRevision = ESP.getChipRevision();

  return board_data;
}

void log_board_data(struct BoardData board_data) {
  Serial.printf("ESP32 Chip model = %s Rev %d\n", board_data.chipModel,
                board_data.chipRevision);
  Serial.printf("This chip has %d cores\n", board_data.cores);
  Serial.print("Chip ID: ");
  Serial.println(board_data.chipId);
}

void log_publish_result(boolean result, char const *payload) {
  if (result) {
    Serial.print("[LOG] Data published on the MQTT server: ");
    Serial.println(payload);
  } else
    Serial.println("\n[ERROR] MQTT connection failed");
}

void log_sensor_data(struct SensorData sensor_data) {
  Serial.print("\n[LOG] Temperature: ");
  Serial.print(sensor_data.temp);
  delay(200);
  Serial.print("\n[LOG] Humidity: ");
  Serial.print(sensor_data.hum);
}

void setup() {
  Serial.begin(115200);
  delay(100);
  dht.begin();
  delay(2000);
  connect_wifi();
  setup_mqtt();
  board_data = chip_info();
  log_board_data(board_data);
}

void loop() {
  if (millis() >= time_now + sample_frequency) {
    struct SensorData sensor_data = read_sensor_data();
    struct BoardData board_data = chip_info();
    long rssi = read_rssi();

    if (clientMQTT.connected()) {
      publishData(TEMP, sensor_data.temp, board_data.chipId, 987);
      publishData(HUM, sensor_data.hum, board_data.chipId, 987);
      publishData(CHIP_ID, board_data.chipId, board_data.chipId, 987);
      publishData(RSS, rssi, board_data.chipId, 987);

      // publish_http(sensor_data, rssi, board_data.chipId, 987);
    } else
      reconnect_mqtt();

    time_now += sample_frequency;
    Serial.println("-----------------------------------------------------");
  }

  clientMQTT.loop();
}