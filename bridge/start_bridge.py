import argparse
import asyncio
import json
import socket
import threading
import webbrowser
from os import environ, path
from pathlib import Path
from urllib.parse import quote

import mock.esp_client as esp_mock
import requests
import uvicorn
import websockets
from bridge_config import ESP_BASE_URL, EXCLUDED_HEADERS
from fastapi import FastAPI, Request, WebSocket, status
from fastapi.responses import FileResponse, RedirectResponse, Response
from fastapi.staticfiles import StaticFiles

DEV_MODE = environ.get("DEV_MODE", "0") == "1"
MOCK_ESP_API = environ.get("MOCK_ESP_API", "0") == "1"
WEBUI_FILEPATH = path.join(Path(__file__).parent.parent, "webui", "src")


class NoCacheStaticFiles(StaticFiles):
    def file_response(self, path, stat_result, scope):
        response = super().file_response(path, stat_result, scope)
        response.headers["Cache-Control"] = "no-store"
        response.headers["Pragma"] = "no-cache"
        response.headers["Expires"] = "0"
        return response


class CachingStaticFiles(StaticFiles):
    def file_response(self, path, stat_result, scope):
        response = super().file_response(path, stat_result, scope)
        response.headers["Cache-Control"] = "public, max-age=31536000, immutable"
        return response


app = FastAPI()
if DEV_MODE:
    Static = NoCacheStaticFiles
else:
    Static = CachingStaticFiles
app.mount("/assets", Static(directory=path.join(WEBUI_FILEPATH, "assets")))


@app.get("/")
def index():
    print("Sending index.html...")
    with open(path.join(WEBUI_FILEPATH, "index.html"), "r", encoding="utf-8") as f:
        html = f.read()
    with open(path.join(WEBUI_FILEPATH, "version.json"), "r", encoding="utf-8") as f:
        version = json.load(f)["version"]
    html = html.replace("%VERSION%", version)  # your value here
    response = Response(content=html, media_type="text/html")
    response.headers["Cache-Control"] = "no-cache"
    return response


"""
@app.get("/version")
def version():
    response = FileResponse(path.join(WEBUI_FILEPATH, "version.json"))
    response.headers["Cache-Control"] = "no-cache"
    return response
"""


"""
@app.get("/404")
def not_found(url):
    return FileResponse(path.join(WEBUI_FILEPATH, "404.html"), status_code=404)
"""


"""
@app.exception_handler(404)
async def custom_404_handler(request: Request, exc):
    return RedirectResponse(url=f"/404?url={quote(str(request.url),safe='')}", status_code=302)
"""


@app.exception_handler(404)
async def custom_404_handler(request: Request, exc):
    return FileResponse(path.join(WEBUI_FILEPATH, "404.html"), status_code=404)


def remove_excluded_headers(headers):
    if headers in EXCLUDED_HEADERS:
        del headers


@app.get("/device/info")
def get_device_info():
    if MOCK_ESP_API:
        return esp_mock.mock_device_info()
    r = requests.get(f"{ESP_BASE_URL}/api/status", timeout=1)
    remove_excluded_headers(r.headers)
    return r.json()


@app.get("/api/v1/snapshot")
def get_api_snapshot():
    if MOCK_ESP_API:
        return esp_mock.mock_snapshot()
    return {
        "wifi": {"ssid": "REALESP32", "ip": "192.168.1.42", "rssi": -10000},
        "valve": {"duration": 15, "openTime": 20},
    }


@app.post("/api/v1/zones/{zone_id}/toggle")
async def toggle_zone(zone_id: str, payload: dict):
    if MOCK_ESP_API:
        return {"ok"}
    return requests.post(f"{ESP_BASE_URL}/api/{zone_id}/command", json=payload, timeout=1).json()


@app.websocket("/ws")
async def websocket_proxy(client_ws: WebSocket):
    await client_ws.accept()

    async with websockets.connect("ws://192.168.1.50/ws") as esp_ws:

        async def client_to_esp():
            while True:
                msg = await client_ws.receive_text()
                await esp_ws.send(msg)

        async def esp_to_client():
            while True:
                msg = await esp_ws.recv(decode=True)
                await client_ws.send_text(msg)

        await asyncio.gather(client_to_esp(), esp_to_client())


@app.get("/.well-known/appspecific/com.chrome.devtools.json")
def devtools_json():
    return Response(status_code=status.HTTP_204_NO_CONTENT)


if DEV_MODE:

    def get_local_ip():
        s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        try:
            # doesn't actually send data
            s.connect(("8.8.8.8", 80))
            ip = s.getsockname()[0]
        finally:
            s.close()
        return ip

    # --- Middleware to inject livereload script --- #
    @app.middleware("http")
    async def livereload_injector(request: Request, call_next):
        response = await call_next(request)
        content_type = response.headers.get("content-type", "")
        if "text/html" in content_type.lower():
            body = b""
            async for chunk in response.body_iterator:
                body += chunk

            try:
                html = body.decode("utf-8")
            except UnicodeDecodeError:
                return Response(content=body, status_code=response.status_code, headers=dict(response.headers))

            # Inject script before </body>
            snippet = f'<script src="http://{get_local_ip()}:35729/livereload.js"></script>'
            if "</body>" in html:
                html = html.replace("</body>", snippet + "\n</body>")
            else:
                html += snippet
            headers = dict(response.headers)
            headers["Cache-Control"] = "no-store, no-cache, must-revalidate, max-age=0"
            headers["Pragma"] = "no-cache"
            headers["Expires"] = "0"
            headers.pop("etag", None)
            headers.pop("last-modified", None)
            headers.pop("content-length", None)  # remove stale Content-Length

            return Response(
                content=html,
                status_code=response.status_code,
                headers=headers,
                media_type="text/html",
            )

        return response


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--dev_mode", action="store_true")
    parser.add_argument("--mock_esp_api", action="store_true")
    args = parser.parse_args()
    dev_mode_arg = args.dev_mode
    mock_esp_api_arg = args.mock_esp_api

    environ["DEV_MODE"] = "1" if dev_mode_arg else "0"
    environ["MOCK_ESP_API"] = "1" if mock_esp_api_arg else "0"
    print("Starting bridge...")
    if dev_mode_arg:
        print("-> DEV MODE ENABLED")
    if mock_esp_api_arg:
        print("-> MOCKING ESP API")

    if dev_mode_arg:
        threading.Timer(1.0, lambda: webbrowser.open("http://localhost:8000")).start()
    uvicorn.run("start_bridge:app", host="0.0.0.0", port=8000, reload=dev_mode_arg)
