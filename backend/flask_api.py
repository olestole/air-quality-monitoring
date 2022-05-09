from flask import Flask

app = Flask(__name__)


@app.route('/sensor/temp')
def hello():
    return 'Hello World!'


def run():
    app.run(debug=True, use_reloader=False)
