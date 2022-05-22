from influx_client import InfluxClient
import schedule
import time
import threading
from datetime import datetime
import pandas as pd
from prophet import Prophet

class Forecasting:
    def __init__(self, influx_client: InfluxClient, interval: int = 1800, prediction_periods: int = 48, prediction_freq: str = "15T", forecast_bucket: str = "air-quality-forecast") -> None:
        self.influx_client = influx_client
        self.stop_run_continuously = None
        self.interval = interval
        self.prediction_periods = prediction_periods
        self.forecast_bucket = forecast_bucket
        self.prediction_freq = prediction_freq
    
    def run_continuously(self):
        cease_continuous_run = threading.Event()

        class ScheduleThread(threading.Thread):
            @classmethod
            def run(cls):
                while not cease_continuous_run.is_set():
                    schedule.run_pending()
                    time.sleep(self.interval)

        continuous_thread = ScheduleThread()
        continuous_thread.start()
        return cease_continuous_run

    def background_job(self):
        print('[BACKGROUND] Running background thread')
        self.forecast_data("hum")
        self.forecast_data("temp")
        self.forecast_data("rss")

    
    def query_data(self, measurement: str):
        query = 'from(bucket:"air-quality")' \
            ' |> range(start:-24h)'\
            f' |> filter(fn: (r) => r["_measurement"] == "{measurement}")' \
            ' |> filter(fn: (r) => r["_value"] != 0)' \
            ' |> aggregateWindow(every: 1m, fn: mean, createEmpty: false)'
        
        result = self.influx_client.query(query)
        raw = []
        for table in result:
            for record in table.records:
                raw.append((record.get_value(), record.get_time()))
        return raw

    def process_raw(self, raw):
        print("=== influxdb query into dataframe ===\n")
        df = pd.DataFrame(raw, columns=['y','ds'], index=None)
        df['ds'] = df['ds'].values.astype('<M8[s]')
        # BUG: InfluxDB returns time in UTC, but pandas wants it in local time
        df['ds'] +=  pd.to_timedelta(2, unit='h')
        df['y'] = df['y'].apply(lambda x: round(x, 2))
        df.set_index('ds')
        return df

    def fit_prophet(self, df):
        print("=== fitting prophet ===\n")
        m = Prophet()
        m.fit(df)

        future = m.make_future_dataframe(periods = self.prediction_periods, freq=self.prediction_freq)
        forecast = m.predict(future)
        return forecast
    
    def process_forecast(self, forecast, measurement: str):
        print("=== processing forecast ===\n")
        forecast['measurement'] = measurement
        cp = forecast[['ds', 'yhat', 'yhat_lower', 'yhat_upper','measurement']].copy()
        lines = [str(cp["measurement"][d]) 
                + ",type=forecast" 
                + " " 
                + "yhat=" + str(cp["yhat"][d]) + ","
                + "yhat_lower=" + str(cp["yhat_lower"][d]) + ","
                + "yhat_upper=" + str(cp["yhat_upper"][d])
                + " " + str(int(time.mktime(cp['ds'][d].timetuple()))) + "000000000" for d in range(len(cp))]
        return lines
    
    def publish_forecast(self, lines):
        self.influx_client.write(self.forecast_bucket, lines)
        print("=== published forecast ===\n")
    
    def forecast_data(self, measurement: str):
        raw = self.query_data(measurement)
        df = self.process_raw(raw)
        forecast = self.fit_prophet(df)
        lines = self.process_forecast(forecast, measurement)
        self.publish_forecast(lines)

    def start(self):
        schedule.every(self.interval).seconds.do(self.background_job)
        self.stop_run_continuously = self.run_continuously()

    def stop(self):
        self.stop_run_continuously.set()
    
    def start_foreground(self):
        schedule.every(self.interval).seconds.do(self.background_job)
        while True:
            schedule.run_pending()
            time.sleep(self.interval)


