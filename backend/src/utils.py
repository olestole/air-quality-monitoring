SENSOR_DATA_TYPES = [
    "temp",
    "hum",
    "rss",
    "aqi",
    "chip_id",
    "gps"
]


def parse_payload(topic, payload):
    topics = {
        "temp": "sensor/temp",
        "hum": "sensor/hum",
        "rss": "sensor/rss",
        "aqi": "sensor/aqi",
        "chip_id": "sensor/chip_id",
        "gps": "sensor/gps",
    }

    # TODO: Don't know what GPS is
    topics_float = ['temp', 'hum']
    topics_int = ['chip_id', 'rss', 'aqi']

    if topic in topics_float:
        payload = float(payload)
    elif topic in topics_int:
        payload = int(float(payload))

    return payload
