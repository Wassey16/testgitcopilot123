from fastapi import FastAPI, WebSocket
import asyncio
import json
from bleak import BleakScanner, BleakClient

app = FastAPI()

# BLE Configuration
DEVICE_NAMES = {
    "foot": "ESP32_FOOT",
    "glove": "ESP32_GLOVE"
}
SERVICE_UUID = "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
CHAR_UUID = "beb5483e-36e1-4688-b7f5-ea07361b26a8"

# Jump Monitoring Variables
jump_active = False
jump_start_ts = None
jump_end_ts = None
peak_az = 0
raw_glove_data = []
max_fsr_per_finger = [0, 0]

# Helper Functions
def detect_jump_start(payload):
    global jump_active, jump_start_ts, peak_az
    az = payload.get("az", 0)
    ts = payload.get("ts", 0)

    if az > 20000 and not jump_active:
        jump_active = True
        jump_start_ts = ts
        peak_az = az
        return True
    return False

def detect_landing(payload):
    global jump_active, jump_end_ts
    az = payload.get("az", 0)
    ts = payload.get("ts", 0)

    if jump_active and az <= 16384:
        jump_active = False
        jump_end_ts = ts
        return True
    return False

def calculate_metrics():
    global jump_start_ts, jump_end_ts, peak_az, max_fsr_per_finger
    jump_duration = (jump_end_ts - jump_start_ts) / 1000  # Convert milliseconds to seconds
    jump_height = (peak_az - 16384) * 0.001 / 9.81  # Calculate jump height using physics formula
    return {
        "Jump Duration (s)": round(jump_duration, 2),
        "Jump Height (m)": round(jump_height, 2),
        "Max FSR Values (Finger 1 & 2)": max_fsr_per_finger
    }

async def summarize_jump(websocket: WebSocket):
    global jump_start_ts, jump_end_ts, peak_az, raw_glove_data, max_fsr_per_finger

    if jump_start_ts and jump_end_ts:
        metrics = calculate_metrics()
        summary = {
            "Jump Start Timestamp": jump_start_ts,
            "Jump End Timestamp": jump_end_ts,
            "Peak Vertical Acceleration (az)": peak_az,
            "Metrics": metrics,
            "Raw Glove Data During Jump": raw_glove_data
        }
        await websocket.send_json(summary)

    # Reset variables for next jump
    jump_start_ts = None
    jump_end_ts = None
    peak_az = 0
    max_fsr_per_finger = [0, 0]
    raw_glove_data = []

async def handle_notify(device_type: str, sender: int, data: bytearray, websocket: WebSocket):
    payload = json.loads(data.decode())

    if device_type == "foot":
        if detect_jump_start(payload):
            await websocket.send_text("Jump Started. Monitoring glove data...")
        elif detect_landing(payload):
            await websocket.send_text("Jump Ended.")
            await summarize_jump(websocket)

    elif device_type == "glove" and jump_active:
        raw_glove_data.append(payload)
        max_fsr_per_finger[0] = max(max_fsr_per_finger[0], payload.get("fsr1", 0))
        max_fsr_per_finger[1] = max(max_fsr_per_finger[1], payload.get("fsr2", 0))

async def connect_to_device(device_type: str, device, websocket: WebSocket):
    try:
        async with BleakClient(device.address) as client:
            await websocket.send_text(f"Connected to {device_type}. Subscribing to notifications...")
            await client.start_notify(
                CHAR_UUID,
                lambda sender, data: asyncio.create_task(
                    handle_notify(device_type, sender, data, websocket)
                )
            )
            while True:
                await asyncio.sleep(1)
    except Exception as e:
        await websocket.send_text(f"[{device_type}] Connection error: {e}")

async def scan_and_connect(websocket: WebSocket):
    await websocket.send_text("Scanning for BLE devices...")
    devices = await BleakScanner.discover(timeout=10.0)

    tasks = []
    for dtype, name in DEVICE_NAMES.items():
        dev = next((d for d in devices if d.name == name), None)
        if dev:
            await websocket.send_text(f"Found {dtype} device: {name} ({dev.address})")
            tasks.append(connect_to_device(dtype, dev, websocket))
        else:
            await websocket.send_text(f"[Warning] Device '{name}' ({dtype}) not found.")

    if tasks:
        await asyncio.gather(*tasks)

@app.websocket("/ws")
async def websocket_endpoint(websocket: WebSocket):
    await websocket.accept()
    try:
        await scan_and_connect(websocket)
    except Exception as e:
        await websocket.send_text(f"Error: {e}")