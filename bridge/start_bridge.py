import argparse
import asyncio
import json
import os
import socket
import threading
import webbrowser
from pathlib import Path

import requests
import uvicorn
import websockets
from fastapi import FastAPI, HTTPException, Request, WebSocket, status
from fastapi.responses import Response
from fastapi.staticfiles import StaticFiles
from starlette.websockets import WebSocketDisconnect

import mock.esp_client as esp_mock
from bridge_config import ESP_BASE_URL, EXCLUDED_HEADERS

DEV_MODE = os.environ.get("DEV_MODE", "0") == "1"
MOCK_ESP_API = os.environ.get("MOCK_ESP_API", "0") == "1"
WEBUI_FILEPATH = os.path.join(Path(__file__).parent.parent, "webui", "src")

if DEV_MODE:
    VERSION = "dev"
else:
    with open(os.path.join(WEBUI_FILEPATH, "version.json"), "r", encoding="utf-8") as f:
        VERSION = json.load(f)["version"]


def make_static_files():

    headers = (
        {
            "Cache-Control": "no-store",
            "Expires": "0",
        }
        if DEV_MODE
        else {
            "Cache-Control": "public, max-age=31536000, immutable",
        }
    )

    class StaticFilesWithHeaders(StaticFiles):
        async def get_response(self, path: str, scope):
            if path.endswith((".html", ".css", ".js")):
                full_path, stat_result = self.lookup_path(path)

                if stat_result is None:
                    return await super().get_response(path, scope)

                with open(full_path, "r", encoding="utf-8") as f:
                    content = f.read().replace("%VERSION%", VERSION)

                if path.endswith(".html"):
                    media_type = "text/html"
                elif path.endswith(".css"):
                    media_type = "text/css"
                else:
                    media_type = "application/javascript"

                response = Response(content, media_type=media_type)
                response.headers.update(headers)
                return response

            response = await super().get_response(path, scope)
            response.headers.update(headers)
            return response

    return StaticFilesWithHeaders


app = FastAPI()
app.mount("/assets", make_static_files()(directory=os.path.join(WEBUI_FILEPATH, "assets")))


@app.get("/.well-known/appspecific/com.chrome.devtools.json")
def devtools_json():
    return Response(status_code=status.HTTP_204_NO_CONTENT)


def create_api_response(r):
    # Remove excluded headers
    headers = {k: v for k, v in r.headers.items() if k.lower() not in EXCLUDED_HEADERS}

    return Response(
        content=r.content,
        status_code=r.status_code,
        headers=headers,
        media_type=r.headers.get("content-type"),
    )


@app.get("/api/status")
def get_device_info():
    if MOCK_ESP_API:
        return esp_mock.mock_device_info()

    r = requests.get(f"{ESP_BASE_URL}/api/status", timeout=1)
    return create_api_response(r)


@app.websocket("/ws")
async def websocket_proxy(client_ws: WebSocket):
    await client_ws.accept()

    if MOCK_ESP_API:

        try:
            await client_ws.send_json(
                {
                    "type": "SNAPSHOT",
                    "payload": [
                        {
                            "topic": "valves",
                            "event": "updateAll",
                            "payload": [
                                {"index": 0, "name": "Expander-GPIO:0", "is_checked": False},
                                {"index": 1, "name": "Expander-GPIO:1", "is_checked": False},
                                {"index": 2, "name": "Expander-GPIO:2", "is_checked": True},
                                {"index": 3, "name": "Expander-GPIO:3", "is_checked": False},
                                {"index": 4, "name": "Expander-GPIO:4", "is_checked": False},
                                {"index": 5, "name": "Expander-GPIO:5", "is_checked": True},
                                {"index": 6, "name": "Expander-GPIO:6", "is_checked": False},
                                {"index": 7, "name": "Expander-GPIO:7", "is_checked": False},
                            ],
                        }
                    ],
                }
            )
            while True:
                # Receive a text message
                data = await client_ws.receive_text()
                message = json.loads(data)
                action = message["payload"]["action"]
                print(f"Received: {data}")

                if action == "toggleValve":
                    await client_ws.send_json(
                        {
                            "type": "EVENT",
                            "topic": "valves",
                            "event": "updateAll",
                            "payload": [{"index": 5, "name": "Expander-GPIO:5", "is_checked": True}],
                        }
                    )
                    await client_ws.send_json(
                        {
                            "id": message["id"],
                            "type": "ACK",
                            "ok": True,
                        }
                    )
                else:
                    await client_ws.send_json(
                        {
                            "id": message["id"],
                            "type": "ACK",
                            "ok": False,
                            "error": {"name": "Unknown action", "message": f"Could not execute '{action}'"},
                        }
                    )

        except WebSocketDisconnect:
            print("Client disconnected")

    else:

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


@app.get("/{path:path}")
def spa_index(path: str):
    if path.startswith("api/"):
        raise HTTPException(404)

    print("Sending index.html...")
    with open(os.path.join(WEBUI_FILEPATH, "index.html"), "r", encoding="utf-8") as f:
        html = f.read()
    html = html.replace("%VERSION%", VERSION)
    response = Response(content=html, media_type="text/html")
    response.headers["Cache-Control"] = "no-store"
    # "no-cache, must-revalidate" -> if implementing etag and "304" response
    return response


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

    if not DEV_MODE:
        return response

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

    os.environ["DEV_MODE"] = "1" if dev_mode_arg else "0"
    os.environ["MOCK_ESP_API"] = "1" if mock_esp_api_arg else "0"
    print("Starting bridge...")
    if dev_mode_arg:
        print("-> DEV MODE ENABLED")
    if mock_esp_api_arg:
        print("-> MOCKING ESP API")

    if dev_mode_arg:
        threading.Timer(1.0, lambda: webbrowser.open("http://localhost:8000")).start()
    uvicorn.run("start_bridge:app", host="0.0.0.0", port=8000, reload=dev_mode_arg)
