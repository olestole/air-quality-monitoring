import paho.mqtt.client as mqtt


class MQTTClient:
    def __init__(self, host, port=1883, username="mosquitto", password="mosquitto", keepalive=60):
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

        self.client = mqtt.Client()
        self.connect()

        # topics = [topic_temp, topic_hum, topic_rss, topic_aqi, topic_board_id, topic_gps]
        topics = "sensor/#"

    def on_connect(self, client, userdata, flags, rc):
        print("Connected with result code "+str(rc))
        topics = [(topic, 0) for topic in self.topics.values()]
        print(topics)
        client.subscribe(topics)

    # TODO: Take these values and write them to InfluxDB
    def on_message(self, client, userdata, msg):
        topic = msg.topic
        payload = str(msg.payload.decode('utf-8'))

        if topic == self.topics['board_id']:
            print(f"{topic}: {int(float(payload))}")
        else:
            print(f"{topic}: {payload}")

    def special_message(self, client, userdata, msg):
        msg = str(msg.payload.decode('utf-8'))
        print(f"special_message: {msg}")

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
