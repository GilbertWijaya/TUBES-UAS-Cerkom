# # # # Jalankan server:
# # # # uvicorn server_api:asgi_app --host 0.0.0.0 --port 8000

# # server_api.py
from fastapi import FastAPI, Request
from fastapi.middleware.cors import CORSMiddleware
import socketio
import tensorflow as tf
import numpy as np
import cv2
import base64
import time

# Load model
model = tf.keras.models.load_model('model_egg.h5')

# SocketIO server
sio = socketio.AsyncServer(
    async_mode="asgi",
    cors_allowed_origins="*"
)

# FastAPI app
app = FastAPI()

# Gabungkan SocketIO & FastAPI dalam satu ASGI app
asgi_app = socketio.ASGIApp(
    sio,
    other_asgi_app=app,
    socketio_path="socket.io"
)

# CORS
app.add_middleware(
    CORSMiddleware,
    allow_origins=["*"],
    allow_methods=["*"],
    allow_headers=["*"],
)

@app.post("/upload")
async def upload_image(request: Request):

    # Baca byte dari ESP32-CAM
    image_bytes = await request.body()
    npimg = np.frombuffer(image_bytes, np.uint8)
    img = cv2.imdecode(npimg, cv2.IMREAD_COLOR)

    # Simpan gambar asli untuk ditampilkan di dashboard
    img_original = img.copy()

    # Convert ke RGB → untuk input model
    img_rgb = cv2.cvtColor(img, cv2.COLOR_BGR2RGB)
    img_resized = cv2.resize(img_rgb, (64, 64))
    img_norm = img_resized.astype("float32") / 255.0
    img_input = np.expand_dims(img_norm, axis=0)

    # Prediksi
    pred = model.predict(img_input)[0][0]

    # Tentukan label
    label = "Telur terdeteksi" if pred < 0.5 else "Tidak ada telur"
    confidence = float(1 - pred if pred < 0.5 else pred)

    # Encode gambar asli ke base64
    _, jpeg = cv2.imencode(".jpg", img_original)
    b64img = base64.b64encode(jpeg).decode("utf-8")

    # Kirim ke dashboard via Socket.IO
    await sio.emit("update", {
        "label": label,
        "confidence": round(confidence, 2),
        "time": time.strftime("%H:%M:%S"),
        "image": f"data:image/jpeg;base64,{b64img}",
        "detected": label == "Telur terdeteksi"
    })

    return {"label": label, "confidence": confidence}

@sio.event
async def connect(sid, environ):
    print(f"Dashboard terhubung: {sid}")
