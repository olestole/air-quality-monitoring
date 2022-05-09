import paho.mqtt.client as mqtt
from influx_client import InfluxClient
import utils


class MQTTClient:
    def __init__(self, host, port=1883, username="mosquitto", password="mosquitto", keepalive=60, influx_client: InfluxClient = None):
        self.username = username
        self.password = password
        self.host = host
        self.port = port

        self.topics = ["temp", "hum", "rss", "aqi", "chip_id", "gps"]
        self.influx_client = influx_client
        self.client = mqtt.Client()
        self.connect()

    def on_connect(self, client, userdata, flags, rc):
        print("[MQTT] Connected with result code "+str(rc))
        topics = [(f"sensor/+/+/{topic}", 0) for topic in self.topics]
        client.subscribe(topics)

    def on_message(self, client, userdata, msg):
        topic: str = msg.topic
        split_topic = topic.split("/")
        sensor: str = split_topic[-1]
        gps: str = split_topic[-2]
        chip_id: str = split_topic[-3]
        payload = utils.parse_payload(sensor, str(msg.payload.decode('utf-8')))

        print(f"[RECEIVED MQTT - {chip_id} - {gps}]: {topic}: {payload}")
        # Write to InfluxDB
        self.influx_client.write_point(sensor, {"gps": gps, "chip_id": chip_id}, {'value': payload})

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
