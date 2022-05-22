from dotenv import load_dotenv
import os
from influx_client import InfluxClient
import pandas as pd

load_dotenv("../.env")

INFLUX_HOST = os.getenv("INFLUX_HOST")
INFLUX_TOKEN = os.getenv("INFLUX_TOKEN")
INFLUX_BUCKET_NAME = os.getenv("INFLUX_BUCKET_NAME")
INFLUX_ORG = os.getenv("INFLUX_ORG")

influx_client = InfluxClient(INFLUX_ORG, INFLUX_BUCKET_NAME, INFLUX_TOKEN, host=INFLUX_HOST)

def create_query(start, stop, measurement, bucket):
    query = f'from(bucket:"{bucket}")' \
        f' |> range(start:-{start}h, stop: {stop}h)'\
        f' |> filter(fn: (r) => r["_measurement"] == "{measurement}")' \
        ' |> aggregateWindow(every: 5s, fn: mean, createEmpty: false)'
    return query

def create_raw(result):
    raw = []
    for table in result:
        for record in table.records:
            raw.append((record.get_value(), record.get_time()))
    return raw

def create_dataframe(raw):
    df=pd.DataFrame(raw, columns=['y','ds'], index=None)
    df['ds'] = df['ds'].values.astype('<M8[s]')
    df['ds'] +=  pd.to_timedelta(2, unit='h')
    df.set_index('ds')
    return df

def drop_duplicates(df):
    df.drop_duplicates(subset='ds', keep='last', inplace=True)
    df.set_index('ds', inplace=True)
    return df

def mse(df):
    return ((df['y'] - df['y_1'])**2).mean()

def compute_mse(start: int, stop: int, measurement: str) -> int:
    query_sensor = create_query(start, stop, measurement, "air-quality")
    query_forecast = create_query(start, stop, measurement, "air-quality-forecast")
    
    result_sensor = influx_client.query(query_sensor)
    result_forecast = influx_client.query(query_forecast)

    raw_sensor = create_raw(result_sensor)
    raw_forecast = create_raw(result_forecast)

    df_forecast = create_dataframe(raw_forecast)
    df_sensor = create_dataframe(raw_sensor)

    df_sensor = drop_duplicates(df_sensor)
    df_forecast = drop_duplicates(df_forecast)

    # Align the indexes of the two dataframes by using pandas join
    joined_df = df_sensor.join(df_forecast, how='outer', rsuffix='_1')

    # Drop columns in joined_df where y or y_1 is NaN
    joined_df.dropna(subset=['y', 'y_1'], inplace=True)
    joined_df.head()

    timseries_mse = mse(joined_df)
    return timseries_mse

def compute_all_mse():
    measurements = ["hum", "temp", "rss"]
    intervals = [24, 12, 6, 3, 1]

    for measurement in measurements:
        for interval in intervals:
            mse = compute_mse(interval, 0, measurement)
            print(f"{measurement} {interval}h: {mse}")
        print("\n---------------------------------\n")

if __name__ == "__main__":
    compute_all_mse()
    influx_client.disconnect()