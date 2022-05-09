from influx_client import InfluxClient
import os
from dotenv import load_dotenv
from flask import Flask, jsonify, request
import utils

load_dotenv(".env")

INFLUX_TOKEN = os.getenv("INFLUX_TOKEN")
INFLUX_BUCKET_NAME = "air-quality"
INFLUX_ORG = "olestole"
influx_client = InfluxClient(INFLUX_ORG, INFLUX_BUCKET_NAME, INFLUX_TOKEN)

app = Flask(__name__)


@app.route('/sensor/<chip_id>/<gps>', methods=['POST'])
def sensor_reading(chip_id, gps=None):
    data = request.get_json()

    values = {}
    for key, value in data.items():
        if key not in utils.SENSOR_DATA_TYPES:
            return jsonify({"Error": f"{key} is not a valid sensor data type"}), 400
        parsed_value = utils.parse_payload(key, value)
        values[key] = parsed_value
        print(f"[RECEIVED HTTP]: {key}: {parsed_value}")

    chip_id = values.get("chip_id") if values.get("chip_id") is not None else 0
    gps = values.get("gps") if values.get("gps") is not None else 0

    for topic, value in values.items():
        influx_client.write_point(topic, {"chip_id": chip_id, "gps": gps}, {'value': value})

    return data, 200

def run():
    app.run(debug=True, use_reloader=False)
