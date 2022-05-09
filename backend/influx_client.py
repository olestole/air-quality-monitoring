from influxdb_client import InfluxDBClient, Point, WriteOptions


class InfluxClient:
    def __init__(self, org, bucket, token, host="localhost", port=8086):
        self.host = host
        self.org = org
        self.bucket = bucket
        self.token = token
        self.port = port

        self.client = self.connect()
        self.write_api = self.client.write_api()
        self.query_api = self.client.query_api()

    def connect(self) -> InfluxDBClient:
        url = f"http://{self.host}:{self.port}"
        # Establish a connection
        client = InfluxDBClient(url=url, token=self.token, org=self.org)
        client.health()
        return client

    def write_point(self, measurement: str, tags, fields, write_options: WriteOptions = None):
        # measurement: str, tags: dict, fields: dict
        # Setting the id and GPS position as tags, and the rest of data as field values
        point = {
            "measurement": measurement,
            "tags": tags,
            "fields": fields,
        }

        point = Point.from_dict(point)
        self.write_api.write(bucket=self.bucket, org=self.org, record=point)

    def query(self, query: str):
        query1 = f'from(bucket: "{self.bucket}")\
        |> range(start: -10m)\
        |> filter(fn: (r) => r._measurement == "tempValue")\
        |> filter(fn: (r) => r._field == "value")\
        |> aggregateWindow(10m, fn: mean))'

        query2 = f'from(bucket: "{self.bucket}")\
        |> range(start: -10m)'

        # Return the table and print the result - NOT needed for the exercise
        result = self.client.query_api().query(org=self.org, query=query2)
        results = []
        for table in result:
            for record in table.records:
                results.append((record.get_value(), record.get_field()))
        print(results)
