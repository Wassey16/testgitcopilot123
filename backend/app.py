import json, os, time
import eventlet
eventlet.monkey_patch()

from flask import Flask, render_template, send_from_directory
from flask_socketio import SocketIO
import paho.mqtt.client as mqtt
from dotenv import load_dotenv

from models import init_db, Session, Shot
from shot_classifier import Classifier

load_dotenv()
init_db()

MQTT_BROKER = os.getenv("MQTT_BROKER", "localhost")
MQTT_PORT   = int(os.getenv("MQTT_PORT", "1883"))
MQTT_USER   = os.getenv("MQTT_USER")
MQTT_PASS   = os.getenv("MQTT_PASSWORD")

TOPIC_GLOVE_RAW = "basket/glove/raw"
TOPIC_FOOT_RAW  = "basket/foot/raw"
TOPIC_HOOP_EVENT = "basket/hoop/event"

app = Flask(__name__, static_folder="../frontend", static_url_path="")
socketio = SocketIO(app, cors_allowed_origins="*")

classifier = Classifier()


# ============ MQTT CALLBACKS ============
def on_connect(client, userdata, flags, rc):
    print("MQTT connected", rc)
    client.subscribe([(TOPIC_GLOVE_RAW, 0), (TOPIC_FOOT_RAW, 0), (TOPIC_HOOP_EVENT, 0)])


def on_message(client, userdata, msg):
    payload = json.loads(msg.payload.decode())
    topic = msg.topic
    if topic == TOPIC_GLOVE_RAW:
        if payload.get("rel") == 1:
            result = classifier.on_release(payload["ts"], grip_peak=payload["fsr"][0] + payload["fsr"][1] + payload["fsr"][2])
            if result:
                persist_and_emit(result)

    elif topic == TOPIC_FOOT_RAW:
        apex_ts = payload.get("apex", 0)
        if apex_ts:
            result = classifier.on_jump_apex(apex_ts)
            if result:
                persist_and_emit(result)

    elif topic == TOPIC_HOOP_EVENT:
        result = classifier.on_score(payload["ts"])
        if result:
            persist_and_emit(result)


def persist_and_emit(data):
    # persist to DB
    with Session() as s:
        shot = Shot(
            ts_release=data["ts_release"],
            ts_apex=data["ts_apex"],
            classification=data["classification"],
            scored=data["scored"],
            grip_peak=data["grip_peak"],
        )
        s.add(shot)
        s.commit()
        payload = {
            "id": shot.id,
            "classification": shot.classification,
            "scored": shot.scored,
            "release": shot.ts_release,
            "apex": shot.ts_apex,
        }
        socketio.emit("shot", payload)
        print("Shot stored & emitted", payload)


# ============ FLASK ROUTES ============
@app.route("/")
def root():
    return send_from_directory(app.static_folder, "index.html")


@app.route("/shots")
def get_shots():
    with Session() as s:
        shots = s.query(Shot).order_by(Shot.id.desc()).limit(20).all()
        return {
            "shots": [
                dict(
                    id=sh.id,
                    classification=sh.classification,
                    scored=sh.scored,
                    release=sh.ts_release,
                    apex=sh.ts_apex,
                )
                for sh in shots
            ]
        }


def run():
    # MQTT Client
    client = mqtt.Client()
    client.username_pw_set(MQTT_USER, MQTT_PASS)
    client.on_connect = on_connect
    client.on_message = on_message
    client.connect(MQTT_BROKER, MQTT_PORT, 60)
    client.loop_start()

    socketio.run(app, host="0.0.0.0", port=5000)


if __name__ == "__main__":
    run()
