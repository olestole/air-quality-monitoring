import paho.mqtt.client as mqtt
from influx_client import InfluxClient


class MQTTClient:
    def __init__(self, host, port=1883, username="mosquitto", password="mosquitto", keepalive=60, influx_client: InfluxClient = None):
        self.username = username
        self.password = password
        self.host = host
        self.port = port

        self.topics = {
            "temp": "sensor/temp",
            "hum": "sensor/hum",
            "rss": "sensor/rss",
            "aqi": "sensor/aqi",
            "board_id": "sensor/board_id",
            "gps": "sensor/gps",
        }

        self.influx_client = influx_client
        self.client = mqtt.Client()
        self.connect()

        # topics = [topic_temp, topic_hum, topic_rss, topic_aqi, topic_board_id, topic_gps]
        topics = "sensor/#"

    def on_connect(self, client, userdata, flags, rc):
        print("[MQTT] Connected with result code "+str(rc))
        topics = [(topic, 0) for topic in self.topics.values()]
        client.subscribe(topics)

    def on_message(self, client, userdata, msg):
        # TODO: Don't know what GPS is
        topics_float = [self.topics['temp'], self.topics['hum'], self.topics['aqi']]
        topics_int = [self.topics['board_id'], self.topics['rss']]

        topic: str = msg.topic
        top_level_topic: str = topic.split("/")[-1]
        payload = str(msg.payload.decode('utf-8'))

        if topic in topics_float:
            payload = float(payload)
        elif topic in topics_int:
            payload = int(float(payload))

        print(f"[RECEIVED MQTT]: {topic}: {payload}")
        # Write to InfluxDB
        self.influx_client.write_point(
            top_level_topic, {"user": "olestole"}, {'value': payload})

    def special_message(self, client, userdata, msg):
        msg = str(msg.payload.decode('utf-8'))
        print(f"[RECEIVED MQTT] special_message: {msg}")

    def connect(self):
        self.client.on_connect = self.on_connect
        self.client.on_message = self.on_message
        self.client.username_pw_set(
            username=self.username, password=self.password)
        self.client.connect(self.host, self.port, 60)

    def start_loop(self, blocking=True):
        if blocking:
            self.client.loop_forever()
        else:
            self.client.loop_start()

    def stop_loop(self):
        self.client.loop_stop()
