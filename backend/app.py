from mqtt_client import MQTTClient
import time
import sys
import flask_api


try:
    mqtt_client = MQTTClient("35.177.125.244", 1883, "mosquitto", "mosquitto")
    mqtt_client.connect()
    mqtt_client.start_loop(blocking=False)

    flask_api.run()

except (KeyboardInterrupt, SystemExit):
    mqtt_client.stop_loop()
    sys.exit()
