#include "WiFi.h"
#include <DHT.h>
#include <PubSubClient.h>
#include <string>

// --- CONSTANTS ---
#define PIN_DHT 12
#define DEFAULT_SENSE_FREQUENCY 2000
#define DEFAULT_MIN_GAS 1
#define DEFAULT_MAX_GAS 5

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
const char *TOPIC_CHIP_ID = "sensor/chip_id";
const char *TOPIC_GPS = "sensor/gps";

const char *TOPIC_EXTERNAL = "external/#";
const char *TOPIC_FREQUENCY = "external/frequency";
const char *TOPIC_MIN_GAS = "external/min_gas";
const char *TOPIC_MAX_GAS = "external/max_gas";

enum DATA_TYPE { TEMP = 1, HUM, RSS, AQI, CHIP_ID, GPS };

// --- STRUCTS ---

struct SensorData {
  float temp;
  float hum;
};

struct BoardData {
  int chipId;
  int cores;
  int chipRevision;
  std::string chipModel;
};

// --- VARIABLES ---

WiFiClient clientWiFi;
PubSubClient clientMQTT(clientWiFi);
DHT dht(PIN_DHT, DHT22);
struct BoardData board_data;

int sample_frequency = DEFAULT_SENSE_FREQUENCY;
int min_gas = DEFAULT_MIN_GAS;
int max_gas = DEFAULT_MAX_GAS;
unsigned long time_now = 0;

// --- FUNCTIONS ---

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
  clientMQTT.setClient(clientWiFi);
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
  case CHIP_ID:
    result = clientMQTT.publish(TOPIC_CHIP_ID, payload);
    break;
  case GPS:
    result = clientMQTT.publish(TOPIC_GPS, payload);
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
    long rssi = read_rssi();

    if (clientMQTT.connected()) {
      publishData(TEMP, sensor_data.temp);
      publishData(HUM, sensor_data.hum);
      publishData(CHIP_ID, board_data.chipId);
      publishData(RSS, rssi);
    } else
      reconnect_mqtt();

    time_now += sample_frequency;
    Serial.println("-----------------------------------------------------");
  }

  clientMQTT.loop();
}