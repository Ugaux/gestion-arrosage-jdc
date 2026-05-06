import asyncio
import logging
import threading
import webbrowser
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
from starlette.requests import Request

RELOAD_SCRIPT = b"""
<script>
  async function pollReload(version = 0) {
    try {
      const res = await fetch(`/dev-reload?version=${version}`);
      const data = await res.json();
      location.reload();
    } catch {
      setTimeout(() => pollReload(version), 1000);
    }
  }
  pollReload();
</script>
"""
logger = logging.getLogger("uvicorn")
reload_condition = asyncio.Condition()
reload_version = 0
shutdown_event = asyncio.Event()


async def watch_files():
    global reload_version
    async for _ in watchfiles.awatch("www", stop_event=shutdown_event):
        logger.info(f"Refreshing browser because files in /wwww have changed")
        async with reload_condition:
            reload_version += 1
            reload_condition.notify_all()  # wake every waiting tab


@asynccontextmanager
async def lifespan(app: FastAPI):
    task = asyncio.create_task(watch_files())
    yield
    shutdown_event.set()
    task.cancel()


app = FastAPI(lifespan=lifespan)
app.mount("/assets", StaticFiles(directory="www/assets"))


if cfg.DEV_MODE:

    class DevReloadMiddleware(BaseHTTPMiddleware):
        async def dispatch(self, request: Request, call_next):
            response = await call_next(request)
            if response.headers.get("content-type", "").startswith("text/html"):
                body = b""
                async for chunk in response.body_iterator:  # type: ignore[attr-defined]
                    body += chunk
                body = body.replace(b"</body>", RELOAD_SCRIPT + b"</body>")
                headers = dict(response.headers)
                del headers["content-length"]  # remove stale Content-Length

                return Response(
                    content=body,
                    status_code=response.status_code,
                    headers=headers,
                    media_type="text/html",
                )
            return response

    app.add_middleware(DevReloadMiddleware)

    @app.get("/dev-reload")
    async def dev_reload(version: int = 0):
        async with reload_condition:
            # Wait until the version advances past what the tab last saw
            await reload_condition.wait_for(lambda: reload_version > version)
            return {"reload": True, "version": reload_version}


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
    threading.Timer(1.0, lambda: webbrowser.open("http://localhost:8000")).start()
    uvicorn.run("start_server:app", host="0.0.0.0", port=8000, reload=True)
