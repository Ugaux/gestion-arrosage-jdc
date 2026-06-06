import asyncio
import threading
from contextlib import asynccontextmanager
from os import path
from pathlib import Path
from uuid import uuid4

import mock.config as cfg
import mock.esp_client as esp_mock
import requests
import uvicorn
import uvicorn_hmr
import watchfiles
from fastapi import FastAPI, WebSocket, status
from fastapi.responses import (
    FileResponse,
    HTMLResponse,
    JSONResponse,
    Response,
    StreamingResponse,
)
from fastapi.staticfiles import StaticFiles
from starlette.middleware.base import BaseHTTPMiddleware

RELOAD_ID = str(uuid4())  # new ID every process start
RELOAD_SCRIPT = b"""
<script>
    let lastReloadId = null;

    async function checkReload() {
      try {
        const res = await fetch("/dev-reload");
        const data = await res.json();
        if (lastReloadId && lastReloadId !== data.reload_id) {
          window.location.reload();  // reload browser on backend reload
        }
        lastReloadId = data.reload_id;
      } catch (err) {
        console.error(err);
      }
    }

    // Poll every 500ms
    setInterval(checkReload, 500);
  </script>
"""


class DevReloadMiddleware(BaseHTTPMiddleware):
    async def dispatch(self, request, call_next):
        response = await call_next(request)
        if response.headers.get("content-type", "").startswith("text/html"):
            body = b""
            async for chunk in response.body_iterator:
                body += chunk
            body = body.replace(b"</body>", RELOAD_SCRIPT + b"</body>")
            return Response(
                content=body,
                status_code=response.status_code,
                headers=dict(response.headers),
                media_type="text/html",
            )
        return response


app = FastAPI()
app.mount("/assets", StaticFiles(directory="www/assets"))

if cfg.DEV_MODE:
    app.add_middleware(DevReloadMiddleware)

    @app.get("/dev-reload")
    def dev_reload():
        # Return current reload identifier
        return {"reload_id": RELOAD_ID}


@app.get("/")
def index():
    return FileResponse("www/index.html")


@app.get("/add")
def add():
    return FileResponse("www/add.html")


@app.get("/edit")
def edit():
    return FileResponse("www/edit.html")


@app.get("/api/live/sensors")
def get_wifi():
    return {"ssid": "MyWifi", "ip": "192.168.1.10", "rssi": -60}


@app.get("/api/snapshot")
def get_snapshot():
    return esp_mock.mock_snapshot()


@app.get("/.well-known/appspecific/com.chrome.devtools.json")
def devtools_json():
    return Response(status_code=status.HTTP_204_NO_CONTENT)


if __name__ == "__main__":
    uvicorn.run(
        "start_server:app",
        host="0.0.0.0",
        port=8000,
        reload=True,
        reload_includes=["*.html", "*.css", "*.js"],
    )
