#include "WiFi.h"
#include <DHT.h>
#include <PubSubClient.h>
#include <string>

// --- CONSTANTS ---
#define PIN_DHT 12
#define DEFAULT_SENSE_FREQUENCY 2000

const char *SSID = "GUEST-FASTWEB-B37487";
const char *PASS = "zPxg9ax5nV";

const char *IP_MQTT_SERVER = "35.177.125.244";
const char *MQTT_USER = "mosquitto";
const char *MQTT_PASSWD = "mosquitto";
// const int MQTT_PORT = 1883;

const char *TOPIC_TEMP = "sensor/temp";
const char *TOPIC_HUM = "sensor/hum";
const char *TOPIC_RSS = "sensor/rss";
const char *TOPIC_AQI = "sensor/aqi";
const char *TOPIC_BOARD_ID = "sensor/board_id";
const char *TOPIC_GPS = "sensor/gps";

enum DATA_TYPE { TEMP = 1, HUM, RSS, AQI, BOARD_ID, GPS };

// --- STRUCTS ---

struct SensorData {
  float temp;
  float hum;
};

struct BoardData {
  uint32_t chipId;
  uint8_t cores;
  uint8_t chipRevision;
  std::string chipModel;
};

// --- VARIABLES ---

PubSubClient clientMQTT;
WiFiClient clientWiFi;

DHT dht(PIN_DHT, DHT22);

float tempValue;
float humValue;
boolean resultMQTT;
struct BoardData board_data;

// --- FUNCTIONS ---

void setup_mqtt() {
  clientMQTT.setClient(clientWiFi);
  clientMQTT.setServer(IP_MQTT_SERVER, 1883);
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

boolean publishData(DATA_TYPE data_type, float value) {
  bool result = false;
  boolean connected = clientMQTT.connected();

  if (!connected)
    connected = clientMQTT.connect("airQualitySensor", MQTT_USER, MQTT_PASSWD);
  if (connected) {
    String message = String(value);
    const char *payload = message.c_str();

    switch (data_type) {
    case TEMP:
      result = clientMQTT.publish(TOPIC_TEMP, payload);
      break;
    case HUM:
      result = clientMQTT.publish(TOPIC_HUM, payload);
      break;
    case RSS:
      result = clientMQTT.publish(TOPIC_RSS, payload);
      break;
    case AQI:
      result = clientMQTT.publish(TOPIC_AQI, payload);
      break;
    case BOARD_ID:
      result = clientMQTT.publish(TOPIC_BOARD_ID, payload);
      break;
    case GPS:
      result = clientMQTT.publish(TOPIC_GPS, payload);
      break;
    default:
      break;
    }

    log_publish_result(result, payload);
    clientMQTT.loop();
  }
  return (false);
}

struct SensorData read_sensor_data() {
  struct SensorData sensor_data;
  sensor_data.temp = dht.readTemperature();
  sensor_data.hum = dht.readHumidity();
  return sensor_data;
}

struct BoardData chip_info() {
  struct BoardData board_data;

  uint32_t chipId = 0;
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
    Serial.print("\n[LOG] Data published on the MQTT server: ");
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
  struct SensorData sensor_data = read_sensor_data();
  log_sensor_data(sensor_data);

  publishData(TEMP, sensor_data.temp);
  publishData(HUM, sensor_data.hum);
  publishData(BOARD_ID, board_data.chipId);

  delay(DEFAULT_SENSE_FREQUENCY);
}