from dotenv import load_dotenv
import os
from influx_client import InfluxClient
from mqtt_client import MQTTClient
import sys
import flask_api
from forecasting import Forecasting

load_dotenv("../.env")

INFLUX_HOST = os.getenv("INFLUX_HOST")
INFLUX_TOKEN = os.getenv("INFLUX_TOKEN")
INFLUX_BUCKET_NAME = os.getenv("INFLUX_BUCKET_NAME")
INFLUX_ORG = os.getenv("INFLUX_ORG")
MQTT_HOST = os.getenv("MQTT_HOST")
MQTT_USERNAME = os.getenv("MQTT_USERNAME")
MQTT_PASSWORD = os.getenv("MQTT_PASSWORD")
MQTT_PORT = 1883

influx_client = InfluxClient(INFLUX_ORG, INFLUX_BUCKET_NAME, INFLUX_TOKEN, host=INFLUX_HOST)
mqtt_client = MQTTClient(MQTT_HOST, MQTT_PORT, MQTT_USERNAME, MQTT_PASSWORD, influx_client=influx_client)
forecasting_client = Forecasting(influx_client)

def run_http_api():
    print("[HTTP API] Starting...")
    flask_api.run()


def run_mqtt(mqtt_client):
    print("[MQTT] Starting...")
    mqtt_client.connect()
    mqtt_client.start_loop(blocking=False)

try:
    forecasting_client.start()
    run_mqtt(mqtt_client)
    run_http_api()

except (KeyboardInterrupt, SystemExit):
    print("\nExiting...")
    mqtt_client.stop_loop()

finally:
    print("Exiting...")
    mqtt_client.stop_loop()
    forecasting_client.stop()
    sys.exit()