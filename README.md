# Air quality monitoring
Monorepo for all services in the IoT-pipeline of an air quality monitoring system. 

## About The Project

The project was implemented using an ESP32 with a DHT22 sensor to measure the temperature and humidity, along with WiFi-data and board-data. The data is transmitted through a MQTT broker to the backend, or directly to the backend using HTTP. The data is then stored on an InlfuxDB and visualized in Grafana.

![Grafana](https://github.com/olestole/air-quality-monitoring/blob/media/GrafanaScreenshot.png?raw=true)




## Getting Started

To get a local copy up and running follow these simple example steps.

### Prerequisites
* A microcontroller with a WiFi-module, e.g. an ESP32, and a DHT22.
* [docker](https://docs.docker.com/get-docker/)
* [docker-compose](https://docs.docker.com/compose/install/)

### Installation

1. Clone the repo
   ```sh
   git clone https://github.com/olestole/air-quality-monitoring.git
   ```
2. Edit the `.env`-files found in `/mosquitto` and `/backend`
  - `/mosquitto/.env`
    ```txt
    MOSQUITTO_USERNAME=
    MOSQUITTO_PASSWORD=
    MOSQUITTO_VERSION=
    ```
  - `/backend/.env`
    ```txt
    INFLUX_TOKEN=
    INFLUX_HOST=
    INFLUX_BUCKET_NAME=
    INFLUX_ORG=
    MQTT_HOST=
    MQTT_USERNAME=
    MQTT_PASSWORD=
    FLASK_PORT=
    ```
3. Run the services locally with `docker-compose`
    ```sh
    $ docker-compose up
    ```
4. Create a bucket in `InfluxDB` called `air-quality`
5. Obtain the `InlfuxDB Token` and add it to the .env-file, `INFLUX_TOKEN`
6. Run the services locally with `docker-compose`
    ```sh
    $ docker-compose up
    ```

