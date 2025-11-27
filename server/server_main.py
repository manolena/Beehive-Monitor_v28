from fastapi import FastAPI, HTTPException, Request
from pydantic import BaseModel
from typing import Optional, Dict, Any
from datetime import datetime
import sqlite3
import os

DB_PATH = os.environ.get("TELEMETRY_DB", "/data/telemetry.db")
API_KEY = os.environ.get("TELEMETRY_API_KEY", "changeme")  # ορίστε ασφαλές στο deploy

app = FastAPI(title="Beehive Telemetry Collector")

# Pydantic model για το POST payload
class TelemetryPayload(BaseModel):
    api_key: str
    fields: Dict[str, Any]
    lat: Optional[float] = None
    lon: Optional[float] = None
    ts: Optional[str] = None  # ISO timestamp προαιρετικά

def get_conn():
    conn = sqlite3.connect(DB_PATH, check_same_thread=False)
    conn.row_factory = sqlite3.Row
    return conn

# Δημιουργία πίνακα αν δεν υπάρχει
def init_db():
    conn = get_conn()
    cur = conn.cursor()
    cur.execute("""
    CREATE TABLE IF NOT EXISTS telemetry (
      id INTEGER PRIMARY KEY AUTOINCREMENT,
      ts TEXT NOT NULL,
      lat REAL,
      lon REAL,
      fields_json TEXT,
      created_at TEXT NOT NULL
    )
    """)
    conn.commit()
    conn.close()

@app.on_event("startup")
def startup():
    init_db()

@app.post("/api/telemetry")
async def post_telemetry(payload: TelemetryPayload, request: Request):
    # απλό api_key check
    if payload.api_key != API_KEY:
        raise HTTPException(status_code=401, detail="invalid api key")

    # timestamp
    if payload.ts:
        ts = payload.ts
    else:
        ts = datetime.utcnow().isoformat() + "Z"

    # αποθήκευση
    import json
    conn = get_conn()
    cur = conn.cursor()
    cur.execute(
        "INSERT INTO telemetry (ts, lat, lon, fields_json, created_at) VALUES (?, ?, ?, ?, ?)",
        (ts, payload.lat, payload.lon, json.dumps(payload.fields), datetime.utcnow().isoformat() + "Z")
    )
    conn.commit()
    rowid = cur.lastrowid
    conn.close()

    return {"status": "ok", "id": rowid}
    
@app.get("/api/telemetry")
def get_telemetry(limit: int = 100):
    conn = get_conn()
    cur = conn.cursor()
    cur.execute("SELECT id, ts, lat, lon, fields_json, created_at FROM telemetry ORDER BY id DESC LIMIT ?", (limit,))
    rows = cur.fetchall()
    import json
    out = []
    for r in rows:
        out.append({
            "id": r["id"],
            "ts": r["ts"],
            "lat": r["lat"],
            "lon": r["lon"],
            "fields": json.loads(r["fields_json"]),
            "created_at": r["created_at"]
        })
    conn.close()
    return out